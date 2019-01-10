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

#include "util.h"  // NOLINT
#include "vr/gvr/capi/include/gvr_types.h"
#include "vr/gvr/capi/include/gvr_version.h"

namespace ndk_hello_vr {

namespace {
static constexpr float kZNear = 0.01f;
static constexpr float kZFar = 10.0f;
static constexpr float kNeckModelFactor = 1.0f;

// The objects are about 1 meter in radius, so the min/max target distance are
// set so that the objects are always within the room (which is about 5 meters
// across) and the reticle is always closer than any objects.
static constexpr float kMinTargetDistance = 2.5f;
static constexpr float kMaxTargetDistance = 3.5f;
static constexpr float kMinTargetHeight = 0.5f;
static constexpr float kMaxTargetHeight = kMinTargetHeight + 3.0f;
static constexpr float kReticleDistance = 1.9f;

// Depth of the ground plane, in meters. If this (and other distances)
// are too far, 6DOF tracking will have no visible effect.
static constexpr float kDefaultFloorHeight = -1.7f;
static constexpr float kSafetyRingHeightDelta = 0.01f;

static constexpr float kDefaultSafetyRingRadius = 1.0f;

static constexpr int kCoordsPerVertex = 3;

static constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

// Angle threshold for determining whether the controller is pointing at the
// object.
static constexpr float kAngleLimit = 0.2f;

// Sound file in APK assets.
static constexpr const char* kObjectSoundFile = "audio/HelloVR_Loop.ogg";
static constexpr const char* kSuccessSoundFile = "audio/HelloVR_Activation.ogg";

static constexpr int kTargetMeshCount = 3;

// Each shader has two variants: a single-eye ES 2.0 variant, and a multiview
// ES 3.0 variant.  The multiview vertex shaders use transforms defined by
// arrays of mat4 uniforms, using gl_ViewID_OVR to determine the array index.

// Simple shaders to render .obj files without any lighting.
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

constexpr const char* kReticleVertexShaders[] = {
    R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    varying vec2 v_Coords;

    void main() {
      v_Coords = a_Position.xy;
      gl_Position = u_MVP * a_Position;
    })glsl",
    // The following shader is for multiview rendering.
    R"glsl(#version 300 es
    #extension GL_OVR_multiview2 : enable

    layout(num_views=2) in;
    uniform mat4 u_MVP[2];
    in vec4 a_Position;
    out vec2 v_Coords;

    void main() {
      v_Coords = a_Position.xy;
      gl_Position = u_MVP[gl_ViewID_OVR] * a_Position;
    })glsl"};

constexpr const char* kReticleFragmentShaders[] = {
    R"glsl(
    precision mediump float;

    varying vec2 v_Coords;

    void main() {
      float r = length(v_Coords);
      float alpha = smoothstep(0.5, 0.6, r) * (1.0 - smoothstep(0.8, 0.9, r));
      if (alpha == 0.0) discard;
      gl_FragColor = vec4(alpha);
    })glsl",
    // The following shader is for multiview rendering.
    R"glsl(#version 300 es
    precision mediump float;

    in vec2 v_Coords;
    out vec4 FragColor;

    void main() {
      float r = length(v_Coords);
      float alpha = smoothstep(0.5, 0.6, r) * (1.0 - smoothstep(0.8, 0.9, r));
      if (alpha == 0.0) discard;
      FragColor = vec4(alpha);
    })glsl"};

}  // anonymous namespace

HelloVrApp::HelloVrApp(JNIEnv* env, jobject asset_mgr_obj,
                       gvr_context* gvr_context,
                       std::unique_ptr<gvr::AudioApi> gvr_audio_api)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      gvr_audio_api_(std::move(gvr_audio_api)),
      viewport_left_(gvr_api_->CreateBufferViewport()),
      viewport_right_(gvr_api_->CreateBufferViewport()),
      reticle_coords_({{
          -1.f,
          1.f,
          0.0f,
          -1.f,
          -1.f,
          0.0f,
          1.f,
          1.f,
          0.0f,
          -1.f,
          -1.f,
          0.0f,
          1.f,
          -1.f,
          0.0f,
          1.f,
          1.f,
          0.0f,
      }}),
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
  HELLOVR_CHECK(safety_ring_.Initialize(env, asset_mgr_, "SafetyRing.obj",
                                        obj_position_param_, obj_uv_param_));
  HELLOVR_CHECK(safety_ring_tex_.Initialize(env, java_asset_mgr_,
                                            "SafetyRing_Alpha.png"));

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
  model_target_ = GetTranslationMatrix({0.0f, 1.5f, -kMinTargetDistance});

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
    gvr::Sizei half_size = {render_size_.width / 2, render_size_.height};
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

  // Trigger click event if app/click/trigger button is clicked.
  if (gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_APP) ||
      gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_CLICK) ||
      gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_TRIGGER)) {
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
  // Note that neck model application is a no-op if the viewer supports 6DoF
  // head tracking
  head_view_ = gvr_api_->ApplyNeckModel(
      gvr_api_->GetHeadSpaceFromStartSpaceTransform(target_time),
      kNeckModelFactor);

  gvr::Value floor_height;
  // This may change when the floor height changes so it's computed every frame.
  float ground_y = gvr_api_->GetCurrentProperties().Get(
                       GVR_PROPERTY_TRACKING_FLOOR_HEIGHT, &floor_height)
                       ? floor_height.f
                       : kDefaultFloorHeight;
  // Incorporate the floor height into the head_view
  head_view_ =
      MatrixMul(head_view_, GetTranslationMatrix({0.0f, ground_y, 0.0f}));

  viewport_list_->SetToRecommendedBufferViewports();

  gvr::BufferViewport reticle_viewport = gvr_api_->CreateBufferViewport();
  reticle_viewport.SetSourceBufferIndex(1);
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) {
    // Do not reproject the reticle if it's head-locked.
    reticle_viewport.SetReprojection(GVR_REPROJECTION_NONE);
  }
  const gvr_rectf fullscreen = {0, 1, 0, 1};
  reticle_viewport.SetSourceUv(fullscreen);
  UpdateReticlePosition();

  gvr::Mat4f modelview_room[2];

  gvr::Value cylinder_radius;
  float safety_ring_radius =
      gvr_api_->GetCurrentProperties().Get(
          GVR_PROPERTY_SAFETY_CYLINDER_ENTER_RADIUS, &cylinder_radius)
          ? cylinder_radius.f
          : kDefaultSafetyRingRadius;
  gvr::Mat4f model_safety_ring = {
      {{safety_ring_radius, 0.0f, 0.0f, 0.0f},
       {0.0f, safety_ring_radius, 0.0f, ground_y + kSafetyRingHeightDelta},
       {0.0f, 0.0f, safety_ring_radius, 0.0f},
       {0.0f, 0.0f, 0.0f, 1.0f}}};
  gvr::Mat4f modelview_safety_ring[2];

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
    modelview_room[eye] = eye_views[eye];
    modelview_safety_ring[eye] = MatrixMul(eye_views[eye], model_safety_ring);

    const gvr_rectf fov = viewport[eye]->GetSourceFov();

    const gvr::Mat4f perspective =
        PerspectiveMatrixFromView(fov, kZNear, kZFar);
    modelview_projection_target_[eye] =
        MatrixMul(perspective, modelview_target_[eye]);
    modelview_projection_room_[eye] =
        MatrixMul(perspective, modelview_room[eye]);
    modelview_projection_safety_ring_[eye] =
        MatrixMul(perspective, modelview_safety_ring[eye]);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
  gvr::Value safety_region;
  int safety_region_type = gvr_api_->GetCurrentProperties().Get(
                               GVR_PROPERTY_SAFETY_REGION, &safety_region)
                               ? safety_region.i
                               : GVR_SAFETY_REGION_NONE;
  if (safety_region_type == GVR_SAFETY_REGION_CYLINDER) {
    DrawSafetyRing(view);
  }
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

void HelloVrApp::DrawSafetyRing(ViewType view) {
  glUseProgram(obj_program_);

  if (view == kMultiview) {
    glUniformMatrix4fv(
        obj_modelview_projection_param_, 2, GL_FALSE,
        MatrixPairToGLArray(modelview_projection_safety_ring_).data());
  } else {
    glUniformMatrix4fv(
        obj_modelview_projection_param_, 1, GL_FALSE,
        MatrixToGLArray(modelview_projection_safety_ring_[view]).data());
  }

  safety_ring_tex_.Bind();
  safety_ring_.Draw();

  CheckGLError("Draw Safety Ring");
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
                        false, 0, reticle_coords_.data());
  glEnableVertexAttribArray(reticle_position_param_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(reticle_position_param_);

  CheckGLError("Drawing reticle");
}

void HelloVrApp::HideTarget() {
  cur_target_object_ = RandomUniformInt(kTargetMeshCount);

  float angle = RandomUniformFloat(-M_PI, M_PI);
  float distance = RandomUniformFloat(kMinTargetDistance, kMaxTargetDistance);
  float height = RandomUniformFloat(kMinTargetHeight, kMaxTargetHeight);
  gvr::Vec3f target_position = {std::cos(angle) * distance, height,
                                std::sin(angle) * distance};

  model_target_ = GetTranslationMatrix(target_position);

  if (audio_source_id_ >= 0) {
    gvr_audio_api_->SetSoundObjectPosition(audio_source_id_, target_position.x,
                                           target_position.y,
                                           target_position.z);
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

  float angle = AngleBetweenVectors(reticle_vector, target_vector);
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
