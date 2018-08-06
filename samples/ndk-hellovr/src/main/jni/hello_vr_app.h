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

#ifndef HELLOVR_APP_SRC_MAIN_JNI_HELLO_VR_APP_H_  // NOLINT
#define HELLOVR_APP_SRC_MAIN_JNI_HELLO_VR_APP_H_  // NOLINT

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/asset_manager.h>
#include <jni.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "util.h"  // NOLINT
#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_audio.h"
#include "vr/gvr/capi/include/gvr_controller.h"
#include "vr/gvr/capi/include/gvr_types.h"

namespace ndk_hello_vr {

/**
 * This is a sample app for the GVR NDK. It loads a simple environment and
 * objects that you can click on.
 */
class HelloVrApp {
 public:
  /**
   * Creates a HelloVrApp using a given |gvr_context|.
   *
   * @param env The JNI environment.
   * @param asset_mgr_obj The asset manager object.
   * @param gvr_api The (non-owned) gvr_context.
   * @param gvr_audio_api The (owned) gvr::AudioApi context.
   */
  HelloVrApp(JNIEnv* env, jobject asset_mgr_obj, gvr_context* gvr_context,
             std::unique_ptr<gvr::AudioApi> gvr_audio_api);

  ~HelloVrApp();

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
   * Hides the target object if it's being targeted.
   */
  void OnTriggerEvent();

  /**
   * Pauses head tracking.
   */
  void OnPause();

  /**
   * Resumes head tracking, refreshing viewer parameters if necessary.
   */
  void OnResume();

 private:
  int CreateTexture(int width, int height, int textureFormat, int textureType);

  /*
   * Prepares the GvrApi framebuffer for rendering, resizing if needed.
   */
  void PrepareFramebuffer();

  enum ViewType { kLeftView, kRightView, kMultiview };

  /**
   * Draws all world-space objects for the given view type.
   *
   * @param view Specifies which view we are rendering.
   */
  void DrawWorld(ViewType view);

  /**
   * Draws the reticle. The reticle is positioned using viewport parameters,
   * so no data about its eye-space position is needed here.
   */
  void DrawReticle();

  /**
   * Draws the target object.
   *
   * We've set all of our transformation matrices. Now we simply pass them
   * into the shader.
   *
   * @param view Specifies which eye we are rendering: left, right, or both.
   */
  void DrawTarget(ViewType view);

  /**
   * Draws the room.
   *
   * @param view Specifies which eye we are rendering: left, right, or both.
   */
  void DrawRoom(ViewType view);

  /**
   * Draws the safety ring.
   *
   * @param view Specifies which eye we are rendering: left, right, or both.
   */
  void DrawSafetyRing(ViewType view);

  /**
   * Finds a new random position for the target object.
   */
  void HideTarget();

  /**
   * Updates the position of the reticle based on controller data.
   * In Cardboard mode, this function simply sets the position to the center
   * of the view.
   */
  void UpdateReticlePosition();

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
   * Processes the controller input.
   *
   * The controller state is updated with the latest touch pad, button clicking
   * information, connection state and status of the controller. A log message
   * is reported if the controller status or connection state changes. A click
   * event is triggered if a click on app/click button is detected.
   */
  void ProcessControllerInput();

  /*
   * Resumes the controller api if needed.
   *
   * If the viewer type is cardboard, set the controller api pointer to null.
   * If the viewer type is daydream, initialize the controller api as needed and
   * resume.
   */
  void ResumeControllerApiAsNeeded();

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  std::unique_ptr<gvr::AudioApi> gvr_audio_api_;
  std::unique_ptr<gvr::BufferViewportList> viewport_list_;
  std::unique_ptr<gvr::SwapChain> swapchain_;
  gvr::BufferViewport viewport_left_;
  gvr::BufferViewport viewport_right_;

  std::vector<float> lightpos_;

  const std::array<float, 18> reticle_coords_;

  TexturedMesh room_;
  Texture room_tex_;

  TexturedMesh safety_ring_;
  Texture safety_ring_tex_;

  std::vector<TexturedMesh> target_object_meshes_;
  std::vector<Texture> target_object_not_selected_textures_;
  std::vector<Texture> target_object_selected_textures_;
  int cur_target_object_;

  int reticle_program_;
  GLuint obj_program_;

  GLuint obj_position_param_;
  GLuint obj_uv_param_;
  GLuint obj_modelview_projection_param_;

  int reticle_position_param_;
  int reticle_modelview_projection_param_;

  const gvr::Sizei reticle_render_size_;

  gvr::Mat4f head_view_;
  gvr::Mat4f model_target_;
  gvr::Mat4f camera_;
  gvr::Mat4f view_;
  gvr::Mat4f model_reticle_;
  gvr::Mat4f modelview_reticle_;
  gvr::Sizei render_size_;

  // View-dependent values.  These are stored in length two arrays to allow
  // syncing with uniforms consumed by the multiview vertex shader.  For
  // simplicity, we stash valid values in both elements (left, right) of these
  // arrays even when multiview is disabled.
  gvr::Mat4f modelview_projection_target_[2];
  gvr::Mat4f modelview_projection_room_[2];
  gvr::Mat4f modelview_projection_safety_ring_[2];
  gvr::Mat4f modelview_target_[2];

  float reticle_distance_;
  bool multiview_enabled_;

  gvr::AudioSourceId audio_source_id_;

  gvr::AudioSourceId success_source_id_;

  std::thread audio_initialization_thread_;

  // Controller API entry point.
  std::unique_ptr<gvr::ControllerApi> gvr_controller_api_;

  // The latest controller state (updated once per frame).
  gvr::ControllerState gvr_controller_state_;

  gvr::ViewerType gvr_viewer_type_;

  jobject java_asset_mgr_;
  AAssetManager* asset_mgr_;
};

}  // namespace ndk_hello_vr

#endif  // HELLOVR_APP_SRC_MAIN_JNI_HELLO_VR_APP_H_  // NOLINT
