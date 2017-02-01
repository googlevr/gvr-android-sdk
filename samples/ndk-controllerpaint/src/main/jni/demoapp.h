/*
 * Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CONTROLLER_PAINT_APP_SRC_MAIN_JNI_DEMOAPP_H_  // NOLINT
#define CONTROLLER_PAINT_APP_SRC_MAIN_JNI_DEMOAPP_H_

#include <android/asset_manager.h>
#include <GLES2/gl2.h>
#include <jni.h>

#include <array>
#include <chrono>  // NOLINT
#include <memory>
#include <vector>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_controller.h"

// This demo app is a "paint program" that allows the user to paint in
// virtual space using the controller. A cursor shows where the controller
// is pointing at. Touching or clicking the touchpad begins drawing.
// Then, as the user moves their hand, lines are drawn. The user can switch
// the drawing color by swiping to the right or left on the touchpad.
// The user can also change the drawing stroke width by moving their finger
// up and down on the touchpad.
//
// This demo uses GvrApi and ControllerApi and shows how to use
// them together.
class DemoApp {
 public:
  // Initializes the demo app.
  // |asset_manager| is the Android Asset Manager obtained from Java.
  // |gvr_context_ptr| a jlong representing a pointer to the GVR context
  //     obtained from Java.
  DemoApp(JNIEnv* env, jobject asset_manager, jlong gvr_context_ptr);
  ~DemoApp();
  // Must be called when the Activity gets onResume().
  // Must be called on the UI thread.
  void OnResume();
  // Must be called when the Activity gets onPause().
  // Must be called on the UI thread.
  void OnPause();
  // Must be called when the GL renderer gets onSurfaceCreated().
  // Must be called on the rendering thread.
  void OnSurfaceCreated();
  // Must be called when the GL renderer gets onSurfaceChanged().
  // Must be called on the rendering thread.
  void OnSurfaceChanged(int width, int height);
  // Must be called when the GL renderer gets onDrawFrame().
  // Must be called on the rendering thread.
  void OnDrawFrame();

 private:
  // Quick explanation of the implementation:
  //
  // When the user paints, we generate geometry (a series of connected
  // triangles).
  //
  // When the user starts painting, we accumulate the new geometry
  // (vertices and texture coordinates) in the |recent_geom_| array.
  // When that gets too crowded (exceeds a threshold number of vertices),
  // we commit the geometry to the GPU using a VBO (Vertex Buffer Object).
  // From then on, that piece of geometry resides in the GPU and can be
  // rendered quickly without us needing to push it down the bus from
  // CPU to GPU on every frame.

  // Prepares the GvrApi framebuffer for rendering, resizing if needed.
  void PrepareFramebuffer();

  // Draws the image for the indicated eye.
  void DrawEye(gvr::Eye which_eye, const gvr::Mat4f& eye_view_matrix,
               const gvr::BufferViewport& params);

  // Draws the ground plane below the player.
  void DrawGround(const gvr::Mat4f& view_matrix, const gvr::Mat4f& proj_matrix);

  // Draws the cursor that indicates where the controller is pointing.
  // This method obtains the current cursor orientation from
  // |controller_state_|, which is assumed to be up to date.
  void DrawCursor(const gvr::Mat4f& view_matrix, const gvr::Mat4f& proj_matrix);

  // Draws a rectangle. The cursor is made of several rectangles.
  void DrawCursorRect(float scale, const std::array<float, 4> color,
                      const gvr::Mat4f& view_matrix,
                      const gvr::Mat4f& proj_matrix);

  // Adds a new segment to the geometry currently being drawn. The new
  // segment will be created in such a way that is connects to the
  // last created paint segment to produce the effect of a continuous
  // brush stroke.
  void AddPaintSegment(const std::array<float, 3>& start_point,
                       const std::array<float, 3>& end_point);

  // Starts painting. This means that as the cursor moves, new geometry
  // will be created to represent the brush stroke.
  void StartPainting(const std::array<float, 3> paint_start_pos);

  // Stops painting. This means that the cursor's motion will cease to
  // create new geometry.
  void StopPainting(bool commit_cur_segment);

  // Adds a single vertex to the geometry.
  void AddVertex(const std::array<float, 3>& coords, float u, float v);

  // Renders all the geometry the user painted, including the recent
  // uncommitted geometry and the committed VBOs.
  void DrawPaintedGeometry(const gvr::Mat4f& view_matrix,
                           const gvr::Mat4f& proj_matrix);

  // Pushes the current geometry to the GPU in the form of a VBO. This
  // does not mean painting needs to stop: it just offloads vertices
  // to the GPU for performance. If painting was active, it continues
  // normally.
  void CommitToVbo();

  // Draws a single object, which may have its geometry specified via a regular
  // pointer, or as a VBO handle.
  //
  // @param mvp The model-view-projection matrix to use.
  // @param color The color to use.
  // @param data If non-NULL, points to the data to draw.
  //     If this is NULL, then this method will use a VBO to draw.
  // @param vbo If data == NULL, this is the VBO to use.
  // @param vertex_count The number of vertices to draw.
  void DrawObject(const gvr::Mat4f& mvp, const std::array<float, 4>& color,
                  const float* data, GLuint vbo, int vertex_count);

  // Checks if the user performed the "switch color" gesture and switches
  // color, if applicable.
  void CheckColorSwitch();

  // Checks if the user wants to change the stroke width.
  void CheckChangeStrokeWidth();

  // Clears the whole drawing.
  void ClearDrawing();

  // Gvr API entry point.
  gvr_context* gvr_context_;
  std::unique_ptr<gvr::GvrApi> gvr_api_;
  bool gvr_api_initialized_;

  // Controller API entry point.
  std::unique_ptr<gvr::ControllerApi> controller_api_;

  // Handle to the swapchain. On every frame, we have to check if the buffers
  // are still the right size for the frame (since they can be resized at any
  // time). This is done by PrepareFramebuffer().
  std::unique_ptr<gvr::SwapChain> swapchain_;

  // List of rendering params (used to render each eye).
  gvr::BufferViewportList viewport_list_;
  gvr::BufferViewport scratch_viewport_;

  // Size of the offscreen framebuffer.
  gvr::Sizei framebuf_size_;

  // The shader we use to render our geometry. Since this is a very simple
  // demo, we use only one shader.
  int shader_;

  // Uniform/attrib locations in the shader. These are looked up after we
  // compile/link the shader.
  int shader_u_color_;
  int shader_u_mvp_matrix_;  // Model-view-projection matrix.
  int shader_u_sampler_;
  int shader_a_position_;
  int shader_a_texcoords_;

  // Ground texture.
  int ground_texture_;

  // Paint texture. This is the texture we use for painting.
  int paint_texture_;

  // Android asset manager (we use it to load the texture).
  AAssetManager* asset_mgr_;

  // The last controller state (updated once per frame).
  gvr::ControllerState controller_state_;

  // The vertex and texture coordinates representing recently painted geometry.
  // As this array grows beyond a certain limit, we commit that geometry
  // to a VBO for performance. This is formatted for rendering, with
  // each group of 5 floats meaning vx, vy, vz, s, t, where (vx, vy, vz) are the
  // vertex coordinates in world space and s,t are the texture coordinates.
  std::vector<float> recent_geom_;

  // Count of vertices in recent_geom_.
  int recent_geom_vertex_count_;

  // Total vertices in the current brush stroke (the brush
  // stroke starts when the user first touches the touchpad and continues
  // until they release it).
  int brush_stroke_total_vertices_;

  // Currently selected color (index).
  int selected_color_;

  // If true, we are currently painting.
  bool painting_;

  // If painting_ == true, then this is the position where the last
  // paint segment ended (or the position where painting began, if no
  // segments were added yet).
  std::array<float, 3> paint_anchor_;

  // Indicates whether we have continuation points to continue the shape
  // from (for smooth drawing).
  bool has_continuation_;

  // If has_continuation_ == true, these are the continuation points.
  std::array<std::array<float, 3>, 2> continuation_points_;

  // This is the list of committed VBOs that contains the static parts
  // of the current drawing. As the drawing accumulates in painted_geom_,
  // we push it to a static VBO on the GPU for performance.
  struct VboInfo {
    GLuint vbo;
    int vertex_count;
    int color;
  };
  std::vector<VboInfo> committed_vbos_;

  // Touchpad coordinates where touch started. We use this to detect
  // the swipe gestures that cause the drawing color to change.
  float touch_down_x_;
  float touch_down_y_;

  // If true, a color switch already happened during this touch cycle.
  bool switched_color_;

  // Width of the painting stroke.
  float stroke_width_;

  // Width of the painting stroke when the user last started touching the
  // touchpad.
  float touch_down_stroke_width_;

  // Disallow copy and assign.
  DemoApp(const DemoApp& other) = delete;
  DemoApp& operator=(const DemoApp& other) = delete;
};

#endif  // CONTROLLER_PAINT_APP_SRC_MAIN_JNI_DEMOAPP_H_  // NOLINT
