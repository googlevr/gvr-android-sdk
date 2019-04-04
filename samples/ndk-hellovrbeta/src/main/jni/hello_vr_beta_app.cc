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

#include "hello_vr_beta_app.h"  // NOLINT

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>

#include "util.h"  // NOLINT
#include "vr/gvr/capi/include/gvr_types.h"
#include "vr/gvr/capi/include/gvr_version.h"

namespace ndk_hello_vr_beta {

namespace {
static constexpr float kZNear = 0.01f;
static constexpr float kZFar = 20.0f;
static constexpr float kNeckModelFactor = 1.0f;

// The objects are about 1 meter in radius, so the min/max target distance are
// set so that the objects are always within a range, which is about 5 meters
// across.
static constexpr float kTargetRadius = 0.5f;
static constexpr float kMinTargetDistance = 2.5f;
static constexpr float kMaxTargetDistance = 3.5f;
static constexpr float kMinTargetHeight = kTargetRadius;
static constexpr float kMaxTargetHeight = kMinTargetHeight + 3.0f;

// Depth of the ground plane, in meters. If this (and other distances)
// are too far, 6DOF tracking will have no visible effect.
static constexpr float kDefaultFloorOffset = -1.7f;

static constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

// Sound file in APK assets.
static constexpr const char* kObjectSoundFile = "audio/HelloVRBeta_Loop.ogg";
static constexpr const char* kSuccessSoundFile =
    "audio/HelloVRBeta_Activation.ogg";
}  // namespace

HelloVrBetaApp::HelloVrBetaApp(JNIEnv* env, jobject asset_mgr_obj,
                               gvr_context* gvr_context,
                               std::unique_ptr<gvr::AudioApi> gvr_audio_api)
    : context_(gvr_context),
      gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      gvr_audio_api_(std::move(gvr_audio_api)),
      viewports_{gvr_api_->CreateBufferViewport(),
                 gvr_api_->CreateBufferViewport()},
      see_through_config_(gvr_beta_see_through_config_create(gvr_context)),
      see_through_mode_(SHOW_SEE_THROUGH),
      see_through_effect_(GVR_BETA_SEE_THROUGH_CAMERA_MODE_RAW_IMAGE),
      controllers_(gvr_api_.get()),
      controller_on_target_index_(-1),
      target_held_(false),
      audio_source_id_(-1),
      success_source_id_(-1),
      java_asset_mgr_(env->NewGlobalRef(asset_mgr_obj)),
      asset_mgr_(AAssetManager_fromJava(env, asset_mgr_obj)) {
  LOGD("Built with GVR version: %s", GVR_SDK_VERSION_STRING);

  controllers_.SetOnClickDown(
      [this](int controller_index) { OnTrigger(controller_index); });
  controllers_.SetOnTriggerDown(
      [this](int controller_index) { OnTrigger(controller_index); });
  controllers_.SetOnAppButtonDown(
      [this](int controller_index) { OnGrabTarget(controller_index); });
  controllers_.SetOnAppButtonUp(
      [this](int controller_index) { OnReleaseTarget(controller_index); });
  controllers_.SetOnGripDown(
      [this](int controller_index) { OnGrabTarget(controller_index); });
  controllers_.SetOnGripUp(
      [this](int controller_index) { OnReleaseTarget(controller_index); });
  controllers_.SetOnSwipe(
      [this](int controller_index, gvr_gesture_direction direction) {
        OnSwipe(controller_index, direction);
      });
}

HelloVrBetaApp::~HelloVrBetaApp() {
  // Join the audio initialization thread in case it still exists.
  if (audio_initialization_thread_.joinable()) {
    audio_initialization_thread_.join();
  }
}

void HelloVrBetaApp::OnSurfaceCreated(JNIEnv* env) {
  gvr_api_->InitializeGl();

  HELLOVRBETA_CHECK(gvr_api_->IsFeatureSupported(GVR_FEATURE_MULTIVIEW));

  // Initialize the see-through settings.
  UpdateSeeThroughSettings();

  shader_.Link();
  alpha_shader_.Link();

  CheckGLError("Obj program");

  GLuint position_param = shader_.GetPositionAttribute();
  GLuint uv_param = shader_.GetUVAttribute();
  GLuint alpha_position_param = alpha_shader_.GetPositionAttribute();
  GLuint alpha_uv_param = alpha_shader_.GetUVAttribute();

  CheckGLError("Obj program params");

  controllers_.Initialize(env, java_asset_mgr_, asset_mgr_);

  HELLOVRBETA_CHECK(room_.Initialize(asset_mgr_, "CubeRoom.obj",
                                     alpha_position_param, alpha_uv_param));
  HELLOVRBETA_CHECK(room_texture_.Initialize(env, java_asset_mgr_,
                                             "CubeRoom_BakedDiffuse.png"));
  HELLOVRBETA_CHECK(target_object_mesh_.Initialize(asset_mgr_, "TriSphere.obj",
                                                   position_param, uv_param));
  HELLOVRBETA_CHECK(target_object_not_selected_texture_.Initialize(
      env, java_asset_mgr_, "TriSphere_Blue_BakedDiffuse.png"));
  HELLOVRBETA_CHECK(target_object_selected_texture_.Initialize(
      env, java_asset_mgr_, "TriSphere_Pink_BakedDiffuse.png"));

  // Target object first appears directly in front of user.
  SetTargetPosition({0.0f, 1.0f, -kMinTargetDistance});

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
  // whose width is half the offscreen render width.
  gvr::Sizei half_size = {render_size_.width / 2, render_size_.height};
  specs[0].SetMultiviewLayers(2);
  specs[0].SetSize(half_size);

  swapchain_.reset(new gvr::SwapChain(gvr_api_->CreateSwapChain(specs)));

  viewport_list_.reset(
      new gvr::BufferViewportList(gvr_api_->CreateEmptyBufferViewportList()));
  viewport_list_->SetToRecommendedBufferViewports();
  const gvr_rectf fullscreen = {0, 1, 0, 1};
  for (int eye = 0; eye < 2; ++eye) {
    viewport_list_->GetBufferViewport(eye, &viewports_[eye]);
    viewports_[eye].SetSourceUv(fullscreen);
    viewports_[eye].SetSourceLayer(eye);
    viewport_list_->SetBufferViewport(eye, viewports_[eye]);
  }
  // Initialize audio engine and preload sample in a separate thread to avoid
  // any delay during construction and app initialization. Only do this once.
  if (!audio_initialization_thread_.joinable()) {
    audio_initialization_thread_ =
        std::thread(&HelloVrBetaApp::LoadAndPlayTargetObjectSound, this);
  }
}

float HelloVrBetaApp::GetFloorOffset() {
  gvr::Value floor_offset;
  constexpr float kMaxPersonHeight = 3.0f;
  if (gvr_api_->GetCurrentProperties().Get(GVR_PROPERTY_TRACKING_FLOOR_HEIGHT,
                                           &floor_offset) &&
      floor_offset.f < 0.0f && floor_offset.f > -kMaxPersonHeight) {
    return floor_offset.f;
  }
  return kDefaultFloorOffset;
}

void HelloVrBetaApp::OnDrawFrame() {
  gvr::Frame frame = swapchain_->AcquireFrame();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;

  // Note that neck model is a no-op, unless head-tracking is lost.
  gvr::Mat4f head_view = gvr_api_->ApplyNeckModel(
      gvr_api_->GetHeadSpaceFromStartSpaceTransform(target_time),
      kNeckModelFactor);

  // This may change when the floor height changes so it's computed every frame.
  float floor_offset = GetFloorOffset();
  // Incorporate the floor height into the head_view
  head_view =
      MatrixMul(head_view, GetTranslationMatrix({0.0f, floor_offset, 0.0f}));

  gvr::Mat4f view[2];
  gvr::Mat4f view_projection[2];
  for (int eye = 0; eye < 2; ++eye) {
    view[eye] = MatrixMul(
        gvr_api_->GetEyeFromHeadMatrix(static_cast<gvr::Eye>(eye)), head_view);
    gvr::Mat4f projection =
        ProjectionMatrixFromView(viewports_[eye].GetSourceFov(), kZNear, kZFar);
    view_projection[eye] = MatrixMul(projection, view[eye]);
  }

  controllers_.Update(head_view, floor_offset);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw the world.
  frame.BindBuffer(0);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  DrawWorld(view, view_projection);
  frame.Unbind();

  // Submit frame.
  frame.Submit(*viewport_list_, head_view);

  CheckGLError("onDrawFrame");

  // Update audio head rotation in audio API.
  gvr_audio_api_->SetHeadPose(head_view);
  gvr_audio_api_->Update();
}

void HelloVrBetaApp::OnTrigger(int controller_index) {
  if (!target_held_ && controller_index == controller_on_target_index_) {
    GenerateNewTargetPosition();
  } else {
    // The last controller that had a trigger pressed shows the laser.
    controllers_.SetControllerForLaser(controller_index);
  }
}

void HelloVrBetaApp::OnGrabTarget(int controller_index) {
  if (controller_index == controller_on_target_index_) {
    target_held_ = true;
  }
}

void HelloVrBetaApp::OnReleaseTarget(int controller_index) {
  if (controller_index == controller_on_target_index_) {
    target_held_ = false;
  }
}

void HelloVrBetaApp::OnSwipe(int controller_index,
                             gvr_gesture_direction direction) {
  switch (direction) {
    case GVR_GESTURE_DIRECTION_LEFT:
      --see_through_mode_;
      // If see_through_mode_ is too low, loop back to the last mode.
      if (see_through_mode_ < SHOW_SEE_THROUGH) {
        see_through_mode_ = NO_SEE_THROUGH;
      }
      break;
    case GVR_GESTURE_DIRECTION_RIGHT:
      ++see_through_mode_;
      // If see_through_mode_ is too high, loop back to the first mode.
      if (see_through_mode_ > NO_SEE_THROUGH) {
        see_through_mode_ = SHOW_SEE_THROUGH;
      }
      break;
    case GVR_GESTURE_DIRECTION_DOWN:
    case GVR_GESTURE_DIRECTION_UP:
      see_through_effect_ =
          (see_through_effect_ == GVR_BETA_SEE_THROUGH_CAMERA_MODE_RAW_IMAGE)
              ? GVR_BETA_SEE_THROUGH_CAMERA_MODE_TONEMAPPED
              : GVR_BETA_SEE_THROUGH_CAMERA_MODE_RAW_IMAGE;
      break;
  }
  UpdateSeeThroughSettings();
}

void HelloVrBetaApp::OnPause() {
  gvr_api_->PauseTracking();
  gvr_audio_api_->Pause();
  controllers_.Pause();
}

void HelloVrBetaApp::OnResume() {
  gvr_api_->ResumeTracking();
  gvr_api_->RefreshViewerProfile();
  gvr_audio_api_->Resume();
  controllers_.Resume();
}

bool HelloVrBetaApp::IsSeeThroughAvailable() const {
  const int32_t see_through_feature = GVR_BETA_FEATURE_SEE_THROUGH;
  if (!gvr_api_->IsFeatureSupported(see_through_feature)) {
    return false;
  }

  if (gvr_api_->GetUserPrefs().IsFeatureEnabled(see_through_feature)) {
    return true;
  }

  // Ask the user to turn on see-through. This is does not block.
  gvr_api_->RequestFeatures(/*required_features=*/nullptr,
                            /*required_count=*/0, &see_through_feature,
                            /*optional_count=*/1,
                            /*on_complete_activity=*/nullptr);

  // Even if see-through is enabled through the feature request, see-through is
  // still not available since turning on see-through requires a reboot.
  return false;
}

void HelloVrBetaApp::UpdateSeeThroughSettings() {
  // If trying to show pass through, but it is not available, return.
  if (!IsSeeThroughAvailable()) {
    see_through_mode_ = NO_SEE_THROUGH;
    return;
  }

  // We have 3 see-through modes, if the modes are SHOW_SEE_THROUGH or
  // SHOW_TRANSLUCENT_SEE_THROUGH, we need to set turn on see-through in gvr.
  // When we use SHOW_TRANSLUCENT_SEE_THROUGH, the room will be rendered
  // translucently. If see-through is enabled, see_through_effect_
  // determines if we use the raw camera image, or if we render the tone mapped
  // camera image.
  int camera_mode = GVR_BETA_SEE_THROUGH_CAMERA_MODE_DISABLED;
  int scene_type = GVR_BETA_SEE_THROUGH_SCENE_TYPE_VIRTUAL_SCENE;

  switch (see_through_mode_) {
    case SHOW_SEE_THROUGH:
    case SHOW_TRANSLUCENT_SEE_THROUGH:
      camera_mode = see_through_effect_;
      // If see-through is on, we also set the scene type to augmented scene.
      // This will alter the head pose to match the see-through's camera
      // position which could be offset from the eye.
      scene_type = GVR_BETA_SEE_THROUGH_SCENE_TYPE_AUGMENTED_SCENE;
      break;
    case NO_SEE_THROUGH:
      break;
  }
  gvr_beta_see_through_config_set_camera_mode(see_through_config_, camera_mode);
  gvr_beta_see_through_config_set_scene_type(see_through_config_, scene_type);
  gvr_beta_set_see_through_config(context_, see_through_config_);
}

/**
 * Draws a frame for a particular view.
 */
void HelloVrBetaApp::DrawWorld(const gvr::Mat4f view[2],
                               const gvr::Mat4f view_projection[2]) {
  glViewport(0, 0, render_size_.width / 2, render_size_.height);
  if (see_through_mode_ != SHOW_SEE_THROUGH) {
    DrawRoom(view_projection);
  }

  DrawTarget(view_projection);
  controllers_.Draw(view, view_projection);
}

void HelloVrBetaApp::DrawTarget(const gvr::Mat4f view_projection[2]) {
  // If we're holding onto the target, attach its position to the controller.
  if (target_held_ && controller_on_target_index_ >= 0) {
    Controller& controller =
        controllers_.GetController(controller_on_target_index_);
    const gvr::Mat4f& laser_transform = controller.GetLaserTransform();
    gvr::Vec3f laser_position = GetMatrixTranslation(laser_transform);
    gvr::Vec3f offset =
        MatrixVectorMul(laser_transform, {0.0f, 0.0f, -1.0f}) * kTargetRadius;
    SetTargetPosition(laser_position + offset);
  }

  shader_.Use();
  shader_.SetModelViewProjection(model_target_, view_projection);

  if (IsPointingAtTarget()) {
    target_object_selected_texture_.Bind();
  } else {
    target_object_not_selected_texture_.Bind();
  }
  target_object_mesh_.Draw();

  CheckGLError("Drawing target object");
}

void HelloVrBetaApp::DrawRoom(const gvr::Mat4f view_projection[2]) {
  gvr::Mat4f model_room = GetTranslationMatrix({0.0f, 0.0f, 0.0f});

  alpha_shader_.Use();
  alpha_shader_.SetModelViewProjection(model_room, view_projection);
  float alpha = see_through_mode_ == SHOW_TRANSLUCENT_SEE_THROUGH ? 0.7f : 1.0f;
  alpha_shader_.SetAlpha(alpha);

  room_texture_.Bind();
  room_.Draw();

  CheckGLError("Drawing room");
}

void HelloVrBetaApp::SetTargetPosition(const gvr::Vec3f& position) {
  model_target_ = GetTranslationMatrix(position);
  if (audio_source_id_ >= 0) {
    gvr_audio_api_->SetSoundObjectPosition(audio_source_id_, position.x,
                                           position.y, position.z);
  }
}

void HelloVrBetaApp::GenerateNewTargetPosition() {
  float angle = RandomUniformFloat(-M_PI, M_PI);
  float distance = RandomUniformFloat(kMinTargetDistance, kMaxTargetDistance);
  float height = RandomUniformFloat(kMinTargetHeight, kMaxTargetHeight);
  SetTargetPosition(
      {std::cos(angle) * distance, height, std::sin(angle) * distance});
}

bool HelloVrBetaApp::IsPointingAtTarget() {
  bool result = false;
  if (target_held_) {
    // consider a held target active.
    return true;
  }
  controller_on_target_index_ = -1;
  controllers_.ForEachLaser(
      [&](int index, const gvr::Vec3f& origin, const gvr::Vec3f& direction) {
        if (DoesRayIntersectSphere(origin, direction,
                                   GetMatrixTranslation(model_target_),
                                   kTargetRadius)) {
          controller_on_target_index_ = index;
          result = true;
        }
      });
  return result;
}

void HelloVrBetaApp::LoadAndPlayTargetObjectSound() {
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
  gvr_audio_api_->PlaySound(audio_source_id_, /*looping_enabled=*/true);
}

}  // namespace ndk_hello_vr_beta
