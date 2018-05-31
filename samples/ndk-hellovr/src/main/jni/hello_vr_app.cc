/* Copyright 2018 Google Inc. All rights reserved.
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

#include "hello_vr_app.h"  // NOLINT

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <random>

#include "hello_vr_shaders.h"  // NOLINT
#include "util.h"              // NOLINT
#include "vr/gvr/capi/include/gvr_version.h"

namespace ndk_hello_vr {

// TODO(b/72458816): move most of this stuff to util.cc
namespace {
static constexpr float kZNear = 0.01f;
static constexpr float kZFar = 10.0f;

// The objects are about 1 meter in radius, so the min/max target distance are
// set so that the objects are always within the room (which is about 5 meters
// across) and the reticle is always closer than any objects.
static constexpr float kMinTargetDistance = 3.0f;
static constexpr float kMaxTargetDistance = 3.5f;
static constexpr float kReticleDistance = 1.9f;

// Depth of the ground plane, in meters. If this (and other distances)
// are too far, 6DOF tracking will have no visible effect.
static constexpr float kDefaultFloorHeight = -2.0f;

static constexpr int kCoordsPerVertex = 3;

static constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

// Angle threshold for determining whether the controller is pointing at the
// object.
static constexpr float kAngleLimit = 0.2f;

// Sound file in APK assets.
// TODO(b/72458816): make sure these sound files play at the right time
static constexpr const char* kObjectSoundFile = "audio/HelloVR_Loop.wav";
static constexpr const char* kSuccessSoundFile = "audio/HelloVR_Activation.wav";

static constexpr int kTargetMeshCount = 3;

// Convert a GVR matrix to an array of floats suitable for passing to OpenGL.
static std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix) {
  // Note that this performs a *transpose* to a column-major matrix array, as
  // expected by GL.
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

// Flatten a pair of mat4's into an array of 32 floats, useful when feeding
// uniform values to OpenGL for multiview.
static std::array<float, 32> MatrixPairToGLArray(const gvr::Mat4f matrix[]) {
  std::array<float, 32> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix[0].m[i][j];
      result[16 + j * 4 + i] = matrix[1].m[i][j];
    }
  }
  return result;
}

// Flatten a pair of vec3's into an array of 6 floats, useful when feeding
// uniform values to OpenGL for multiview.
static std::array<float, 6> VectorPairToGLArray(
    const std::array<float, 3> vec[]) {
  std::array<float, 6> result;
  for (int k = 0; k < 3; ++k) {
    result[k] = vec[0][k];
    result[k + 3] = vec[1][k];
  }
  return result;
}

// Multiply a vector by a matrix.
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

// Multiply two matrices.
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

// Drop the last element of a vector.
static std::array<float, 3> Vec4ToVec3(const std::array<float, 4>& vec) {
  return {vec[0], vec[1], vec[2]};
}

// Given a field of view in degrees, compute the corresponding projection
// matrix.
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

// Multiplies both X coordinates of the rectangle by the given width and both Y
// coordinates by the given height.
static gvr::Rectf ModulateRect(const gvr::Rectf& rect, float width,
                               float height) {
  gvr::Rectf result = {rect.left * width, rect.right * width,
                       rect.bottom * height, rect.top * height};
  return result;
}

// Given the size of a texture in pixels and a rectangle in UV coordinates,
// computes the corresponding rectangle in pixel coordinates.
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

static int RandomUniformInt(int max_val) {
  static std::random_device random_device;
  static std::mt19937 random_generator(random_device());
  std::uniform_int_distribution<int> random_distribution(0, max_val - 1);
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

// Computes a texture size that has approximately half as many pixels. This is
// equivalent to scaling each dimension by approximately sqrt(2)/2.
static gvr::Sizei HalfPixelCount(const gvr::Sizei& in) {
  // Scale each dimension by sqrt(2)/2 ~= 7/10ths.
  gvr::Sizei out;
  out.width = (7 * in.width) / 10;
  out.height = (7 * in.height) / 10;
  return out;
}

// Convert the quaternion describing the controller's orientation to a
// rotation matrix.
static gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  const float x2 = quat.qx * quat.qx;
  const float y2 = quat.qy * quat.qy;
  const float z2 = quat.qz * quat.qz;
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

// Simple shaders to render .obj files without any lighting.
// TODO(b/72458816): consider moving these shaders to util.cc and having the Obj
// class load them.
constexpr const char* kObjVertexShaders[] = {
    R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;

    void main() {
      v_UV = a_UV;
      gl_Position = u_MVP * a_Position;
    })glsl",
    // The following shader is for multiview rendering.
    R"glsl(#version 300 es
    #extension GL_OVR_multiview2 : enable

    layout(num_views=2) in;

    uniform mat4 u_MVP[2];
    in vec4 a_Position;
    in vec2 a_UV;
    out vec2 v_UV;

    void main() {
      mat4 mvp = u_MVP[gl_ViewID_OVR];
      v_UV = a_UV;
      gl_Position = mvp * a_Position;
    })glsl"};

constexpr const char* kObjFragmentShaders[] = {
    R"glsl(
    precision mediump float;
    varying vec2 v_UV;
    uniform sampler2D u_Texture;

    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      gl_FragColor = texture2D(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
    })glsl",
    // The following shader is for multiview rendering.
    R"glsl(#version 300 es

    precision mediump float;
    in vec2 v_UV;
    out vec4 FragColor;
    uniform sampler2D u_Texture;

    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      FragColor = texture(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
    })glsl"};

}  // anonymous namespace

HelloVrApp::HelloVrApp(JNIEnv* env, jobject asset_mgr_obj,
                       gvr_context* gvr_context,
                       std::unique_ptr<gvr::AudioApi> gvr_audio_api)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      gvr_audio_api_(std::move(gvr_audio_api)),
      viewport_left_(gvr_api_->CreateBufferViewport()),
      viewport_right_(gvr_api_->CreateBufferViewport()),
      reticle_vertices_(world_layout_data_.reticle_coords.data()),
      target_object_meshes_(kTargetMeshCount),
      target_object_not_selected_textures_(kTargetMeshCount),
      target_object_selected_textures_(kTargetMeshCount),
      cur_target_object_(RandomUniformInt(kTargetMeshCount)),
      reticle_program_(0),
      obj_program_(0),
      obj_position_param_(0),
      obj_uv_param_(0),
      obj_modelview_projection_param_(0),
      reticle_position_param_(0),
      reticle_modelview_projection_param_(0),
      reticle_render_size_{128, 128},
      light_pos_world_space_({0.0f, 2.0f, 0.0f, 1.0f}),
      audio_source_id_(-1),
      success_source_id_(-1),
      gvr_controller_api_(nullptr),
      gvr_viewer_type_(gvr_api_->GetViewerType()),
      java_asset_mgr_(env->NewGlobalRef(asset_mgr_obj)),
      asset_mgr_(AAssetManager_fromJava(env, asset_mgr_obj)) {
  ResumeControllerApiAsNeeded();

  LOGD("Built with GVR version: %s", GVR_SDK_VERSION_STRING);
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) {
    LOGD("Viewer type: CARDBOARD");
  } else if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    LOGD("Viewer type: DAYDREAM");
  } else {
    LOGE("Unexpected viewer type.");
  }
}

HelloVrApp::~HelloVrApp() {
  // Join the audio initialization thread in case it still exists.
  if (audio_initialization_thread_.joinable()) {
    audio_initialization_thread_.join();
  }
}

void HelloVrApp::OnSurfaceCreated(JNIEnv* env) {
  gvr_api_->InitializeGl();
  multiview_enabled_ = gvr_api_->IsFeatureSupported(GVR_FEATURE_MULTIVIEW);
  LOGD(multiview_enabled_ ? "Using multiview." : "Not using multiview.");

  int index = multiview_enabled_ ? 1 : 0;
  const int reticle_vertex_shader =
      LoadGLShader(GL_VERTEX_SHADER, kReticleVertexShaders[index]);
  const int reticle_fragment_shader =
      LoadGLShader(GL_FRAGMENT_SHADER, kReticleFragmentShaders[index]);
  const int obj_vertex_shader =
      LoadGLShader(GL_VERTEX_SHADER, kObjVertexShaders[index]);
  const int obj_fragment_shader =
      LoadGLShader(GL_FRAGMENT_SHADER, kObjFragmentShaders[index]);

  obj_program_ = glCreateProgram();
  glAttachShader(obj_program_, obj_vertex_shader);
  glAttachShader(obj_program_, obj_fragment_shader);
  glLinkProgram(obj_program_);
  glUseProgram(obj_program_);

  CheckGLError("Obj program");

  obj_position_param_ = glGetAttribLocation(obj_program_, "a_Position");
  obj_uv_param_ = glGetAttribLocation(obj_program_, "a_UV");
  obj_modelview_projection_param_ = glGetUniformLocation(obj_program_, "u_MVP");

  CheckGLError("Obj program params");

  // TODO(b/72458816): make sure the scale of these objects is correct. The .obj
  // files claim to use centimeters, but CubeRoom.obj clearly uses meters and
  // the others might as well.
  HELLOVR_CHECK(room_.Initialize(env, asset_mgr_, "CubeRoom.obj",
                                 obj_position_param_, obj_uv_param_));
  HELLOVR_CHECK(
      room_tex_.Initialize(env, java_asset_mgr_, "CubeRoom_BakedDiffuse.png"));
  HELLOVR_CHECK(target_object_meshes_[0].Initialize(
      env, asset_mgr_, "Icosahedron.obj", obj_position_param_, obj_uv_param_));
  HELLOVR_CHECK(target_object_not_selected_textures_[0].Initialize(
      env, java_asset_mgr_, "Icosahedron_Blue_BakedDiffuse.png"));
  HELLOVR_CHECK(target_object_selected_textures_[0].Initialize(
      env, java_asset_mgr_, "Icosahedron_Pink_BakedDiffuse.png"));
  HELLOVR_CHECK(target_object_meshes_[1].Initialize(
      env, asset_mgr_, "QuadSphere.obj", obj_position_param_, obj_uv_param_));
  HELLOVR_CHECK(target_object_not_selected_textures_[1].Initialize(
      env, java_asset_mgr_, "QuadSphere_Blue_BakedDiffuse.png"));
  HELLOVR_CHECK(target_object_selected_textures_[1].Initialize(
      env, java_asset_mgr_, "QuadSphere_Pink_BakedDiffuse.png"));
  HELLOVR_CHECK(target_object_meshes_[2].Initialize(
      env, asset_mgr_, "TriSphere.obj", obj_position_param_, obj_uv_param_));
  HELLOVR_CHECK(target_object_not_selected_textures_[2].Initialize(
      env, java_asset_mgr_, "TriSphere_Blue_BakedDiffuse.png"));
  HELLOVR_CHECK(target_object_selected_textures_[2].Initialize(
      env, java_asset_mgr_, "TriSphere_Pink_BakedDiffuse.png"));

  reticle_program_ = glCreateProgram();
  glAttachShader(reticle_program_, reticle_vertex_shader);
  glAttachShader(reticle_program_, reticle_fragment_shader);
  glLinkProgram(reticle_program_);
  glUseProgram(reticle_program_);

  CheckGLError("Reticle program");

  reticle_position_param_ = glGetAttribLocation(reticle_program_, "a_Position");
  reticle_modelview_projection_param_ =
      glGetUniformLocation(reticle_program_, "u_MVP");

  CheckGLError("Reticle program params");

  // Target object first appears directly in front of user.
  model_target_ = {
      {{1.0f, 0.0f, 0.0f, 0.0f},
       {0.0f, cosf(M_PI / 4.0f), -sinf(M_PI / 4.0f), 0.0f},
       {0.0f, sinf(M_PI / 4.0f), cosf(M_PI / 4.0f), -kMinTargetDistance},
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
  specs[0].SetSamples(2);

  // With multiview, the distortion buffer is a texture array with two layers
  // whose width is half the display width.
  if (multiview_enabled_) {
    gvr::Sizei half_size = { render_size_.width / 2, render_size_.height };
    specs[0].SetMultiviewLayers(2);
    specs[0].SetSize(half_size);
  } else {
    specs[0].SetSize(render_size_);
  }

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
        std::thread(&HelloVrApp::LoadAndPlayTargetObjectSound, this);
  }
}

void HelloVrApp::ResumeControllerApiAsNeeded() {
  switch (gvr_viewer_type_) {
    case GVR_VIEWER_TYPE_CARDBOARD:
      gvr_controller_api_.reset();
      break;
    case GVR_VIEWER_TYPE_DAYDREAM:
      if (!gvr_controller_api_) {
        // Initialized controller api.
        gvr_controller_api_.reset(new gvr::ControllerApi);
        HELLOVR_CHECK(gvr_controller_api_);
        HELLOVR_CHECK(gvr_controller_api_->Init(
            gvr::ControllerApi::DefaultOptions(), gvr_api_->cobj()));
      }
      gvr_controller_api_->Resume();
      break;
    default:
      LOGE("unexpected viewer type.");
      break;
  }
}

void HelloVrApp::ProcessControllerInput() {
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) return;
  const int old_status = gvr_controller_state_.GetApiStatus();
  const int old_connection_state = gvr_controller_state_.GetConnectionState();

  // Read current controller state.
  gvr_controller_state_.Update(*gvr_controller_api_);

  // Print new API status and connection state, if they changed.
  if (gvr_controller_state_.GetApiStatus() != old_status ||
      gvr_controller_state_.GetConnectionState() != old_connection_state) {
    LOGD("HelloVrApp: controller API status: %s, connection state: %s",
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

void HelloVrApp::UpdateReticlePosition() {
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    ProcessControllerInput();
    gvr::Mat4f controller_matrix =
        ControllerQuatToMatrix(gvr_controller_state_.GetOrientation());
    modelview_reticle_ =
        MatrixMul(head_view_, MatrixMul(controller_matrix, model_reticle_));
  } else {
    modelview_reticle_ = model_reticle_;
  }
}

void HelloVrApp::OnDrawFrame() {
  PrepareFramebuffer();
  gvr::Frame frame = swapchain_->AcquireFrame();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;
  gvr::BufferViewport* viewport[2] = {
    &viewport_left_,
    &viewport_right_,
  };
  head_view_ = gvr_api_->GetHeadSpaceFromStartSpaceTransform(target_time);
  viewport_list_->SetToRecommendedBufferViewports();

  gvr::BufferViewport reticle_viewport = gvr_api_->CreateBufferViewport();
  reticle_viewport.SetSourceBufferIndex(1);
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) {
    // Do not reproject the reticle if it's head-locked.
    reticle_viewport.SetReprojection(GVR_REPROJECTION_NONE);
  }
  const gvr_rectf fullscreen = { 0, 1, 0, 1 };
  reticle_viewport.SetSourceUv(fullscreen);
  UpdateReticlePosition();

  gvr::Value floor_height;
  // This may change when the floor height changes so it's computed every frame.
  float ground_y = gvr_api_->GetCurrentProperties().Get(
                       GVR_PROPERTY_TRACKING_FLOOR_HEIGHT, &floor_height)
                       ? floor_height.f
                       : kDefaultFloorHeight;
  gvr::Mat4f model_room = {{{1.0f, 0.0f, 0.0f, 0.0f},
                            {0.0f, 1.0f, 0.0f, ground_y},
                            {0.0f, 0.0f, 1.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f, 1.0f}}};
  gvr::Mat4f modelview_room[2];

  gvr::Mat4f eye_views[2];
  for (int eye = 0; eye < 2; ++eye) {
    const gvr::Eye gvr_eye = eye == 0 ? GVR_LEFT_EYE : GVR_RIGHT_EYE;
    const gvr::Mat4f eye_from_head = gvr_api_->GetEyeFromHeadMatrix(gvr_eye);
    eye_views[eye] = MatrixMul(eye_from_head, head_view_);

    viewport_list_->GetBufferViewport(eye, viewport[eye]);

    if (multiview_enabled_) {
      viewport[eye]->SetSourceUv(fullscreen);
      viewport[eye]->SetSourceLayer(eye);
      viewport_list_->SetBufferViewport(eye, *viewport[eye]);
    }

    reticle_viewport.SetTransform(MatrixMul(eye_from_head, modelview_reticle_));
    reticle_viewport.SetTargetEye(gvr_eye);
    // The first two viewports are for the 3D scene (one for each eye), the
    // latter two viewports are for the reticle (one for each eye).
    viewport_list_->SetBufferViewport(2 + eye, reticle_viewport);

    modelview_target_[eye] = MatrixMul(eye_views[eye], model_target_);
    modelview_room[eye] = MatrixMul(eye_views[eye], model_room);
    const gvr_rectf fov = viewport[eye]->GetSourceFov();

    const gvr::Mat4f perspective =
        PerspectiveMatrixFromView(fov, kZNear, kZFar);
    modelview_projection_target_[eye] =
        MatrixMul(perspective, modelview_target_[eye]);
    modelview_projection_room_[eye] =
        MatrixMul(perspective, modelview_room[eye]);
    light_pos_eye_space_[eye] =
        Vec4ToVec3(MatrixVectorMul(eye_views[eye], light_pos_world_space_));
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);

  // Draw the world.
  frame.BindBuffer(0);
  // The clear color doesn't matter here because it's completely obscured by
  // the room. However, the color buffer is still cleared because it may
  // improve performance.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (multiview_enabled_) {
    DrawWorld(kMultiview);
  } else {
    DrawWorld(kLeftView);
    DrawWorld(kRightView);
  }
  frame.Unbind();

  // Draw the reticle on a separate layer.
  frame.BindBuffer(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent background.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  DrawReticle();
  frame.Unbind();

  // Submit frame.
  frame.Submit(*viewport_list_, head_view_);

  CheckGLError("onDrawFrame");

  // Update audio head rotation in audio API.
  gvr_audio_api_->SetHeadPose(head_view_);
  gvr_audio_api_->Update();
}

void HelloVrApp::PrepareFramebuffer() {
  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  const gvr::Sizei recommended_size =
      HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  if (render_size_.width != recommended_size.width ||
      render_size_.height != recommended_size.height) {
    // We need to resize the framebuffer. Note that multiview uses two texture
    // layers, each with half the render width.
    gvr::Sizei framebuffer_size = recommended_size;
    if (multiview_enabled_) {
      framebuffer_size.width /= 2;
    }
    swapchain_->ResizeBuffer(0, framebuffer_size);
    render_size_ = recommended_size;
  }
}

void HelloVrApp::OnTriggerEvent() {
  if (IsPointingAtTarget()) {
    success_source_id_ = gvr_audio_api_->CreateStereoSound(kSuccessSoundFile);
    gvr_audio_api_->PlaySound(success_source_id_, false /* looping disabled */);
    HideTarget();
  }
}

void HelloVrApp::OnPause() {
  gvr_api_->PauseTracking();
  gvr_audio_api_->Pause();
  if (gvr_controller_api_) gvr_controller_api_->Pause();
}

void HelloVrApp::OnResume() {
  gvr_api_->ResumeTracking();
  gvr_api_->RefreshViewerProfile();
  gvr_audio_api_->Resume();
  gvr_viewer_type_ = gvr_api_->GetViewerType();
  ResumeControllerApiAsNeeded();
}

/**
 * Draws a frame for a particular view.
 *
 * @param view The view to render: left, right, or both (multiview).
 */
void HelloVrApp::DrawWorld(ViewType view) {
  if (view == kMultiview) {
    glViewport(0, 0, render_size_.width / 2, render_size_.height);
  } else {
    const gvr::BufferViewport& viewport =
        view == kLeftView ? viewport_left_ : viewport_right_;
    const gvr::Recti pixel_rect =
        CalculatePixelSpaceRect(render_size_, viewport.GetSourceUv());
    glViewport(pixel_rect.left, pixel_rect.bottom,
               pixel_rect.right - pixel_rect.left,
               pixel_rect.top - pixel_rect.bottom);
  }
  DrawTarget(view);
  DrawRoom(view);
}

void HelloVrApp::DrawTarget(ViewType view) {
  glUseProgram(obj_program_);

  if (view == kMultiview) {
    glUniformMatrix4fv(
        obj_modelview_projection_param_, 2, GL_FALSE,
        MatrixPairToGLArray(modelview_projection_target_).data());
  } else {
    glUniformMatrix4fv(
        obj_modelview_projection_param_, 1, GL_FALSE,
        MatrixToGLArray(modelview_projection_target_[view]).data());
  }

  if (IsPointingAtTarget()) {
    target_object_selected_textures_[cur_target_object_].Bind();
  } else {
    target_object_not_selected_textures_[cur_target_object_].Bind();
  }
  target_object_meshes_[cur_target_object_].Draw();

  CheckGLError("Drawing target object");
}

void HelloVrApp::DrawRoom(ViewType view) {
  glUseProgram(obj_program_);

  if (view == kMultiview) {
    glUniformMatrix4fv(obj_modelview_projection_param_, 2, GL_FALSE,
                       MatrixPairToGLArray(modelview_projection_room_).data());
  } else {
    glUniformMatrix4fv(
        obj_modelview_projection_param_, 1, GL_FALSE,
        MatrixToGLArray(modelview_projection_room_[view]).data());
  }

  room_tex_.Bind();
  room_.Draw();

  CheckGLError("Drawing room");
}

void HelloVrApp::DrawReticle() {
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

void HelloVrApp::HideTarget() {
  cur_target_object_ = RandomUniformInt(kTargetMeshCount);
  const float yaw = (RandomUniformFloat() - 0.5) * 200.0f * M_PI / 180.0f;
  const float pitch = (RandomUniformFloat() - 0.5) * 50.0f * M_PI / 180.0f;
  const float target_object_distance =
      RandomUniformFloat() * (kMaxTargetDistance - kMinTargetDistance) +
      kMinTargetDistance;

  model_target_ = {
      {{1.0f, 0.0f, 0.0f, 0.0f},
       {0.0f, cosf(M_PI / 4.0f), -sinf(M_PI / 4.0f), 0.0f},
       {0.0f, sinf(M_PI / 4.0f), cosf(M_PI / 4.0f), -target_object_distance},
       {0.0f, 0.0f, 0.0f, 1.0f}}};

  std::array<float, 4> target_position = {
      model_target_.m[0][3], model_target_.m[1][3], model_target_.m[2][3], 1.f};

  const gvr::Mat4f rotation_matrix = {{{cosf(yaw), 0.f, -sinf(yaw), 0.f},
                                       {0.f, 1.f, 0.f, 0.f},
                                       {sinf(yaw), 0.f, cosf(yaw), 0.f},
                                       {0.f, 0.f, 0.f, 1.f}}};
  target_position = MatrixVectorMul(rotation_matrix, target_position);
  model_target_ = MatrixMul(rotation_matrix, model_target_);

  target_position[1] = tanf(pitch) * target_object_distance;

  model_target_.m[0][3] = target_position[0];
  model_target_.m[1][3] = target_position[1];
  model_target_.m[2][3] = target_position[2];

  if (audio_source_id_ >= 0) {
    gvr_audio_api_->SetSoundObjectPosition(audio_source_id_, target_position[0],
                                           target_position[1],
                                           target_position[2]);
  }
}

bool HelloVrApp::IsPointingAtTarget() {
  // Compute vectors pointing towards the reticle and towards the target object
  // in head space.
  gvr::Mat4f head_from_reticle = modelview_reticle_;
  gvr::Mat4f head_from_target = MatrixMul(head_view_, model_target_);
  const std::array<float, 4> reticle_vector =
      MatrixVectorMul(head_from_reticle, {0.f, 0.f, 0.f, 1.f});
  const std::array<float, 4> target_vector =
      MatrixVectorMul(head_from_target, {0.f, 0.f, 0.f, 1.f});

  // Compute the angle between the vector towards the reticle and the vector
  // towards the target object by computing the arc cosine of their normalized
  // dot product.
  float angle = std::acos(std::max(
      -1.f, std::min(1.f, VectorInnerProduct(reticle_vector, target_vector) /
                              VectorNorm(reticle_vector) /
                              VectorNorm(target_vector))));
  return angle < kAngleLimit;
}

void HelloVrApp::LoadAndPlayTargetObjectSound() {
  // Preload sound files.
  gvr_audio_api_->PreloadSoundfile(kObjectSoundFile);
  gvr_audio_api_->PreloadSoundfile(kSuccessSoundFile);
  // Create sound file handler from preloaded sound file.
  audio_source_id_ = gvr_audio_api_->CreateSoundObject(kObjectSoundFile);
  // Set sound object to current target position.
  gvr_audio_api_->SetSoundObjectPosition(
      audio_source_id_, model_target_.m[0][3], model_target_.m[1][3],
      model_target_.m[2][3]);
  // Trigger sound object playback.
  gvr_audio_api_->PlaySound(audio_source_id_, true /* looped playback */);
}

}  // namespace ndk_hello_vr
