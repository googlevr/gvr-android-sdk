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

#ifndef HELLOVRBETA_APP_SRC_MAIN_JNI_CONTROLLERS_H_  // NOLINT
#define HELLOVRBETA_APP_SRC_MAIN_JNI_CONTROLLERS_H_  // NOLINT

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/asset_manager.h>
#include <jni.h>

#include <vector>

#include "shader_program.h"  // NOLINT
#include "util.h"  // NOLINT
#include "vr/gvr/capi/include/gvr_beta.h"
#include "vr/gvr/capi/include/gvr_controller.h"
#include "vr/gvr/capi/include/gvr_gesture.h"
#include "vr/gvr/capi/include/gvr_types.h"

namespace ndk_hello_vr_beta {

/**
 * Represents a single 3DOF or 6DOF controller.
 */
class Controller {
 public:
  Controller(gvr::ControllerApi* gvr_controller_api, int32_t index,
             gvr::ControllerHandedness handedness);

  void Update(gvr::ControllerApi* gvr_controller_api_,
              const gvr::Mat4f& head_space_from_start_space_transform,
              float floor_offset);

  int32_t GetIndex() const { return index_; }
  gvr_beta_controller_configuration_type GetType() const { return type_; }
  const gvr::ControllerState& GetState() const { return state_; }
  const gvr::Vec3f& GetPosition() const { return position_; }
  const gvr::Quatf GetOrientation() const { return state_.GetOrientation(); }
  const gvr::Mat4f& GetTransform() const { return transform_; }
  const gvr::Mat4f& GetLaserTransform() const { return laser_transform_; }

  // 6DOF controller tracking can fail if it becomes occluded or out of view.
  bool IsTracking() const { return is_tracking_; }
  bool IsOutOfFov() const { return is_out_of_fov_; }
  bool IsLaserShown() const { return IsTracking() && show_laser_; }
  void SetIsLaserShown(bool show_laser) { show_laser_ = show_laser; }

  float GetBatteryCharge() const { return battery_charge_; }

  bool GetSwipeGesture(gvr_gesture_direction* swipe_direction) const;

 private:
  void UpdateTrackingStatus();

  int32_t index_;
  gvr::ControllerHandedness handedness_;
  gvr_beta_controller_configuration_type type_;
  gvr::ControllerState state_;
  gvr::GestureApi gesture_api_;
  gvr::Vec3f position_;
  gvr::Mat4f transform_;
  gvr::Mat4f laser_transform_;
  bool show_laser_;
  bool is_tracking_;
  bool is_out_of_fov_;
  float battery_charge_;
};

/**
 * Manages connected controllers and rendering.
 */
class Controllers {
 public:
  explicit Controllers(gvr::GvrApi* gvr_api);
  void Initialize(JNIEnv* env, jobject java_asset_mgr,
                  AAssetManager* asset_mgr);

  void Pause();
  void Resume();
  void Update(const gvr::Mat4f& head_space_from_start_space_transform,
              float floor_offset);

  void Draw(const gvr::Mat4f view[2],
            const gvr::Mat4f view_projection[2]) const;

  void ForEachLaser(
      const std::function<void(int, const gvr::Vec3f& origin,
                               const gvr::Vec3f& direction)>& callback);

  void SetControllerForLaser(int index);

  void SetOnClickDown(std::function<void(int)> on_click);
  void SetOnClickUp(std::function<void(int)> on_click);
  void SetOnTriggerDown(std::function<void(int)> on_trigger);
  void SetOnTriggerUp(std::function<void(int)> on_trigger);
  void SetOnGripDown(std::function<void(int)> on_grip);
  void SetOnGripUp(std::function<void(int)> on_grip);
  void SetOnAppButtonDown(std::function<void(int)> on_app_button);
  void SetOnAppButtonUp(std::function<void(int)> on_app_button);
  void SetOnSwipe(std::function<void(int, gvr_gesture_direction)> on_swipe);

  Controller& GetController(int index) { return controllers_[index]; }

 private:
  void UpdateBatteryUniforms(const Controller& controller) const;
  void ReconnectIfRequired();

  gvr::GvrApi* gvr_api_;
  std::unique_ptr<gvr::ControllerApi> gvr_controller_api_;

  ControllerShaderProgram controller_shader_;
  TexturedShaderProgram laser_shader_;
  TexturedMesh controller_6dof_mesh_;
  Texture controller_6dof_texture_;
  TexturedMesh controller_3dof_mesh_;
  Texture controller_3dof_texture_;
  TexturedMesh laser_mesh_;
  Texture laser_texture_;

  std::vector<Controller> controllers_;

  std::function<void(int index)> on_click_down_;
  std::function<void(int index)> on_click_up_;
  std::function<void(int index)> on_trigger_down_;
  std::function<void(int index)> on_trigger_up_;
  std::function<void(int index)> on_grip_down_;
  std::function<void(int index)> on_grip_up_;
  std::function<void(int index)> on_app_button_down_;
  std::function<void(int index)> on_app_button_up_;
  std::function<void(int index, gvr_gesture_direction direction)> on_swipe_;
};

}  // namespace ndk_hello_vr_beta

#endif  // HELLOVRBETA_APP_SRC_MAIN_JNI_CONTROLLERS_H_  // NOLINT
