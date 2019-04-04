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

#include "controllers.h"  // NOLINT

#include <cmath>

namespace ndk_hello_vr_beta {

namespace {

// Hardcoded offsets and rotations for the controller laser.
// Laser is tilted -15 degrees from the angle of the touchpad. See:
// https://developers.google.com/vr/distribute/daydream/design-requirements#UX-C1
const gvr::Mat4f kLaserRotation =
    GetAxisAngleRotationMatrix({1.0f, 0.0f, 0.0f}, -.262f);

const gvr::Mat4f k6dofLaserTransform =
    MatrixMul(GetTranslationMatrix({0.0f, -0.007f, -0.12f}), kLaserRotation);
constexpr gvr::Rectf k6dofBatteryUVRect({0.1079f, 0.1914f, 0.5391f, 0.5601f});
constexpr gvr::Vec2f k6dofBatteryChargeOffset({0.0f, -0.4072f});
constexpr gvr::Vec2f k6dofBatteryCriticalOffset({0.0f, -0.3862f});

const gvr::Mat4f k3dofLaserTransform =
    MatrixMul(GetTranslationMatrix({0.0f, -0.007f, -0.055f}), kLaserRotation);
constexpr gvr::Rectf k3dofBatteryUVRect({0.06641f, 0.2539f, 0.2304f, 0.25f});
constexpr gvr::Vec2f k3dofBatteryChargeOffset({0.0f, -0.1797f});
constexpr gvr::Vec2f k3dofBatteryCriticalOffset({0.0f, -0.2207f});

constexpr float kBatteryCriticalPercentage = 0.25f;

}  // unnamed namespace

Controller::Controller(gvr::ControllerApi* gvr_controller_api, int32_t index,
                       gvr::ControllerHandedness handedness)
    : index_(index),
      handedness_(handedness),
      type_(GVR_BETA_CONTROLLER_CONFIGURATION_UNKNOWN),
      show_laser_(index == 0),
      is_tracking_(true),
      is_out_of_fov_(false) {
  // Read current controller state for controller type.
  state_.Update(*gvr_controller_api, index_);
  type_ = static_cast<gvr_beta_controller_configuration_type>(
      gvr_beta_controller_get_configuration_type(gvr_controller_api->cobj(),
                                                 state_.cobj()));
}

void Controller::UpdateTrackingStatus() {
  if (type_ == GVR_BETA_CONTROLLER_CONFIGURATION_3DOF) return;

  // If we're a 6DOF controller, see if it's tracking correctly.
  auto status = gvr_beta_controller_state_get_tracking_status(state_.cobj());
  is_tracking_ =
      status == GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_TRACKING_NOMINAL;
  is_out_of_fov_ =
      status == GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_OUT_OF_FOV;
}

void Controller::Update(gvr::ControllerApi* gvr_controller_api,
                        const gvr::Mat4f& head_space_from_start_space_transform,
                        float floor_offset) {
  const int old_status = state_.GetApiStatus();
  const int old_connection_state = state_.GetConnectionState();

  // Apply an arm model for 3DOF controllers, 6DOF controllers will ignore this.
  gvr_controller_api->ApplyArmModel(
      index_, handedness_, gvr::kArmModelBehaviorFollowGazeWith6DOFPosition,
      head_space_from_start_space_transform);

  // Read current controller state.
  state_.Update(*gvr_controller_api, index_);

  UpdateTrackingStatus();

  // Print new API status and connection state, if they changed.
  if (state_.GetApiStatus() != old_status ||
      state_.GetConnectionState() != old_connection_state) {
    LOGD(
        "Index: %d, controller API status: %s, connection state: %s", index_,
        gvr_controller_api_status_to_string(state_.GetApiStatus()),
        gvr_controller_connection_state_to_string(state_.GetConnectionState()));
  }

  position_ = state_.GetPosition();
  // ApplyArmModel updates the floor offset in the 3DOF case, but not 6DOF.
  if (type_ == GVR_BETA_CONTROLLER_CONFIGURATION_6DOF) {
    position_.y -= floor_offset;
  }

  transform_ = MatrixMul(GetTranslationMatrix(GetPosition()),
                         ControllerQuatToMatrix(GetOrientation()));
  if (type_ == GVR_BETA_CONTROLLER_CONFIGURATION_3DOF) {
    laser_transform_ = MatrixMul(transform_, k3dofLaserTransform);
  } else {
    laser_transform_ = MatrixMul(transform_, k6dofLaserTransform);
  }

  // Calculate the battery charge.
  float level = static_cast<float>(state_.GetBatteryLevel());
  battery_charge_ =
      level / static_cast<float>(GVR_CONTROLLER_BATTERY_LEVEL_FULL);

  // Track any gestures made since the last update:
  gesture_api_.Update(&state_);
}

bool Controller::GetSwipeGesture(gvr_gesture_direction* swipe_direction) const {
  // Get the number of detected gestures
  int num_gestures = gesture_api_.GetGestureCount();
  for (int i = 0; i < num_gestures; ++i) {
    const gvr::Gesture* gesture = gesture_api_.GetGesture(i);
    if (gesture_api_.GetGestureType(gesture) == GVR_GESTURE_SWIPE) {
      *swipe_direction = gesture_api_.GetGestureDirection(gesture);
      return true;
    }
  }
  return false;
}

Controllers::Controllers(gvr::GvrApi* gvr_api)
    : gvr_api_(gvr_api), gvr_controller_api_(new gvr::ControllerApi) {
  HELLOVRBETA_CHECK(gvr_controller_api_->Init(
      gvr::ControllerApi::DefaultOptions() | GVR_CONTROLLER_ENABLE_ARM_MODEL |
          GVR_CONTROLLER_ENABLE_GYRO,
      gvr_api_->cobj()));
}

void Controllers::Initialize(JNIEnv* env, jobject java_asset_mgr,
                             AAssetManager* asset_mgr) {
  controller_shader_.Link();

  GLuint position_attrib = controller_shader_.GetPositionAttribute();
  GLuint uv_attrib = controller_shader_.GetUVAttribute();

  HELLOVRBETA_CHECK(controller_6dof_mesh_.Initialize(
      asset_mgr, "Controller6DOF.obj", position_attrib, uv_attrib));
  HELLOVRBETA_CHECK(controller_6dof_texture_.Initialize(
      env, java_asset_mgr, "Controller6DOFDiffuse.png"));
  HELLOVRBETA_CHECK(controller_3dof_mesh_.Initialize(
      asset_mgr, "Controller3DOF.obj", position_attrib, uv_attrib));
  HELLOVRBETA_CHECK(controller_3dof_texture_.Initialize(
      env, java_asset_mgr, "Controller3DOFDiffuse.png"));

  laser_shader_.Link();

  position_attrib = laser_shader_.GetPositionAttribute();
  uv_attrib = laser_shader_.GetUVAttribute();

  HELLOVRBETA_CHECK(laser_mesh_.Initialize(asset_mgr, "Laser.obj",
                                           position_attrib, uv_attrib));
  HELLOVRBETA_CHECK(
      laser_texture_.Initialize(env, java_asset_mgr, "Laser.png"));

  Resume();
}

void Controllers::Pause() {
  if (gvr_controller_api_) gvr_controller_api_->Pause();
  controllers_.clear();
}

void Controllers::Resume() { gvr_controller_api_->Resume(); }

void Controllers::ReconnectIfRequired() {
  int32_t controller_count = gvr_controller_api_->GetControllerCount();
  if (controller_count != controllers_.size()) {
    const gvr::UserPrefs user_prefs = gvr_api_->GetUserPrefs();
    const int32_t dominant_hand =
        gvr_user_prefs_get_controller_handedness(user_prefs.cobj());
    controllers_.clear();
    for (int32_t i = 0; i < controller_count; ++i) {
      gvr::ControllerHandedness handedness =
          static_cast<gvr::ControllerHandedness>(i == 0 ? dominant_hand
                                                        : !dominant_hand);
      controllers_.push_back(
          Controller(gvr_controller_api_.get(), i, handedness));
    }
  }
}

void Controllers::Update(
    const gvr::Mat4f& head_space_from_start_space_transform,
    float floor_offset) {
  ReconnectIfRequired();

  for (auto& controller : controllers_) {
    controller.Update(gvr_controller_api_.get(),
                      head_space_from_start_space_transform, floor_offset);

    // Call controller input callbacks.
    if (on_click_down_ &&
        controller.GetState().GetButtonDown(GVR_CONTROLLER_BUTTON_CLICK)) {
      on_click_down_(controller.GetIndex());
    }
    if (on_click_up_ &&
        controller.GetState().GetButtonUp(GVR_CONTROLLER_BUTTON_CLICK)) {
      on_click_up_(controller.GetIndex());
    }
    if (on_app_button_down_ &&
        controller.GetState().GetButtonDown(GVR_CONTROLLER_BUTTON_APP)) {
      on_app_button_down_(controller.GetIndex());
    }
    if (on_app_button_up_ &&
        controller.GetState().GetButtonUp(GVR_CONTROLLER_BUTTON_APP)) {
      on_app_button_up_(controller.GetIndex());
    }
    if (on_grip_down_ &&
        controller.GetState().GetButtonDown(GVR_CONTROLLER_BUTTON_GRIP)) {
      on_grip_down_(controller.GetIndex());
    }
    if (on_grip_up_ &&
        controller.GetState().GetButtonUp(GVR_CONTROLLER_BUTTON_GRIP)) {
      on_grip_up_(controller.GetIndex());
    }
    if (on_trigger_down_ &&
        controller.GetState().GetButtonDown(GVR_CONTROLLER_BUTTON_TRIGGER)) {
      on_trigger_down_(controller.GetIndex());
    }
    if (on_trigger_up_ &&
        controller.GetState().GetButtonUp(GVR_CONTROLLER_BUTTON_TRIGGER)) {
      on_trigger_up_(controller.GetIndex());
    }

    gvr_gesture_direction gesture_direction;
    if (on_swipe_ && controller.GetSwipeGesture(&gesture_direction)) {
      on_swipe_(controller.GetIndex(), gesture_direction);
    }
  }
}

void Controllers::UpdateBatteryUniforms(const Controller& controller) const {
  // UV Rectangle in the texture that surrounds the battery indicator.
  gvr::Rectf uv_rect = {};
  // UV offset to move the UV rectangle to either the charge or critical icons.
  gvr::Vec2f offset = {};
  float charge = controller.GetBatteryCharge();

  // If the battery level is zero, it's unknown.
  if (controller.GetType() == GVR_BETA_CONTROLLER_CONFIGURATION_6DOF) {
    uv_rect = k6dofBatteryUVRect;
    offset = charge < kBatteryCriticalPercentage ? k6dofBatteryCriticalOffset
                                                 : k6dofBatteryChargeOffset;
  } else {
    uv_rect = k3dofBatteryUVRect;
    offset = charge < kBatteryCriticalPercentage ? k3dofBatteryCriticalOffset
                                                 : k3dofBatteryChargeOffset;
  }

  if (charge > 0.0) {
    uv_rect.right = Lerp(uv_rect.left, uv_rect.right, charge);
  }
  controller_shader_.SetBatteryOffset(offset);
  controller_shader_.SetBatteryUVRect(uv_rect);
}

void Controllers::Draw(const gvr::Mat4f view[2],
                       const gvr::Mat4f view_projection[2]) const {
  for (const auto& controller : controllers_) {
    // Don't draw controllers that are out of tracking FOV.
    if (controller.IsOutOfFov()) {
      continue;
    }

    const gvr::Mat4f& model_matrix = controller.GetTransform();

    controller_shader_.Use();
    UpdateBatteryUniforms(controller);
    controller_shader_.SetModelViewProjection(model_matrix, view_projection);
    // Show that tracking has failed by setting making it transparent.
    controller_shader_.SetAlpha(controller.IsTracking() ? 1.0f : 0.25f);

    if (controller.GetType() == GVR_BETA_CONTROLLER_CONFIGURATION_6DOF) {
      controller_6dof_texture_.Bind();
      controller_6dof_mesh_.Draw();
    } else {
      // Assume 3DOF
      controller_3dof_texture_.Bind();
      controller_3dof_mesh_.Draw();
    }

    if (controller.IsLaserShown()) {
      gvr::Mat4f laser_matrix = controller.GetLaserTransform();
      // Transform the laser so that it's always facing the camera.
      gvr::Vec3f controller_position = controller.GetPosition();
      gvr::Vec3f head_position = PositionFromHeadSpace(view[0]);
      gvr::Vec3f controller_to_head = head_position - controller_position;
      gvr::Vec3f controller_to_head_controller_space =
          MatrixVectorMul(GetOrthoInverse(model_matrix), controller_to_head);
      // Get the angle between {0, 1, 0} and controller_to_head_controller_space
      // projected onto the z plane.
      float angle = -std::atan2(controller_to_head_controller_space.x,
                                -controller_to_head_controller_space.y);
      gvr::Mat4f laser_model = MatrixMul(
          laser_matrix, GetAxisAngleRotationMatrix({0.0f, 0.0f, 1.0f}, -angle));

      // Transform the laser using left eye, Ideally this should be per eye.
      laser_shader_.Use();
      laser_shader_.SetModelViewProjection(laser_model, view_projection);

      // Use premultiplied alpha.
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      laser_texture_.Bind();
      laser_mesh_.Draw();
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
  }
  CheckGLError("Drawing controllers");
}

void Controllers::ForEachLaser(
    const std::function<void(int, const gvr::Vec3f& origin,
                             const gvr::Vec3f& direction)>& callback) {
  for (auto& controller : controllers_) {
    if (controller.IsLaserShown()) {
      gvr::Mat4f laser_transform = controller.GetLaserTransform();
      gvr::Vec3f origin = MatrixPointMul(laser_transform, {0.0f, 0.0f, 0.0f});
      gvr::Vec3f direction =
          MatrixVectorMul(laser_transform, {0.0f, 0.0f, -1.0f});

      callback(controller.GetIndex(), origin, direction);
    }
  }
}

void Controllers::SetControllerForLaser(int index) {
  for (int i = 0; i < static_cast<int>(controllers_.size()); ++i) {
    controllers_[i].SetIsLaserShown(index == i);
  }
}

void Controllers::SetOnClickDown(std::function<void(int index)> on_click_down) {
  on_click_down_ = std::move(on_click_down);
}

void Controllers::SetOnClickUp(std::function<void(int index)> on_click_up) {
  on_click_up_ = std::move(on_click_up);
}

void Controllers::SetOnTriggerDown(
    std::function<void(int index)> on_trigger_down) {
  on_trigger_down_ = std::move(on_trigger_down);
}

void Controllers::SetOnTriggerUp(std::function<void(int index)> on_trigger_up) {
  on_trigger_up_ = std::move(on_trigger_up);
}

void Controllers::SetOnGripDown(std::function<void(int index)> on_grip_down) {
  on_grip_down_ = std::move(on_grip_down);
}

void Controllers::SetOnGripUp(std::function<void(int index)> on_grip_up) {
  on_grip_up_ = std::move(on_grip_up);
}

void Controllers::SetOnAppButtonDown(
    std::function<void(int index)> on_app_button_down) {
  on_app_button_down_ = std::move(on_app_button_down);
}

void Controllers::SetOnAppButtonUp(
    std::function<void(int index)> on_app_button_up) {
  on_app_button_up_ = std::move(on_app_button_up);
}

void Controllers::SetOnSwipe(
    std::function<void(int, gvr_gesture_direction)> on_swipe) {
  on_swipe_ = std::move(on_swipe);
}

}  // namespace ndk_hello_vr_beta
