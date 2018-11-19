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
const gvr::Mat4f k3dofLaserTransform =
    MatrixMul(GetTranslationMatrix({0.0f, -0.007f, -0.055f}), kLaserRotation);

}  // unnamed namespace

Controller::Controller(gvr::ControllerApi* gvr_controller_api, int32_t index)
    : index_(index),
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
  // Read current controller state.
  state_.Update(*gvr_controller_api, index_);

  gvr::Vec3f head_position = {};
  // If this controller is 3DOF, apply an arm model and offset.
  if (type_ == GVR_BETA_CONTROLLER_CONFIGURATION_3DOF) {
    gvr_controller_api->ApplyArmModel(index_, gvr::kControllerRightHanded,
                                      GVR_ARM_MODEL_SYNC_GAZE,
                                      head_space_from_start_space_transform);
    head_position =
        PositionFromHeadSpace(head_space_from_start_space_transform);
  } else {
    // Incorporate the floor offset into the controller position.
    head_position.y = -floor_offset;
  }

  UpdateTrackingStatus();

  // Print new API status and connection state, if they changed.
  if (state_.GetApiStatus() != old_status ||
      state_.GetConnectionState() != old_connection_state) {
    LOGD(
        "Index: %d, controller API status: %s, connection state: %s", index_,
        gvr_controller_api_status_to_string(state_.GetApiStatus()),
        gvr_controller_connection_state_to_string(state_.GetConnectionState()));
  }

  position_ = state_.GetPosition() + head_position;

  transform_ = MatrixMul(GetTranslationMatrix(GetPosition()),
                         ControllerQuatToMatrix(GetOrientation()));
  if (type_ == GVR_BETA_CONTROLLER_CONFIGURATION_3DOF) {
    laser_transform_ = MatrixMul(transform_, k3dofLaserTransform);
  } else {
    laser_transform_ = MatrixMul(transform_, k6dofLaserTransform);
  }
}

Controllers::Controllers(gvr::GvrApi* gvr_api)
    : gvr_controller_api_(new gvr::ControllerApi) {
  HELLOVRBETA_CHECK(gvr_controller_api_->Init(
      gvr::ControllerApi::DefaultOptions() | GVR_CONTROLLER_ENABLE_ARM_MODEL,
      gvr_api->cobj()));
}

void Controllers::Initialize(JNIEnv* env, jobject java_asset_mgr,
                             AAssetManager* asset_mgr) {
  shader_.Link();

  GLuint position_attrib = shader_.GetPositionAttribute();
  GLuint uv_attrib = shader_.GetUVAttribute();

  HELLOVRBETA_CHECK(controller_6dof_mesh_.Initialize(
      asset_mgr, "Controller6DOF.obj", position_attrib, uv_attrib));
  HELLOVRBETA_CHECK(controller_6dof_texture_.Initialize(
      env, java_asset_mgr, "Controller6DOFDiffuse.png"));
  HELLOVRBETA_CHECK(controller_3dof_mesh_.Initialize(
      asset_mgr, "Controller3DOF.obj", position_attrib, uv_attrib));
  HELLOVRBETA_CHECK(controller_3dof_texture_.Initialize(
      env, java_asset_mgr, "Controller3DOFDiffuse.png"));
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

void Controllers::Update(
    const gvr::Mat4f& head_space_from_start_space_transform,
    float floor_offset) {
  // Check to see if we've lost or gained a new controller.
  int32_t controller_count = gvr_controller_api_->GetControllerCount();
  if (controller_count != controllers_.size()) {
    controllers_.clear();
    for (int32_t i = 0; i < controller_count; ++i) {
      controllers_.push_back(Controller(gvr_controller_api_.get(), i));
    }
  }

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
  }
}

void Controllers::Draw(const gvr::Mat4f view[2],
                       const gvr::Mat4f view_projection[2]) const {
  shader_.Use();

  for (const auto& controller : controllers_) {
    // Don't draw controllers that are out of tracking FOV.
    if (controller.IsOutOfFov()) {
      continue;
    }

    const gvr::Mat4f& model_matrix = controller.GetTransform();

    shader_.SetModelViewProjection(model_matrix, view_projection);
    // Show that tracking has failed by setting making it transparent.
    shader_.SetAlpha(controller.IsTracking() ? 1.0f : 0.25f);

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
      shader_.SetModelViewProjection(laser_model, view_projection);

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

}  // namespace ndk_hello_vr_beta
