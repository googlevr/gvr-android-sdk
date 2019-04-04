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

#ifndef HELLOVRBETA_APP_SRC_MAIN_JNI_HELLO_VR_BETA_APP_H_  // NOLINT
#define HELLOVRBETA_APP_SRC_MAIN_JNI_HELLO_VR_BETA_APP_H_  // NOLINT

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/asset_manager.h>
#include <jni.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "controllers.h"  // NOLINT
#include "shader_program.h"  // NOLINT
#include "util.h"  // NOLINT
#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_audio.h"
#include "vr/gvr/capi/include/gvr_controller.h"
#include "vr/gvr/capi/include/gvr_types.h"

namespace ndk_hello_vr_beta {

/**
 * This is a sample app for the GVR NDK. It loads a simple environment and
 * objects that you can click on.
 */
class HelloVrBetaApp {
 public:
  /**
   * Creates a HelloVrBetaApp using a given |gvr_context|.
   *
   * @param env The JNI environment.
   * @param asset_mgr_obj The asset manager object.
   * @param gvr_api The (non-owned) gvr_context.
   * @param gvr_audio_api The (owned) gvr::AudioApi context.
   */
  HelloVrBetaApp(JNIEnv* env, jobject asset_mgr_obj, gvr_context* gvr_context,
                 std::unique_ptr<gvr::AudioApi> gvr_audio_api);

  ~HelloVrBetaApp();

  /**
   * Initializes any GL-related objects. This should be called on the rendering
   * thread with a valid GL context.
   */
  void OnSurfaceCreated(JNIEnv* env);

  /**
   * Draws the scene. This should be called on the rendering thread.
   */
  void OnDrawFrame();

  /**
   * Pauses head tracking.
   */
  void OnPause();

  /**
   * Resumes head tracking, refreshing viewer parameters if necessary.
   */
  void OnResume();

 private:
  /**
   * Draws all world-space objects.
   */
  void DrawWorld(const gvr::Mat4f view[2], const gvr::Mat4f view_projection[2]);

  /**
   * Draws the target object.
   *
   * We've set all of our transformation matrices. Now we simply pass them
   * into the shader.
   */
  void DrawTarget(const gvr::Mat4f view_projection[2]);

  /**
   * Draws the room.
   */
  void DrawRoom(const gvr::Mat4f view_projection[2]);

  /**
   * Finds a new random position for the target object.
   */
  void GenerateNewTargetPosition();

  /**
   * Checks if user is pointing or looking at the target object by calculating
   * whether the angle between the user's gaze or controller orientation and the
   * vector pointing towards the object is lower than some threshold.
   *
   * @return true if the user is pointing at the target object.
   */
  bool IsPointingAtTarget();

  /**
   * Preloads the target object sound sample and starts the spatialized playback
   * at the current target location. This method is executed from a separate
   * thread to avoid any delay during construction and app initialization.
   */
  void LoadAndPlayTargetObjectSound();

  /**
   * Generate a new target if the controller is pointing at it.
   */
  void OnTrigger(int controller_index);

  /**
   * Grab the target if the controller is pointing at it.
   */
  void OnGrabTarget(int controller_index);

  /**
   * Release the target if we are holding one.
   */
  void OnReleaseTarget(int controller_index);

  /**
   * Controller touchpad is swiped.
   */
  void OnSwipe(int controller_index, gvr_gesture_direction direction);

  bool IsSeeThroughAvailable() const;
  void UpdateSeeThroughSettings();

  void SetTargetPosition(const gvr::Vec3f& position);
  float GetFloorOffset();

  gvr_context* context_;

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  std::unique_ptr<gvr::AudioApi> gvr_audio_api_;
  std::unique_ptr<gvr::BufferViewportList> viewport_list_;
  std::array<gvr::BufferViewport, 2> viewports_;
  std::unique_ptr<gvr::SwapChain> swapchain_;

  gvr_beta_see_through_config* see_through_config_;
  enum SeeThroughMode {
    SHOW_SEE_THROUGH,              // Do not render room.
    SHOW_TRANSLUCENT_SEE_THROUGH,  // Render room translucently.
    NO_SEE_THROUGH                 // Turn off see-through and render room.
  };
  int see_through_mode_;
  int see_through_effect_;

  Controllers controllers_;
  int controller_on_target_index_;
  bool target_held_;

  TexturedMesh room_;
  Texture room_texture_;
  TexturedMesh target_object_mesh_;
  Texture target_object_not_selected_texture_;
  Texture target_object_selected_texture_;

  TexturedShaderProgram shader_;
  TexturedAlphaShaderProgram alpha_shader_;

  gvr::Mat4f model_target_;
  gvr::Sizei render_size_;

  gvr::AudioSourceId audio_source_id_;
  gvr::AudioSourceId success_source_id_;

  std::thread audio_initialization_thread_;

  jobject java_asset_mgr_;
  AAssetManager* asset_mgr_;
};

}  // namespace ndk_hello_vr_beta

#endif  // HELLOVRBETA_APP_SRC_MAIN_JNI_HELLO_VR_BETA_APP_H_  // NOLINT
