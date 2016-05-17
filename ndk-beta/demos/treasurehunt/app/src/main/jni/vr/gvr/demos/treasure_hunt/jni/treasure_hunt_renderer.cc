/* Copyright 2016 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vr/gvr/demos/treasure_hunt/jni/treasure_hunt_renderer.h"

#include <android/log.h>
#include <assert.h>
#include <cmath>

#define LOG_TAG "TreasureHuntCPP"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace {
static const int kTextureFormat = GL_RGB;
static const int kTextureType = GL_UNSIGNED_BYTE;

static const float kZNear = 1.0f;
static const float kZFar = 100.0f;

static const int kCoordsPerVertex = 3;

static const uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

static const char* kGridFragmentShader =
    "precision mediump float;\n"
    "varying vec4 v_Color;\n"
    "varying vec3 v_Grid;\n"
    "\n"
    "void main() {\n"
    "    float depth = gl_FragCoord.z / gl_FragCoord.w;\n"
    "    if ((mod(abs(v_Grid.x), 10.0) < 0.1) ||\n"
    "        (mod(abs(v_Grid.z), 10.0) < 0.1)) {\n"
    "      gl_FragColor = max(0.0, (90.0-depth) / 90.0) *\n"
    "                     vec4(1.0, 1.0, 1.0, 1.0) + \n"
    "                     min(1.0, depth / 90.0) * v_Color;\n"
    "    } else {\n"
    "      gl_FragColor = v_Color;\n"
    "    }\n"
    "}\n";

static const char* kLightVertexShader =
    "uniform mat4 u_Model;\n"
    "uniform mat4 u_MVP;\n"
    "uniform mat4 u_MVMatrix;\n"
    "uniform vec3 u_LightPos;\n"
    "attribute vec4 a_Position;\n"
    "\n"
    "attribute vec4 a_Color;\n"
    "attribute vec3 a_Normal;\n"
    "\n"
    "varying vec4 v_Color;\n"
    "varying vec3 v_Grid;\n"
    "\n"
    "void main() {\n"
    "  v_Grid = vec3(u_Model * a_Position);\n"
    "  vec3 modelViewVertex = vec3(u_MVMatrix * a_Position);\n"
    "  vec3 modelViewNormal = vec3(u_MVMatrix * vec4(a_Normal, 0.0));\n"
    "  float distance = length(u_LightPos - modelViewVertex);\n"
    "  vec3 lightVector = normalize(u_LightPos - modelViewVertex);\n"
    "  float diffuse = max(dot(modelViewNormal, lightVector), 0.5);\n"
    "  diffuse = diffuse * (1.0 / (1.0 + (0.00001 * distance * distance)));\n"
    "  v_Color = a_Color * diffuse;\n"
    "  gl_Position = u_MVP * a_Position;\n"
    "}\n";

static const char* kPassthroughFragmentShader =
    "precision mediump float;\n"
    "\n"
    "varying vec4 v_Color;\n"
    "\n"
    "void main() {\n"
    "  gl_FragColor = v_Color;\n"
    "}\n";

// Sound file in APK assets.
static const char* kSoundFile = "cube_sound.wav";

static std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix) {
  // Note that this performs a *tranpose* to a column-major matrix array, as
  // expected by GL.
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

static gvr::Mat4f PoseToMatrix(const gvr::HeadPose& head_pose) {
  gvr::Mat4f result;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      result.m[i][j] = head_pose.rotation.m[i][j];
    }
  }
  result.m[0][3] = head_pose.position.x;
  result.m[1][3] = head_pose.position.y;
  result.m[2][3] = head_pose.position.z;
  result.m[3][0] = 0.0f;
  result.m[3][1] = 0.0f;
  result.m[3][2] = 0.0f;
  result.m[3][3] = 1.0f;

  return result;
}

static std::array<float, 4> MatrixVectorMul(const gvr::Mat4f& matrix,
                                            const std::array<float, 4>& vec) {
  std::array<float, 4> result;
  for (int i = 0; i < 4; ++i) {
    result[i] = 0;
    for (int k = 0; k < 4; ++k) {
      result[i] += matrix.m[i][k]*vec[k];
    }
  }
  return result;
}

static gvr::Mat4f MatrixMul(const gvr::Mat4f& matrix1,
                            const gvr::Mat4f& matrix2) {
  gvr::Mat4f result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += matrix1.m[i][k]*matrix2.m[k][j];
      }
    }
  }
  return result;
}

static gvr::Mat4f PerspectiveMatrixFromView(const gvr::Rectf& fov, float z_near,
                                            float z_far) {
  gvr::Mat4f result;
  const float x_left = -std::tan(fov.left * M_PI / 180.0f) * z_near;
  const float x_right = std::tan(fov.right * M_PI / 180.0f) * z_near;
  const float y_bottom = -std::tan(fov.bottom * M_PI / 180.0f) * z_near;
  const float y_top = std::tan(fov.top * M_PI / 180.0f) * z_near;
  const float zero = 0.0f;

  assert(x_left < x_right && y_bottom < y_top && z_near < z_far &&
         z_near > zero && z_far > zero);
  const float X = (2 * z_near) / (x_right - x_left);
  const float Y = (2 * z_near) / (y_top - y_bottom);
  const float A = (x_right + x_left) / (x_right - x_left);
  const float B = (y_top + y_bottom) / (y_top - y_bottom);
  const float C = (z_near + z_far) / (z_near - z_far);
  const float D = (2 * z_near * z_far) / (z_near - z_far);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
    }
  }
  result.m[0][0] = X;
  result.m[0][2] = A;
  result.m[1][1] = Y;
  result.m[1][2] = B;
  result.m[2][2] = C;
  result.m[2][3] = D;
  result.m[3][2] = -1;

  return result;
}

static gvr::Rectf ModulateRect(const gvr::Rectf& rect, float width,
                               float height) {
  gvr::Rectf result = {rect.left * width, rect.right * width,
                       rect.bottom * height, rect.top * height};
  return result;
}

static gvr::Recti CalculatePixelSpaceRect(const gvr::Sizei& texture_size,
                                          const gvr::Rectf& texture_rect) {
  float width = static_cast<float>(texture_size.width);
  float height = static_cast<float>(texture_size.height);
  gvr::Rectf rect = ModulateRect(texture_rect, width, height);
  gvr::Recti result = {
      static_cast<int>(rect.left), static_cast<int>(rect.right),
      static_cast<int>(rect.bottom), static_cast<int>(rect.top)};
  return result;
}

// Generate a random floating point number between 0 and 1. Use intermediate
// double precision values to avoid quantisation in the cast of a 32bit int to
// a float.
static float RandomUniformFloat() {
  return static_cast<float>(static_cast<double>(rand()) /  // NOLINT
                            static_cast<double>(RAND_MAX));
}

static void CheckGLError(const char* label) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    LOGW("GL error @ %s: %d", label, gl_error);
  }
  assert(glGetError() == GL_NO_ERROR);
}

}  // namespace

TreasureHuntRenderer::TreasureHuntRenderer(
    gvr_context* gvr_context, std::unique_ptr<gvr::AudioApi> gvr_audio_api)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      gvr_audio_api_(std::move(gvr_audio_api)),
      floor_vertices_(nullptr),
      floor_colors_(nullptr),
      floor_normals_(nullptr),
      cube_vertices_(nullptr),
      cube_colors_(nullptr),
      cube_found_colors_(nullptr),
      cube_normals_(nullptr),
      light_pos_world_space_({0.0f, 2.0f, 0.0f, 1.0f}),
      object_distance_(3.5f),
      floor_depth_(20.0f),
      sound_id_(-1) {}

TreasureHuntRenderer::~TreasureHuntRenderer() {
  // Join the audio initialization thread in case it still exists.
  audio_initialization_thread_.join();
}

void TreasureHuntRenderer::InitializeGl() {
  gvr_api_->InitializeGl();

  glClearColor(0.1f, 0.1f, 0.1f, 0.5f);  // Dark background so text shows up.

  cube_vertices_ = world_layout_data_.CUBE_COORDS.data();
  cube_colors_ = world_layout_data_.CUBE_COLORS.data();
  cube_found_colors_ = world_layout_data_.CUBE_FOUND_COLORS.data();
  cube_normals_ = world_layout_data_.CUBE_NORMALS.data();
  floor_vertices_ = world_layout_data_.FLOOR_COORDS.data();
  floor_normals_ = world_layout_data_.FLOOR_NORMALS.data();
  floor_colors_ = world_layout_data_.FLOOR_COLORS.data();

  int vertex_shader = LoadGLShader(GL_VERTEX_SHADER, &kLightVertexShader);
  int grid_shader = LoadGLShader(GL_FRAGMENT_SHADER, &kGridFragmentShader);
  int pass_through_shader = LoadGLShader(GL_FRAGMENT_SHADER,
                                         &kPassthroughFragmentShader);

  cube_program_ = glCreateProgram();
  glAttachShader(cube_program_, vertex_shader);
  glAttachShader(cube_program_, pass_through_shader);
  glLinkProgram(cube_program_);
  glUseProgram(cube_program_);

  cube_position_param_ = glGetAttribLocation(cube_program_, "a_Position");
  cube_normal_param_ = glGetAttribLocation(cube_program_, "a_Normal");
  cube_color_param_ = glGetAttribLocation(cube_program_, "a_Color");

  cube_model_param_ = glGetUniformLocation(cube_program_, "u_Model");
  cube_modelview_param_ = glGetUniformLocation(cube_program_, "u_MVMatrix");
  cube_modelview_projection_param_ =
      glGetUniformLocation(cube_program_, "u_MVP");
  cube_light_pos_param_ = glGetUniformLocation(cube_program_, "u_LightPos");

  CheckGLError("Cube program params");

  floor_program_ = glCreateProgram();
  glAttachShader(floor_program_, vertex_shader);
  glAttachShader(floor_program_, grid_shader);
  glLinkProgram(floor_program_);
  glUseProgram(floor_program_);

  CheckGLError("Floor program");

  floor_position_param_ = glGetAttribLocation(floor_program_, "a_Position");
  floor_normal_param_ = glGetAttribLocation(floor_program_, "a_Normal");
  floor_color_param_ = glGetAttribLocation(floor_program_, "a_Color");

  floor_model_param_ = glGetUniformLocation(floor_program_, "u_Model");
  floor_modelview_param_ = glGetUniformLocation(floor_program_, "u_MVMatrix");
  floor_modelview_projection_param_ =
      glGetUniformLocation(floor_program_, "u_MVP");
  floor_light_pos_param_ = glGetUniformLocation(floor_program_, "u_LightPos");

  CheckGLError("Floor program params");

  // Object first appears directly in front of user.
  model_cube_ = {1.0f, 0.0f, 0.0f, 0.0f,
                 0.0f, 0.707f, -0.707f, 0.0f,
                 0.0f, 0.707f, 0.707f, -object_distance_,
                 0.0f, 0.0f, 0.0f, 1.0f};
  model_floor_ = {1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, -floor_depth_,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f};

  render_size_ = gvr_api_->GetRecommendedRenderTargetSize();

  framebuffer_handle_.reset(new gvr::OffscreenFramebufferHandle(
      gvr_api_->CreateOffscreenFramebuffer(render_size_)));

  render_params_list_.reset(new gvr::RenderParamsList(
      gvr_api_->CreateEmptyRenderParamsList()));

  // Initialize audio engine and preload sample in a separate thread to avoid
  // any delay during construction and app initialization. Only do this once.
  if (!audio_initialization_thread_.joinable()) {
    audio_initialization_thread_ =
        std::thread(&TreasureHuntRenderer::LoadAndPlayCubeSound, this);
  }
}

void TreasureHuntRenderer::DrawFrame() {
  render_params_list_->SetToRecommendedRenderParams();
  framebuffer_handle_->SetActive();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos +=
      kPredictionTimeWithoutVsyncNanos;
  head_pose_ = gvr_api_->GetHeadPoseInStartSpace(target_time);

  gvr::Mat4f left_eye_view_matrix =
      MatrixMul(gvr_api_->GetEyeFromHeadMatrix(GVR_LEFT_EYE),
                head_pose_.object_from_reference_matrix);
  gvr::Mat4f right_eye_view_matrix =
      MatrixMul(gvr_api_->GetEyeFromHeadMatrix(GVR_RIGHT_EYE),
                head_pose_.object_from_reference_matrix);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  DrawEye(GVR_LEFT_EYE, left_eye_view_matrix,
          render_params_list_->GetRenderParams(0));
  DrawEye(GVR_RIGHT_EYE, right_eye_view_matrix,
          render_params_list_->GetRenderParams(1));

  // Bind back to the default framebuffer.
  gvr_api_->SetDefaultFramebufferActive();

  gvr_api_->DistortOffscreenFramebufferToScreen(
      *framebuffer_handle_, *render_params_list_, &head_pose_, &target_time);

  CheckGLError("onDrawFrame");

  // Update audio head rotation in audio API.
  gvr_audio_api_->SetHeadRotation(head_pose_.rotation);
  gvr_audio_api_->Update();
}

void TreasureHuntRenderer::OnTriggerEvent() {
  if (IsLookingAtObject()) {
    HideObject();
  }
}

void TreasureHuntRenderer::OnPause() {
  gvr_api_->PauseTracking();
  gvr_audio_api_->Pause();
}

void TreasureHuntRenderer::OnResume() {
  gvr_api_->RefreshViewerProfile();
  gvr_api_->ResumeTracking();
  gvr_audio_api_->Resume();
}

/**
 * Converts a raw text file, saved as a resource, into an OpenGL ES shader.
 *
 * @param type  The type of shader we will be creating.
 * @param resId The resource ID of the raw text file.
 * @return The shader object handler.
 */
int TreasureHuntRenderer::LoadGLShader(int type, const char** shadercode) {
  int shader = glCreateShader(type);
  glShaderSource(shader, 1, shadercode, nullptr);
  glCompileShader(shader);

  // Get the compilation status.
  int* compileStatus = new int[1];
  glGetShaderiv(shader, GL_COMPILE_STATUS, compileStatus);

  // If the compilation failed, delete the shader.
  if (compileStatus[0] == 0) {
      glDeleteShader(shader);
      shader = 0;
  }

  return shader;
}

/**
 * Draws a frame for an eye.
 *
 * @param eye The eye to render. Includes all required transformations.
 */
void TreasureHuntRenderer::DrawEye(gvr::Eye eye, const gvr::Mat4f& view_matrix,
                                   const gvr::RenderParams& params) {
  const gvr::Recti pixel_rect =
      CalculatePixelSpaceRect(render_size_, params.eye_viewport_bounds);

  glViewport(pixel_rect.left, pixel_rect.bottom,
             pixel_rect.right - pixel_rect.left,
             pixel_rect.top - pixel_rect.bottom);
  glScissor(pixel_rect.left, pixel_rect.bottom,
            pixel_rect.right - pixel_rect.left,
            pixel_rect.top - pixel_rect.bottom);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  CheckGLError("ColorParam");

  // Set the position of the light
  light_pos_eye_space_  = MatrixVectorMul(view_matrix, light_pos_world_space_);
  gvr::Mat4f perspective =
      PerspectiveMatrixFromView(params.eye_fov, kZNear, kZFar);

  modelview_ = MatrixMul(view_matrix, model_cube_);
  modelview_projection_cube_ = MatrixMul(perspective, modelview_);
  DrawCube();

  // Set modelview_ for the floor, so we draw floor in the correct location
  modelview_ = MatrixMul(view_matrix, model_floor_);
  modelview_projection_floor_ = MatrixMul(perspective, modelview_);
  DrawFloor();
}

void TreasureHuntRenderer::DrawCube() {
  glUseProgram(cube_program_);

  glUniform3fv(cube_light_pos_param_, 1, light_pos_eye_space_.data());

  // Set the Model in the shader, used to calculate lighting
  glUniformMatrix4fv(cube_model_param_, 1, GL_FALSE,
                     MatrixToGLArray(model_cube_).data());

  // Set the ModelView in the shader, used to calculate lighting
  glUniformMatrix4fv(cube_modelview_param_, 1, GL_FALSE,
                     MatrixToGLArray(modelview_).data());

  // Set the position of the cube
  glVertexAttribPointer(cube_position_param_, kCoordsPerVertex, GL_FLOAT,
                        false, 0, cube_vertices_);

  // Set the ModelViewProjection matrix in the shader.
  glUniformMatrix4fv(cube_modelview_projection_param_, 1, GL_FALSE,
                     MatrixToGLArray(modelview_projection_cube_).data());

  // Set the normal positions of the cube, again for shading
  glVertexAttribPointer(
      cube_normal_param_, 3, GL_FLOAT, false, 0, cube_normals_);
  glVertexAttribPointer(cube_color_param_, 4, GL_FLOAT, false, 0,
                        IsLookingAtObject() ? cube_found_colors_ :cube_colors_);

  glEnableVertexAttribArray(cube_position_param_);
  glEnableVertexAttribArray(cube_normal_param_);
  glEnableVertexAttribArray(cube_color_param_);

  glDrawArrays(GL_TRIANGLES, 0, 36);
  CheckGLError("Drawing cube");
}

void TreasureHuntRenderer::DrawFloor() {
  glUseProgram(floor_program_);

  // Set ModelView, MVP, position, normals, and color.
  glUniform3fv(floor_light_pos_param_, 1, light_pos_eye_space_.data());
  glUniformMatrix4fv(floor_model_param_, 1, GL_FALSE,
                     MatrixToGLArray(model_floor_).data());
  glUniformMatrix4fv(floor_modelview_param_, 1, GL_FALSE,
                     MatrixToGLArray(modelview_).data());
  glUniformMatrix4fv(floor_modelview_projection_param_, 1, GL_FALSE,
                     MatrixToGLArray(modelview_projection_floor_).data());
  glVertexAttribPointer(floor_position_param_, kCoordsPerVertex, GL_FLOAT,
                        false, 0, floor_vertices_);
  glVertexAttribPointer(floor_normal_param_, 3, GL_FLOAT, false, 0,
                        floor_normals_);
  glVertexAttribPointer(
      floor_color_param_, 4, GL_FLOAT, false, 0, floor_colors_);

  glEnableVertexAttribArray(floor_position_param_);
  glEnableVertexAttribArray(floor_normal_param_);
  glEnableVertexAttribArray(floor_color_param_);

  glDrawArrays(GL_TRIANGLES, 0, 24);

  CheckGLError("Drawing floor");
}

void TreasureHuntRenderer::HideObject() {
  static const float kMaxModelDistance = 7.0f;
  static const float kMinModelDistance = 3.0f;

  std::array<float, 4> cube_position = {
      model_cube_.m[0][3], model_cube_.m[1][3], model_cube_.m[2][3], 1.f};

  // First rotate in XZ plane, between pi/2 and 3pi/2 radians away, apply this
  // to model_cube_ to keep the front face of the cube towards the user.
  float angle_xz = M_PI * (RandomUniformFloat() + 0.5f);
  gvr::Mat4f rotation_matrix = {{{cosf(angle_xz), 0.f, -sinf(angle_xz), 0.f},
                                 {0.f, 1.f, 0.f, 0.f},
                                 {sinf(angle_xz), 0.f, cosf(angle_xz), 0.f},
                                 {0.f, 0.f, 0.f, 1.f}}};
  cube_position = MatrixVectorMul(rotation_matrix, cube_position);
  model_cube_ = MatrixMul(rotation_matrix, model_cube_);

  // Pick a new distance for the cube, and apply that scale to the position.
  float old_object_distance = object_distance_;
  object_distance_ =
      RandomUniformFloat() * (kMaxModelDistance - kMinModelDistance) +
      kMinModelDistance;
  float scale = object_distance_ / old_object_distance;
  cube_position[0] *= scale;
  cube_position[1] *= scale;
  cube_position[2] *= scale;

  // Choose a random yaw for the cube between pi/4 and -pi/4.
  float yaw = M_PI * (RandomUniformFloat() - 0.5f) / 2;
  cube_position[1] = tanf(yaw) * object_distance_;

  model_cube_.m[0][3] = cube_position[0];
  model_cube_.m[1][3] = cube_position[1];
  model_cube_.m[2][3] = cube_position[2];

  if (sound_id_ >= 0) {
    gvr_audio_api_->SetSoundObjectPosition(sound_id_, cube_position[0],
                                           cube_position[1], cube_position[2]);
  }
}

bool TreasureHuntRenderer::IsLookingAtObject() {
  static const float kPitchLimit = 0.12f;
  static const float kYawLimit = 0.12f;

  gvr::Mat4f modelview =
      MatrixMul(head_pose_.object_from_reference_matrix, model_cube_);

  std::array<float, 4> temp_position =
      MatrixVectorMul(modelview, {0.f, 0.f, 0.f, 1.f});

  float pitch = std::atan2(temp_position[1], -temp_position[2]);
  float yaw = std::atan2(temp_position[0], -temp_position[2]);

  return std::abs(pitch) < kPitchLimit && std::abs(yaw) < kYawLimit;
}

void TreasureHuntRenderer::LoadAndPlayCubeSound() {
  // Preload sound file.
  gvr_audio_api_->PreloadSoundfile(kSoundFile);
  // Create sound file handler from preloaded sound file.
  sound_id_ = gvr_audio_api_->CreateSoundObject(kSoundFile);
  // Set sound object to current cube position.
  gvr_audio_api_->SetSoundObjectPosition(
      sound_id_, model_cube_.m[0][3], model_cube_.m[1][3], model_cube_.m[2][3]);
  // Trigger sound object playback.
  gvr_audio_api_->PlaySound(sound_id_, true /* looped playback */);
}
