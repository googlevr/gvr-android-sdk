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

#ifndef TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTRENDERER_H_  // NOLINT
#define TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTRENDERER_H_  // NOLINT

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_audio.h"
#include "vr/gvr/capi/include/gvr_types.h"
#include "vr/gvr/demos/treasure_hunt/jni/world_layout_data.h"

class TreasureHuntRenderer {
 public:
  /**
   * Create a TreasureHuntRenderer using a given |gvr_context|.
   *
   * @param gvr_api The (non-owned) gvr_context.
   * @param gvr_audio_api The (owned) gvr::AudioApi context.
   */
  TreasureHuntRenderer(gvr_context* gvr_context,
                       std::unique_ptr<gvr::AudioApi> gvr_audio_api);

  /**
   * Destructor.
   */
  ~TreasureHuntRenderer();

  /**
   * Initialize any GL-related objects. This should be called on the rendering
   * thread with a valid GL context.
   */
  void InitializeGl();

  /**
   * Draw the TreasureHunt scene. This should be called on the rendering thread.
   */
  void DrawFrame();

  /**
   * Hide the cube if it's being targeted.
   */
  void OnTriggerEvent();

  /**
   * Pause head tracking.
   */
  void OnPause();

  /**
   * Resume head tracking, refreshing viewer parameters if necessary.
   */
  void OnResume();

 private:
  int CreateTexture(int width, int height, int textureFormat, int textureType);

  /*
   * Prepares the GvrApi framebuffer for rendering, resizing if needed.
   */
  void PrepareFramebuffer();

  /**
   * Converts a raw text file, saved as a resource, into an OpenGL ES shader.
   *
   * @param type  The type of shader we will be creating.
   * @param resId The resource ID of the raw text file.
   * @return The shader object handler.
   */
  int LoadGLShader(int type, const char** shadercode);

  /**
   * Draws all world-space objects for one eye.
   *
   * @param view_matrix View transformation for the current eye.
   * @param viewport The buffer viewport for which we are rendering.
   */
  void DrawWorld(const gvr::Mat4f& view_matrix,
                 const gvr::BufferViewport& viewport);

  /**
   * Draws the reticle for one eye.
   *
   * @param eye_matrix Head-to-eye matrix for the current eye.
   * @param viewport The buffer viewport for which we are rendering.
   */
  void DrawReticle(const gvr::Mat4f& view_matrix,
                   const gvr::BufferViewport& viewport);

  /**
   * Draw the cube.
   *
   * We've set all of our transformation matrices. Now we simply pass them
   * into the shader.
   */
  void DrawCube();

  /**
   * Draw the floor.
   *
   * This feeds in data for the floor into the shader. Note that this doesn't
   * feed in data about position of the light, so if we rewrite our code to
   * draw the floor first, the lighting might look strange.
   */
  void DrawFloor();

  /**
   * Find a new random position for the object.
   *
   * We'll rotate it around the Y-axis so it's out of sight, and then up or
   * down by a little bit.
   */
  void HideObject();

  /**
   * Check if user is looking at object by calculating where the object is
   * in eye-space.
   *
   * @return true if the user is looking at the object.
   */
  bool IsLookingAtObject();

  /**
   * Preloads the cube sound sample and starts the spatialized playback at the
   * current cube location. This method is executed from a separate thread to
   * avoid any delay during construction and app initialization.
   */
  void LoadAndPlayCubeSound();

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  std::unique_ptr<gvr::AudioApi> gvr_audio_api_;
  std::unique_ptr<gvr::BufferViewportList> viewport_list_;
  std::unique_ptr<gvr::SwapChain> swapchain_;
  gvr::BufferViewport scratch_viewport_;

  std::vector<float> lightpos_;

  WorldLayoutData world_layout_data_;
  float* floor_vertices_;
  float* cube_vertices_;
  float* cube_colors_;
  float* cube_normals_;
  float* reticle_vertices_;

  int cube_program_;
  int floor_program_;
  int reticle_program_;
  int cube_position_param_;
  int cube_normal_param_;
  int cube_color_param_;
  int cube_model_param_;
  int cube_modelview_param_;
  int cube_modelview_projection_param_;
  int cube_light_pos_param_;
  int floor_position_param_;
  int floor_normal_param_;
  int floor_color_param_;
  int floor_model_param_;
  int floor_modelview_param_;
  int floor_modelview_projection_param_;
  int floor_light_pos_param_;
  int reticle_position_param_;
  int reticle_modelview_projection_param_;

  std::array<float, 4> light_pos_world_space_;
  std::array<float, 4> light_pos_eye_space_;
  gvr::Mat4f head_view_;
  gvr::Mat4f model_cube_;
  gvr::Mat4f camera_;
  gvr::Mat4f view_;
  gvr::Mat4f modelview_projection_cube_;
  gvr::Mat4f modelview_projection_floor_;
  gvr::Mat4f modelview_;
  gvr::Mat4f model_floor_;
  gvr::Mat4f model_reticle_;
  gvr::Sizei render_size_;
  gvr::Sizei reticle_render_size_;

  int score_;
  float object_distance_;
  float reticle_distance_;
  float floor_depth_;

  gvr::AudioSourceId audio_source_id_;

  gvr::AudioSourceId success_source_id_;

  std::thread audio_initialization_thread_;
};

#endif  // TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTRENDERER_H_  // NOLINT
