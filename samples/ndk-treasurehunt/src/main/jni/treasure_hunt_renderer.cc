/* Copyright 2017 Google Inc. All rights reserved.
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

#include "treasure_hunt_renderer.h"  // NOLINT

#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <random>

#define LOG_TAG "TreasureHuntCPP"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define CHECK(condition)                                                   \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }

namespace {
static const float kZNear = 1.0f;
static const float kZFar = 100.0f;

// For now, to avoid writing a raycasting system, put the reticle closer than
// any objects.
static const float kMinCubeDistance = 4.5f;
static const float kMaxCubeDistance = 8.0f;
static const float kReticleDistance = 3.0f;

static const float kFloorDepth = 20.0f;

static const int kCoordsPerVertex = 3;

static const uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

// Angle threshold for determining whether the controller is pointing at the
// object.
static const float kAngleLimit = 0.12f;
// Angle threshold for determining whether the viewer is looking at the object.
static const float kPitchLimit = 0.12f;
static const float kYawLimit = 0.12f;

static const char* kGridFragmentShader = R"glsl(
    precision mediump float;
    varying vec4 v_Color;
    varying vec3 v_Grid;

    void main() {
      float depth = gl_FragCoord.z / gl_FragCoord.w;
      if ((mod(abs(v_Grid.x), 10.0) < 0.1) ||
          (mod(abs(v_Grid.z), 10.0) < 0.1)) {
        gl_FragColor = max(0.0, (90.0-depth) / 90.0) *
                       vec4(1.0, 1.0, 1.0, 1.0) +
                       min(1.0, depth / 90.0) * v_Color;
      } else {
        gl_FragColor = v_Color;
      }
    })glsl";

static const char* kLightVertexShader = R"glsl(
    uniform mat4 u_Model;
    uniform mat4 u_MVP;
    uniform mat4 u_MVMatrix;
    uniform vec3 u_LightPos;
    attribute vec4 a_Position;
    attribute vec4 a_Color;
    attribute vec3 a_Normal;
    varying vec4 v_Color;
    varying vec3 v_Grid;

    void main() {
      v_Grid = vec3(u_Model * a_Position);
      vec3 modelViewVertex = vec3(u_MVMatrix * a_Position);
      vec3 modelViewNormal = vec3(u_MVMatrix * vec4(a_Normal, 0.0));
      float distance = length(u_LightPos - modelViewVertex);
      vec3 lightVector = normalize(u_LightPos - modelViewVertex);
      float diffuse = max(dot(modelViewNormal, lightVector), 0.5);
      diffuse = diffuse * (1.0 / (1.0 + (0.00001 * distance * distance)));
      v_Color = vec4(a_Color.rgb * diffuse, a_Color.a);
      gl_Position = u_MVP * a_Position;
    })glsl";

static const char* kPassthroughFragmentShader = R"glsl(
    precision mediump float;
    varying vec4 v_Color;

    void main() {
      gl_FragColor = v_Color;
    })glsl";

static const char* kReticleVertexShader = R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    varying vec2 v_Coords;

    void main() {
      v_Coords = a_Position.xy;
      gl_Position = u_MVP * a_Position;
    })glsl";

static const char* kReticleFragmentShader = R"glsl(
    precision mediump float;

    varying vec2 v_Coords;

    void main() {
      float r = length(v_Coords);
      float alpha = smoothstep(0.5, 0.6, r) * (1.0 - smoothstep(0.8, 0.9, r));
      if (alpha == 0.0) discard;
      gl_FragColor = vec4(alpha);
    })glsl";

// Sound file in APK assets.
static const char* kObjectSoundFile = "cube_sound.wav";
static const char* kSuccessSoundFile = "success.wav";

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

static std::array<float, 4> MatrixVectorMul(const gvr::Mat4f& matrix,
                                            const std::array<float, 4>& vec) {
  std::array<float, 4> result;
  for (int i = 0; i < 4; ++i) {
    result[i] = 0;
    for (int k = 0; k < 4; ++k) {
      result[i] += matrix.m[i][k] * vec[k];
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
        result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
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
  const float width = static_cast<float>(texture_size.width);
  const float height = static_cast<float>(texture_size.height);
  const gvr::Rectf rect = ModulateRect(texture_rect, width, height);
  const gvr::Recti result = {
      static_cast<int>(rect.left), static_cast<int>(rect.right),
      static_cast<int>(rect.bottom), static_cast<int>(rect.top)};
  return result;
}

// Generate a random floating point number between 0 and 1.
static float RandomUniformFloat() {
  static std::random_device random_device;
  static std::mt19937 random_generator(random_device());
  static std::uniform_real_distribution<float> random_distribution(0, 1);
  return random_distribution(random_generator);
}

static void CheckGLError(const char* label) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    LOGW("GL error @ %s: %d", label, gl_error);
    // Crash immediately to make OpenGL errors obvious.
    abort();
  }
}

static gvr::Sizei HalfPixelCount(const gvr::Sizei& in) {
  // Scale each dimension by sqrt(2)/2 ~= 7/10ths.
  gvr::Sizei out;
  out.width = (7 * in.width) / 10;
  out.height = (7 * in.height) / 10;
  return out;
}

static gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  gvr::Mat4f result;
  const float x = quat.qx;
  const float x2 = quat.qx * quat.qx;
  const float y = quat.qy;
  const float y2 = quat.qy * quat.qy;
  const float z = quat.qz;
  const float z2 = quat.qz * quat.qz;
  const float w = quat.qw;
  const float xy = quat.qx * quat.qy;
  const float xz = quat.qx * quat.qz;
  const float xw = quat.qx * quat.qw;
  const float yz = quat.qy * quat.qz;
  const float yw = quat.qy * quat.qw;
  const float zw = quat.qz * quat.qw;

  const float m11 = 1.0f - 2.0f * y2 - 2.0f * z2;
  const float m12 = 2.0f * (xy - zw);
  const float m13 = 2.0f * (xz + yw);
  const float m21 = 2.0f * (xy + zw);
  const float m22 = 1.0f - 2.0f * x2 - 2.0f * z2;
  const float m23 = 2.0f * (yz - xw);
  const float m31 = 2.0f * (xz - yw);
  const float m32 = 2.0f * (yz + xw);
  const float m33 = 1.0f - 2.0f * x2 - 2.0f * y2;

  return {{{m11, m12, m13, 0.0f},
           {m21, m22, m23, 0.0f},
           {m31, m32, m33, 0.0f},
           {0.0f, 0.0f, 0.0f, 1.0f}}};
}

static inline float VectorNorm(const std::array<float, 4>& vect) {
  return std::sqrt(vect[0] * vect[0] + vect[1] * vect[1] + vect[2] * vect[2]);
}

static float VectorInnerProduct(const std::array<float, 4>& vect1,
                                const std::array<float, 4>& vect2) {
  float product = 0;
  for (int i = 0; i < 3; i++) {
    product += vect1[i] * vect2[i];
  }
  return product;
}
}  // anonymous namespace

TreasureHuntRenderer::TreasureHuntRenderer(
    gvr_context* gvr_context, std::unique_ptr<gvr::AudioApi> gvr_audio_api)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      gvr_audio_api_(std::move(gvr_audio_api)),
      scratch_viewport_(gvr_api_->CreateBufferViewport()),
      floor_vertices_(world_layout_data_.floor_coords.data()),
      cube_vertices_(world_layout_data_.cube_coords.data()),
      cube_colors_(world_layout_data_.cube_colors.data()),
      cube_found_colors_(world_layout_data_.cube_found_color.data()),
      cube_normals_(world_layout_data_.cube_normals.data()),
      reticle_vertices_(world_layout_data_.reticle_coords.data()),
      reticle_render_size_{128, 128},
      light_pos_world_space_({0.0f, 2.0f, 0.0f, 1.0f}),
      object_distance_(kMinCubeDistance),
      audio_source_id_(-1),
      success_source_id_(-1),
      gvr_controller_api_(nullptr),
      gvr_viewer_type_(gvr_api_->GetViewerType()) {
  ResumeControllerApiAsNeeded();
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) {
    LOGD("Viewer type: CARDBOARD");
  } else if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    LOGD("Viewer type: DAYDREAM");
  } else {
    LOGE("Unexpected viewer type.");
  }
}

TreasureHuntRenderer::~TreasureHuntRenderer() {
  // Join the audio initialization thread in case it still exists.
  if (audio_initialization_thread_.joinable()) {
    audio_initialization_thread_.join();
  }
}

void TreasureHuntRenderer::InitializeGl() {
  gvr_api_->InitializeGl();

  const int vertex_shader = LoadGLShader(GL_VERTEX_SHADER, &kLightVertexShader);
  const int grid_shader =
      LoadGLShader(GL_FRAGMENT_SHADER, &kGridFragmentShader);
  const int pass_through_shader =
      LoadGLShader(GL_FRAGMENT_SHADER, &kPassthroughFragmentShader);
  const int reticle_vertex_shader =
      LoadGLShader(GL_VERTEX_SHADER, &kReticleVertexShader);
  const int reticle_fragment_shader =
      LoadGLShader(GL_FRAGMENT_SHADER, &kReticleFragmentShader);

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

  reticle_program_ = glCreateProgram();
  glAttachShader(reticle_program_, reticle_vertex_shader);
  glAttachShader(reticle_program_, reticle_fragment_shader);
  glLinkProgram(reticle_program_);
  glUseProgram(reticle_program_);

  CheckGLError("Reticle program");

  reticle_position_param_ = glGetAttribLocation(floor_program_, "a_Position");
  reticle_modelview_projection_param_ =
      glGetUniformLocation(reticle_program_, "u_MVP");

  CheckGLError("Reticle program params");

  // Object first appears directly in front of user.
  model_cube_ = {{{1.0f, 0.0f, 0.0f, 0.0f},
                  {0.0f, 0.707f, -0.707f, 0.0f},
                  {0.0f, 0.707f, 0.707f, -object_distance_},
                  {0.0f, 0.0f, 0.0f, 1.0f}}};
  model_floor_ = {{{1.0f, 0.0f, 0.0f, 0.0f},
                   {0.0f, 1.0f, 0.0f, -kFloorDepth},
                   {0.0f, 0.0f, 1.0f, 0.0f},
                   {0.0f, 0.0f, 0.0f, 1.0f}}};
  const float rs = 0.04f;  // Reticle scale.
  model_reticle_ = {{{rs, 0.0f, 0.0f, 0.0f},
                     {0.0f, rs, 0.0f, 0.0f},
                     {0.0f, 0.0f, rs, -kReticleDistance},
                     {0.0f, 0.0f, 0.0f, 1.0f}}};

  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  render_size_ =
      HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  std::vector<gvr::BufferSpec> specs;

  specs.push_back(gvr_api_->CreateBufferSpec());
  specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
  specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
  specs[0].SetSize(render_size_);
  specs[0].SetSamples(2);

  specs.push_back(gvr_api_->CreateBufferSpec());
  specs[1].SetSize(reticle_render_size_);
  specs[1].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
  specs[1].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_NONE);
  specs[1].SetSamples(1);
  swapchain_.reset(new gvr::SwapChain(gvr_api_->CreateSwapChain(specs)));

  viewport_list_.reset(
      new gvr::BufferViewportList(gvr_api_->CreateEmptyBufferViewportList()));

  // Initialize audio engine and preload sample in a separate thread to avoid
  // any delay during construction and app initialization. Only do this once.
  if (!audio_initialization_thread_.joinable()) {
    audio_initialization_thread_ =
        std::thread(&TreasureHuntRenderer::LoadAndPlayCubeSound, this);
  }
}

void TreasureHuntRenderer::ResumeControllerApiAsNeeded() {
  switch (gvr_viewer_type_) {
    case GVR_VIEWER_TYPE_CARDBOARD:
      gvr_controller_api_.reset();
      break;
    case GVR_VIEWER_TYPE_DAYDREAM:
      if (!gvr_controller_api_) {
        // Initialized controller api.
        gvr_controller_api_.reset(new gvr::ControllerApi);
        CHECK(gvr_controller_api_);
        CHECK(gvr_controller_api_->Init(gvr::ControllerApi::DefaultOptions(),
                                        gvr_api_->cobj()));
      }
      gvr_controller_api_->Resume();
      break;
    default:
      LOGE("unexpected viewer type.");
      break;
  }
}

void TreasureHuntRenderer::ProcessControllerInput() {
  const int old_status = gvr_controller_state_.GetApiStatus();
  const int old_connection_state = gvr_controller_state_.GetConnectionState();

  // Read current controller state.
  gvr_controller_state_.Update(*gvr_controller_api_);

  // Print new API status and connection state, if they changed.
  if (gvr_controller_state_.GetApiStatus() != old_status ||
      gvr_controller_state_.GetConnectionState() != old_connection_state) {
    LOGD("TreasureHuntApp: controller API status: %s, connection state: %s",
         gvr_controller_api_status_to_string(
             gvr_controller_state_.GetApiStatus()),
         gvr_controller_connection_state_to_string(
             gvr_controller_state_.GetConnectionState()));
  }

  // Trigger click event if app/click button is clicked.
  if (gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_APP) ||
      gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_CLICK)) {
    OnTriggerEvent();
  }
}

void TreasureHuntRenderer::DrawFrame() {
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    ProcessControllerInput();
  }
  PrepareFramebuffer();
  gvr::Frame frame = swapchain_->AcquireFrame();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;

  head_view_ = gvr_api_->GetHeadSpaceFromStartSpaceRotation(target_time);
  gvr::Mat4f left_eye_matrix = gvr_api_->GetEyeFromHeadMatrix(GVR_LEFT_EYE);
  gvr::Mat4f right_eye_matrix = gvr_api_->GetEyeFromHeadMatrix(GVR_RIGHT_EYE);
  gvr::Mat4f left_eye_view = MatrixMul(left_eye_matrix, head_view_);
  gvr::Mat4f right_eye_view = MatrixMul(right_eye_matrix, head_view_);

  viewport_list_->SetToRecommendedBufferViewports();
  gvr::BufferViewport reticle_viewport = gvr_api_->CreateBufferViewport();
  reticle_viewport.SetSourceBufferIndex(1);
  reticle_viewport.SetReprojection(GVR_REPROJECTION_NONE);
  reticle_viewport.SetSourceUv({0.f, 1.f, 0.f, 1.f});

  // Use the viewport transform to put the reticle in the correct place.
  reticle_viewport.SetTransform(MatrixMul(left_eye_matrix, model_reticle_));
  reticle_viewport.SetTargetEye(GVR_LEFT_EYE);
  viewport_list_->SetBufferViewport(2, reticle_viewport);
  reticle_viewport.SetTransform(MatrixMul(right_eye_matrix, model_reticle_));
  reticle_viewport.SetTargetEye(GVR_RIGHT_EYE);
  viewport_list_->SetBufferViewport(3, reticle_viewport);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);

  // Draw the world.
  frame.BindBuffer(0);
  glClearColor(0.1f, 0.1f, 0.1f, 0.5f);  // Dark background so text shows up.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  viewport_list_->GetBufferViewport(0, &scratch_viewport_);
  DrawWorld(left_eye_view, scratch_viewport_);
  viewport_list_->GetBufferViewport(1, &scratch_viewport_);
  DrawWorld(right_eye_view, scratch_viewport_);
  frame.Unbind();

  frame.BindBuffer(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent background.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // In Cardboard viewer, draw head-locked reticle on a separate layer since the
  // cursor is controlled by head movement. In Daydream viewer, this layer is
  // left empty, since the cursor is controlled by controller and drawn with
  // DrawCursor() in the same frame buffer as the virtual scene.
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) {
    DrawReticle();
  }
  frame.Unbind();

  // Submit frame.
  frame.Submit(*viewport_list_, head_view_);

  CheckGLError("onDrawFrame");

  // Update audio head rotation in audio API.
  gvr_audio_api_->SetHeadPose(head_view_);
  gvr_audio_api_->Update();
}

void TreasureHuntRenderer::PrepareFramebuffer() {
  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  const gvr::Sizei recommended_size =
      HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  if (render_size_.width != recommended_size.width ||
      render_size_.height != recommended_size.height) {
    // We need to resize the framebuffer.
    swapchain_->ResizeBuffer(0, recommended_size);
    render_size_ = recommended_size;
  }
}

void TreasureHuntRenderer::OnTriggerEvent() {
  if (ObjectIsFound()) {
    success_source_id_ = gvr_audio_api_->CreateStereoSound(kSuccessSoundFile);
    gvr_audio_api_->PlaySound(success_source_id_, false /* looping disabled */);
    HideObject();
  }
}

void TreasureHuntRenderer::OnPause() {
  gvr_api_->PauseTracking();
  gvr_audio_api_->Pause();
  if (gvr_controller_api_) gvr_controller_api_->Pause();
}

void TreasureHuntRenderer::OnResume() {
  gvr_api_->ResumeTracking();
  gvr_audio_api_->Resume();
  gvr_viewer_type_ = gvr_api_->GetViewerType();
  ResumeControllerApiAsNeeded();
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
  int compileStatus;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

  // If the compilation failed, delete the shader.
  if (compileStatus == 0) {
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
void TreasureHuntRenderer::DrawWorld(const gvr::Mat4f& view_matrix,
                                     const gvr::BufferViewport& viewport) {
  const gvr::Recti pixel_rect =
      CalculatePixelSpaceRect(render_size_, viewport.GetSourceUv());

  glViewport(pixel_rect.left, pixel_rect.bottom,
             pixel_rect.right - pixel_rect.left,
             pixel_rect.top - pixel_rect.bottom);

  CheckGLError("World drawing setup");

  // Set the position of the light
  light_pos_eye_space_ = MatrixVectorMul(view_matrix, light_pos_world_space_);
  const gvr::Mat4f perspective =
      PerspectiveMatrixFromView(viewport.GetSourceFov(), kZNear, kZFar);
  modelview_ = MatrixMul(view_matrix, model_cube_);
  modelview_projection_cube_ = MatrixMul(perspective, modelview_);
  DrawCube();

  // Set modelview_ for the floor, so we draw floor in the correct location
  modelview_ = MatrixMul(view_matrix, model_floor_);
  modelview_projection_floor_ = MatrixMul(perspective, modelview_);
  DrawFloor();

  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    gvr::Mat4f controller_matrix =
        ControllerQuatToMatrix(gvr_controller_state_.GetOrientation());
    model_cursor_ = MatrixMul(controller_matrix, model_reticle_);
    modelview_ = MatrixMul(view_matrix, model_cursor_);
    modelview_projection_cursor_ = MatrixMul(perspective, modelview_);
    DrawCursor();
  }
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
  // Set the ModelViewProjection matrix in the shader.
  glUniformMatrix4fv(cube_modelview_projection_param_, 1, GL_FALSE,
                     MatrixToGLArray(modelview_projection_cube_).data());

  // Set the position of the cube
  glVertexAttribPointer(cube_position_param_, kCoordsPerVertex, GL_FLOAT, false,
                        0, cube_vertices_);
  glEnableVertexAttribArray(cube_position_param_);

  // Set the normal positions of the cube, again for shading
  glVertexAttribPointer(cube_normal_param_, 3, GL_FLOAT, false, 0,
                        cube_normals_);
  glEnableVertexAttribArray(cube_normal_param_);

  // Set vertex colors
  if (ObjectIsFound()) {
    const float* found_color = world_layout_data_.cube_found_color.data();
    glVertexAttrib4f(cube_color_param_, found_color[0], found_color[1],
                     found_color[2], 1.0f);
    glDisableVertexAttribArray(cube_color_param_);
  } else {
    glVertexAttribPointer(cube_color_param_, 3, GL_FLOAT, false, 0,
                          cube_colors_);
    glEnableVertexAttribArray(cube_color_param_);
  }

  glDrawArrays(GL_TRIANGLES, 0, 36);

  glDisableVertexAttribArray(cube_position_param_);
  glDisableVertexAttribArray(cube_normal_param_);
  glDisableVertexAttribArray(cube_color_param_);

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
  glVertexAttrib3f(floor_normal_param_, 0.0f, 1.0f, 0.0f);
  glVertexAttrib4f(floor_color_param_, 0.0f, 0.3398f, 0.9023f, 1.0f);

  glEnableVertexAttribArray(floor_position_param_);
  glDrawArrays(GL_TRIANGLES, 0, 24);
  glDisableVertexAttribArray(floor_position_param_);

  CheckGLError("Drawing floor");
}

void TreasureHuntRenderer::DrawCursor() {
  glUseProgram(reticle_program_);
  glUniformMatrix4fv(reticle_modelview_projection_param_, 1, GL_FALSE,
                     MatrixToGLArray(modelview_projection_cursor_).data());
  glVertexAttribPointer(reticle_position_param_, kCoordsPerVertex, GL_FLOAT,
                        false, 0, reticle_vertices_);
  glEnableVertexAttribArray(reticle_position_param_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(reticle_position_param_);
  CheckGLError("Drawing cursor");
}

void TreasureHuntRenderer::DrawReticle() {
  glViewport(0, 0, reticle_render_size_.width, reticle_render_size_.height);
  glUseProgram(reticle_program_);
  const gvr::Mat4f uniform_matrix = {{{1.f, 0.f, 0.f, 0.f},
                                      {0.f, 1.f, 0.f, 0.f},
                                      {0.f, 0.f, 1.f, 0.f},
                                      {0.f, 0.f, 0.f, 1.f}}};
  glUniformMatrix4fv(reticle_modelview_projection_param_, 1, GL_FALSE,
                     MatrixToGLArray(uniform_matrix).data());
  glVertexAttribPointer(reticle_position_param_, kCoordsPerVertex, GL_FLOAT,
                        false, 0, reticle_vertices_);
  glEnableVertexAttribArray(reticle_position_param_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(reticle_position_param_);

  CheckGLError("Drawing reticle");
}

void TreasureHuntRenderer::HideObject() {
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
  const float old_object_distance = object_distance_;
  object_distance_ =
      RandomUniformFloat() * (kMaxCubeDistance - kMinCubeDistance) +
      kMinCubeDistance;
  const float scale = object_distance_ / old_object_distance;
  cube_position[0] *= scale;
  cube_position[1] *= scale;
  cube_position[2] *= scale;

  // Choose a random yaw for the cube between pi/4 and -pi/4.
  const float yaw = M_PI * (RandomUniformFloat() - 0.5f) / 2.0f;
  cube_position[1] = tanf(yaw) * object_distance_;

  model_cube_.m[0][3] = cube_position[0];
  model_cube_.m[1][3] = cube_position[1];
  model_cube_.m[2][3] = cube_position[2];

  if (audio_source_id_ >= 0) {
    gvr_audio_api_->SetSoundObjectPosition(audio_source_id_, cube_position[0],
                                           cube_position[1], cube_position[2]);
  }
}

bool TreasureHuntRenderer::ObjectIsFound() {
  switch (gvr_viewer_type_) {
    case GVR_VIEWER_TYPE_CARDBOARD: {
      return IsLookingAtObject();
      break;
    }
    case GVR_VIEWER_TYPE_DAYDREAM: {
      return IsPointingAtObject();
      break;
    }
    default:
      LOGW("Unexpected viewer type.");
      return false;
      break;
  }
}

bool TreasureHuntRenderer::IsLookingAtObject() {
  const gvr::Mat4f modelview = MatrixMul(head_view_, model_cube_);

  const std::array<float, 4> temp_position =
      MatrixVectorMul(modelview, {0.f, 0.f, 0.f, 1.f});

  float pitch = std::atan2(temp_position[1], -temp_position[2]);
  float yaw = std::atan2(temp_position[0], -temp_position[2]);

  return std::abs(pitch) < kPitchLimit && std::abs(yaw) < kYawLimit;
}

bool TreasureHuntRenderer::IsPointingAtObject() {
  gvr::Mat4f modelview_cursor = MatrixMul(head_view_, model_cursor_);
  gvr::Mat4f modelview_cube = MatrixMul(head_view_, model_cube_);

  const std::array<float, 4> center_cursor_position =
      MatrixVectorMul(modelview_cursor, {0.f, 0.f, 0.f, 1.f});

  const std::array<float, 4> center_cube_position =
      MatrixVectorMul(modelview_cube, {0.f, 0.f, 0.f, 1.f});

  float angle = std::acos(
      VectorInnerProduct(center_cursor_position, center_cube_position) /
      VectorNorm(center_cursor_position) / VectorNorm(center_cube_position));
  return angle < kAngleLimit;
}

void TreasureHuntRenderer::LoadAndPlayCubeSound() {
  // Preload sound files.
  gvr_audio_api_->PreloadSoundfile(kObjectSoundFile);
  gvr_audio_api_->PreloadSoundfile(kSuccessSoundFile);
  // Create sound file handler from preloaded sound file.
  audio_source_id_ = gvr_audio_api_->CreateSoundObject(kObjectSoundFile);
  // Set sound object to current cube position.
  gvr_audio_api_->SetSoundObjectPosition(audio_source_id_, model_cube_.m[0][3],
                                         model_cube_.m[1][3],
                                         model_cube_.m[2][3]);
  // Trigger sound object playback.
  gvr_audio_api_->PlaySound(audio_source_id_, true /* looped playback */);
}
