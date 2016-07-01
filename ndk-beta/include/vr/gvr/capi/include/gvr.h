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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_H_
#define VR_GVR_CAPI_INCLUDE_GVR_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <array>
#include <memory>
#endif

#include "vr/gvr/capi/include/gvr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup Base Google VR Base C API
/// @brief This is the Google VR C API. It supports clients writing VR
/// experiences for head mounted displays that consist of a mobile phone and a
/// VR viewer.
///
/// Example API usage:
///
///     #ifdef __ANDROID__
///     // On Android, the gvr_context should almost always be obtained from
///     // the Java GvrLayout object via
///     // GvrLayout.getGvrApi().getNativeGvrContext().
///     gvr_context* gvr = ...;
///     #else
///     gvr_context* gvr = gvr_create();
///     #endif
///
///     gvr_initialize_gl(gvr);
///
///     gvr_render_params_list* gvr_params_list =
///         gvr_create_empty_render_params_list(gvr);
///     gvr_get_recommended_render_params_list(gvr, gvr_params_list);
///
///     while (client_app_should_render) {
///       // A client app should be ready for the render target size to change
///       // whenever a new QR code is scanned, or a new viewer is paired.
///       gvr_sizei render_target_size =
///           gvr_get_recommended_render_target_size(gvr);
///       int offscreen_texture_id =
///           AppInitializeOrResizeOffscreenBuffer(render_target_size);
///
///       // This function will depend on your render loop's implementation.
///       gvr_clock_time_point next_vsync = AppGetNextVsyncTime();
///
///       const gvr_head_pose head_pose =
///           gvr_get_head_pose_in_start_space(gvr, next_vsync);
///       const gvr_mat4f head_matrix = head_pose.object_from_reference_matrix;
///       const gvr_mat4f left_eye_matrix = MatrixMultiply(
///           gvr_get_eye_from_head_matrix(gvr, GVR_LEFT_EYE), head_matrix);
///       const gvr::Mat4f right_eye_matrix = MatrixMultiply(
///           gvr_get_eye_from_head_matrix(gvr, GVR_RIGHT_EYE), head_matrix);
///
///       // Insert client rendering code here.
///
///       AppSetRenderTarget(offscreen_texture_id);
///
///       AppDoSomeRenderingForEye(
///           left_eye_params.eye_viewport_bounds, left_eye_matrix);
///       AppDoSomeRenderingForEye(
///           right_eye_params.eye_viewport_bounds, right_eye_matrix);
///       AppSetRenderTarget(primary_display);
///
///       gvr_distort_to_screen(gvr, frame_texture_id, render_params_list,
///                             &head_pose, &next_vsync);
///       AppSwapBuffers();
///     }
///
///     #ifdef __ANDROID__
///     // On Android, The Java GvrLayout owns the gvr_context.
///     #else
///     gvr_destroy(gvr);
///     #endif
///
/// Head tracking is enabled by default, and will begin as soon as the
/// gvr_context is created. The client should call gvr_pause_tracking() and
/// gvr_resume_tracking() when the app is paused and resumed, respectively.
///
/// Note: Unless otherwise noted, the methods on this class may not be
/// thread-safe with respect to the gvr_context, and it is up the caller to use
/// the API in a thread-safe manner.
///
/// @{

/// Creates a new gvr instance.
///
/// Warning: On Android, the gvr_context should *almost always* be obtained from
/// the Java GvrLayout object, rather than explicitly creaed here. The GvrLayout
/// should live in the app's View hierarchy, and its use is required to ensure
/// consistent behavior across all varieties of GVR-compatible viewers. See
/// the Java GvrLayout and GvrApi documentation for more details.
///
#ifdef __ANDROID__
/// @param env The JNIEnv associated with the current thread.
/// @param app_context The Android application context. This must be the
///     application context, NOT an Activity context (Note: from any Android
///     Activity in your app, you can call getApplicationContext() to
///     retrieve the application context).
/// @param class_loader The class loader to use when loading Java classes.
///     This must be your app's main class loader (usually accessible through
///     activity.getClassLoader() on any of your Activities).
///
/// @return Pointer to the created gvr instance, nullptr on failure.
gvr_context* gvr_create(JNIEnv* env, jobject context, jobject class_loader);
#else
/// @return Pointer to the created gvr instance, nullptr on failure.
gvr_context* gvr_create();
#endif  // #ifdef __ANDROID__

/// Get a string representing the GVR release version.  This is the version of
/// the GVR DSDK, not the version of this API.  It is in the form MAJOR.MINOR,
/// e.g., "0.1", "1.2" etc.
///
/// @return The version as a static char pointer.
const char* gvr_get_release_version();

/// Destroys a gvr_context instance.  The parameter will be nulled by this
/// operation.
///
/// @param gvr Pointer to a pointer to the gvr instance to be destroyed and
///     nulled.
void gvr_destroy(gvr_context** gvr);

/// Initializes necessary GL-related objects and uses the current thread and
/// GL context for rendering. Please make sure that a valid GL context is
/// available when this function is called.
///
/// @param gvr Pointer to the gvr instance to be initialised.
void gvr_initialize_gl(gvr_context* gvr);

/// Creates a new, empty list of render parameters. Render parameters
/// are used internally to inform distortion rendering, and are a function
/// of intrinisic screen and viewer parameters.
///
/// The caller should populate the returned params using one of:
///   - gvr_get_recommended_render_params_list()
///   - gvr_get_screen_render_params_list()
///   - gvr_set_render_params()
///
/// @param gvr Pointer the gvr instance from which to allocate the params list.
///
/// @return A handle to an allocated gvr_render_params_list. The caller is
///     responsible for calling destroy on the returned handle.
gvr_render_params_list* gvr_create_render_params_list(gvr_context* gvr);

/// Destroys a gvr_render_params_list instance. The parameter will be nulled
/// by this operation.
///
/// @param params_list Pointer to a pointer to the params list instance to be
///     destroyed and nulled.
void gvr_destroy_render_params_list(gvr_render_params_list** params_list);

/// Returns the size of a given render params list.
///
/// @param params_list Pointer to a render params list.
///
/// @return The number of entries in the params list.
size_t gvr_get_render_params_list_size(
    const gvr_render_params_list* params_list);

/// Gets the recommended render params, populating a previously allocated
/// gvr_render_params_list object. The updated values include the per-eye
/// recommended viewport and field of view for the target.
///
/// When the recommended params are used for distortion rendering, this method
/// should always be called after calling refresh_viewer_profile(). That will
/// ensure that the populated params reflect the currently paired viewer.
///
/// @param gvr Pointer to the gvr instance from which to get the params.
/// @param params_list Pointer to a previously allocated params list. This
///     will be populated with the recommended render params and resized if
///     necessary.
void gvr_get_recommended_render_params_list(
    const gvr_context* gvr, gvr_render_params_list* params_list);

/// Gets the screen (non-distorted) render params, populating a previously
/// allocated gvr_render_params_list object. The updated values include the
/// per-eye recommended viewport and field of view for the target.
///
/// @param gvr Pointer to the gvr instance from which to get the params.
/// @param params_list Pointer to a previously allocated params list. This will
///     be populated with the screen render params and resized if necessary.
void gvr_get_screen_render_params_list(const gvr_context* gvr,
                                       gvr_render_params_list* params_list);

/// Returns a recommended size for the client's render target, given the
/// parameters of the head mounted device selected.
///
/// @param gvr Pointer to the gvr instance from which to get the size.
///
/// @return Recommended size for the target render target.
gvr_sizei gvr_get_recommended_render_target_size(const gvr_context* gvr);

/// Returns a non-distorted size for the screen, given the parameters
/// of the phone and/or the head mounted device selected.
///
/// @param gvr Pointer to the gvr instance from which to get the size.
///
/// @return Screen (non-distorted) size for the render target.
gvr_sizei gvr_get_screen_target_size(const gvr_context* gvr);

/// Distorts the target that is passed in based on the characteristics of
/// the lenses -- as specified by user's most recently scanned QR code --
/// and renders it to the screen. It performs post-frame operations like
/// distortion correction and more.  Should be called after the app renders its
/// frame. Note that the target that is referenced by the `target_id` will be
/// post-processed to the display, and the original target will remain
/// unmodified.
///
/// WARNING: initialize_gl() must have been called before calling this method.
///
/// @param gvr Pointer to the gvr instance which will do the distortion.
/// @param target_id The OpenGL target ID of the target into which the frame
///     was rendered by the app. The original target referenced by the
///     target_id will remain unmodified.
/// @param params_list The render target parameters to be distorted.
/// @param rendered_head_pose_in_start_space An optional pointer to the head
///     pose in Start Space that was used to render input_target.  If not
///     null, this pose may be used for EDS (electronic display
///     stabilization).  If this or target_presentation_time is null then EDS
///     is not applied to the frame.
/// @param target_presentation_time An optional pointer to the expected time
///     that this frame will appear on the screen. If this or
///     rendered_head_pose_in_start_space is null then EDS is not applied to
///     the frame.
void gvr_distort_to_screen(
    gvr_context* gvr, int32_t target_id,
    const gvr_render_params_list* params_list,
    const gvr_head_pose* rendered_head_pose_in_start_space,
    const gvr_clock_time_point* target_presentation_time);

/// Distorts the color target that is passed in based on the characteristics of
/// the lenses, and renders it to the display. It performs post-frame
/// operations like distortion correction and more. Should be called after the
/// app renders its frame. Note that the contents of the passed offscreen
/// framebuffer are not modified.
///
/// WARNING: initialize_gl() must have been called before calling this method.
///
/// @param gvr Pointer to the gvr instance which will do the distortion.
/// @param offscreen_fbo_handle A handle to an offscreen framebuffer object that
///     contains the render targets used by the app when drawing frames. The
///     contents of the render targets is not modified.
/// @param params_list The render target parameters to be distorted.
/// @param rendered_head_pose_in_start_space An optional pointer to the head
///     pose in Start Space that was used to render input_texture (this
///     is typically the pose returned by GetHeadPoseForNextFrame()).  If
///     not null, this pose may be used for EDS (electronic display
///     stabilization).  If this or texture_presentation_time is null then EDS
///     is not applied to the frame.
/// @param target_presentation_time An optional pointer to the expected time
///     that this frame will appear on the screen. If this or
///     rendered_head_pose_in_start_space is null then EDS is not applied to
///     the frame.
void gvr_distort_offscreen_framebuffer_to_screen(
    gvr_context* gvr, int32_t offscreen_fbo_handle,
    const gvr_render_params_list* params_list,
    const gvr_head_pose* rendered_head_pose_in_start_space,
    const gvr_clock_time_point* target_presentation_time);

/// Updates a render params entry from the given eye parameters. Data from the
/// device -- as well as the provided parameters -- will be used to populate
/// the internal distortion params, which are used for rendering the distortion.
///
/// Note: This method should be used rarely and only for custom, non-standard
/// rendering flows. The typical flow will use get_recommended_render_params
/// to populate the necessary values for distortion rendering.
///
/// @param gvr Pointer to a gvr instance from which to create the params.
/// @param params_list Pointer to a previously allocated params list.
/// @param index Index of the render params entry to update. If the
///     `params_list` size is equal to the index, a new params entry will be
///     added. The `params_list` size must *not* be less than the index value.
/// @param render_params A pointer to the eye params object.
void gvr_set_render_params(const gvr_context* gvr,
                           gvr_render_params_list* params_list, size_t index,
                           const gvr_render_params* render_params);

/// Returns the render params from a given render params list entry. This
/// includes the eye viewport bounds, the fov, and the eye type.
///
/// @param params_list Pointer to the previously allocated params list.
/// @param index Index of the render params entry to query. The `params_list`
///    size must be greater than the provided (0-based) index.
///
/// @return The render params.
gvr_render_params gvr_get_render_params(
    const gvr_render_params_list* params_list, size_t index);

/// @}

/////////////////////////////////////////////////////////////////////////////
// Offscreen Framebuffers
/////////////////////////////////////////////////////////////////////////////
/// @defgroup framebuffers Offscreen Framebuffers
/// @brief Functions to create, access, and modify offscreen framebuffers.
/// @{

/// Creates a default framebuffer specification.
gvr_framebuffer_spec* gvr_framebuffer_spec_create(gvr_context* gvr);

/// Gets the size of the framebuffer to be created.
/// @param spec Framebuffer specification.
/// @return Size of the framebuffer. The default is equal to the recommended
///     render target size at the time when the specification was created.
gvr_sizei gvr_framebuffer_spec_get_size(const gvr_framebuffer_spec* spec);

/// Sets the size of the framebuffer to be created.
/// @param spec Framebuffer specification.
/// @param size The size. Width and height must both be greater than zero.
void gvr_framebuffer_spec_set_size(gvr_framebuffer_spec* spec, gvr_sizei size);

/// Gets the number of samples per pixel in the framebuffer to be created.
/// @param spec Framebuffer specification.
/// @return Value >= 1 giving the number of samples. 1 means multisampling is
///     disabled. Negative values and 0 are never returned.
int gvr_framebuffer_spec_get_samples(const gvr_framebuffer_spec* spec);

/// Sets the number of samples per pixel in the framebuffer to be created.
/// @param spec Framebuffer specification.
/// @param num_samples The number of samples. Negative values are an error.
///     The values 0 and 1 are treated identically and indicate that
//      multisampling should be disabled.
void gvr_framebuffer_spec_set_samples(gvr_framebuffer_spec* spec,
                                      int num_samples);

/// Destroy the framebuffer specification and null the pointer.
void gvr_framebuffer_spec_destroy(gvr_framebuffer_spec** spec);

/// Creates an offscreen framebuffer and adds it to the pool of managed
/// offscreen framebuffers.
///
/// Note that, for scanline-racing applications, only one offscreen
/// framebuffer can be created.
///
/// WARNING: Framebuffer creation and usage should occur only *after*
/// initialize_gl() has been called on `gvr`.
///
/// @param gvr Pointer to the gvr instance that will do the creating.
/// @param spec Framebuffer creation specification.
/// @return Handle to the newly created offscreen framebuffer.
int32_t gvr_create_offscreen_framebuffer(gvr_context* gvr,
                                         const gvr_framebuffer_spec* spec);

/// Releases the specified offscreen framebuffer from the pool of managed
/// offscreen framebuffers and deletes it.
///
/// @param gvr Pointer to the gvr instance that will do the releasing.
/// @param framebuffer_handle The handle of the offscreen framebuffer to delete.
void gvr_release_offscreen_framebuffer(gvr_context* gvr,
                                       int32_t framebuffer_handle);

/// Sets the specified offscreen framebuffer to be active. Binds
/// the offscreen framebuffer to the renderer.
///
/// @param gvr Pointer to the gvr instance that the frame buffer will become
///     active for.
/// @param framebuffer_handle The handle of the offscreen framebuffer to set
///     as active.
void gvr_set_active_offscreen_framebuffer(gvr_context* gvr,
                                          int32_t framebuffer_handle);

/// Sets the default offscreen framebuffer to be active. Binds the default
/// offscreen framebuffer to the renderer.
void gvr_set_default_framebuffer_active(gvr_context* gvr);

/// Resizes the managed offscreen framebuffer associated with the given handle.
///
/// @param gvr Pointer to the gvr instance that will do the resizing.
/// @param framebuffer_handle The handle of the offscreen framebuffer to
///     resize.
/// @param framebuffer_size The desired size of the offscreen framebuffer.
void gvr_resize_offscreen_framebuffer(gvr_context* gvr,
                                      int32_t framebuffer_handle,
                                      gvr_sizei framebuffer_size);

/// Gets the size of the specified offscreen framebuffer.
///
/// @param gvr Pointer to the gvr instance that will report the size.
/// @param framebuffer_handle The handle of the offscreen framebuffer to get the
///      size of.
/// @return The size of the offscreen framebuffer. Returns size (0, 0) if no
///      offscreen framebuffer mapping to framebuffer_handle is found.
gvr_sizei gvr_get_offscreen_framebuffer_size(const gvr_context* gvr,
                                             int32_t framebuffer_handle);

/// Warning: EXPERIMENTAL -- USE AT YOUR OWN RISK
///
/// Gets the resource id (i.e., GL frame buffer id) for the specified
/// OffscreenFramebuffer.
///
/// Note that, when scanline racing, the resource associated with a framebuffer
/// handle will become invalid after each call to
/// gvr_distort_offscreen_framebuffer_to_screen(), and this method will not be
/// functional until gvr_set_active_offscreen_framebuffer() is called again.
///
/// For the non-scanline racing case, the resource id can be retrieved as soon
/// as the handle is created.
///
/// @param gvr Pointer to the gvr instance that will return the resource id.
/// @param framebuffer_handle The handle of the offscreen framebuffer to get
///      the resource id of.
/// @return The resource id of the offscreen framebuffer. Returns 0 if no
///      offscreen framebuffer mapping to framebuffer_handle is found.
int32_t gvr_get_offscreen_framebuffer_resource_id(const gvr_context* gvr,
                                                  int32_t framebuffer_handle);

/// @}

/////////////////////////////////////////////////////////////////////////////
// Head tracking
/////////////////////////////////////////////////////////////////////////////
/// @defgroup Headtracking Head tracking
/// @brief Functions for managing head tracking.
/// @{

/// Gets the predicted head's pose in Start Space at given time.
///
/// @param gvr Pointer to the gvr instance from which to get the pose.
/// @param time The time at which to get the head pose. The time should be in
///     the future.
// TODO(b/27951372): What happens if this time is not in the future?
/// @return Head pose in Start Space.
gvr_head_pose gvr_get_head_pose_in_start_space(const gvr_context* gvr,
                                               gvr_clock_time_point time);

/// Pauses head tracking, disables all sensors (to save power).
///
/// @param gvr Pointer to the gvr instance for which tracking will be paused and
///     sensors disabled.
void gvr_pause_tracking(gvr_context* gvr);

/// Resumes head tracking, re-enables all sensors.
///
/// @param gvr Pointer to the gvr instance for which tracking will be resumed.
void gvr_resume_tracking(gvr_context* gvr);

/// Resets head tracking. Warning ResetTracking() wipes out all the state,
/// so RecenterTracking() should be used instead in most cases.
///
/// @param gvr Pointer to the gvr instance for which tracking will be reseted.
void gvr_reset_tracking(gvr_context* gvr);

/// Recenters the head orientation (resets the yaw to zero, leaving pitch and
/// roll unmodified).
///
/// @param gvr Pointer to the gvr instance for which tracking will be
///     recentered.
void gvr_recenter_tracking(gvr_context* gvr);

/// @}


/////////////////////////////////////////////////////////////////////////////
// Head mounted display.
/////////////////////////////////////////////////////////////////////////////
/// @defgroup HMD Head Mounted Display
/// @brief Functions for managing viewer information.
/// @{

/// Sets the default viewer profile specified by viewer_profile_uri.
/// The viewer_profile_uri that is passed in will be ignored if a valid
/// viewer profile has already been stored on the device that the app
/// is running on.
///
/// Note: This function has the potential of blocking for up to 30 seconds for
/// each redirect if a shortened URI is passed in as argument. It will try to
/// unroll the shortened URI for a maximum number of 5 times if the redirect
/// continues. In that case, it is recommended to create a separate thread to
/// call this function so that other tasks like rendering will not be blocked
/// on this. The blocking can be avoided if a standard URI is passed in.
///
/// @param gvr Pointer to the gvr instance which to set the profile on.
/// @param viewer_profile_uri A string that contains either the shortened URI or
///     the standard URI representing the viewer profile that the app should be
///     using. If the valid viewer profile can be found on the device, the URI
///     that is passed in will be ignored and nothing will happen. Otherwise,
///     gvr will look for the viewer profile specified by viewer_profile_uri,
///     and it will be stored if found. Also, the values will be applied to gvr.
///     A valid standard URI can be generated from this page:
///     https://www.google.com/get/cardboard/viewerprofilegenerator/
/// @return True if the viewer profile specified by viewer_profile_uri was
///     successfully stored and applied, false otherwise.
bool gvr_set_default_viewer_profile(gvr_context* gvr,
                                    const char* viewer_profile_uri);

/// Refreshes gvr_context with the viewer profile that is stored on the device.
/// If it can not find the viewer profile, nothing will happen.
///
/// @param gvr Pointer to the gvr instance to refresh the profile on.
void gvr_refresh_viewer_profile(gvr_context* gvr);

/// Gets the name of the viewer vendor.
///
/// @param gvr Pointer to the gvr instance from which to get the vendor.
/// @return A pointer to the vendor name. May be null if no viewer is paired.
///     WARNING: This method guarantees the validity of the returned pointer
///     only until the next use of the `gvr` context. The string should be
///     copied immediately if persistence is required.
const char* gvr_get_viewer_vendor(const gvr_context* gvr);

/// Gets the name of the viewer model.
///
/// @param gvr Pointer to the gvr instance from which to get the name.
/// @return A pointer to the model name. May be null if no viewer is paired.
///     WARNING: This method guarantees the validity of the returned pointer
///     only until the next use of the `gvr` context. The string should be
///     copied immediately if persistence is required.
const char* gvr_get_viewer_model(const gvr_context* gvr);

/// Gets the transformation matrix to convert from Head Space to Eye Space for
/// the given eye.
///
/// @param gvr Pointer to the gvr instance from which to get the matrix.
/// @param eye Selected eye.
/// @return Transformation matrix from Head Space to selected Eye Space.
gvr_mat4f gvr_get_eye_from_head_matrix(const gvr_context* gvr,
                                       const gvr_eye eye);

/// Gets the window bounds.
///
/// @param gvr Pointer to the gvr instance from which to get the bounds.
///
/// @return Window bounds in physical pixels.
gvr_recti gvr_get_window_bounds(const gvr_context* gvr);

/// Computes the distorted point for a given point in a given eye.  The
/// distortion inverts the optical distortion caused by the lens for the eye.
/// Due to chromatic aberration, the distortion is different for each
/// color channel.
///
/// @param gvr Pointer to the gvr instance which will do the computing.
/// @param eye The eye (left or right).
/// @param uv_in A point in screen eye Viewport Space in [0,1]^2 with (0, 0)
///     in the lower left corner of the eye's viewport and (1, 1) in the
///     upper right corner of the eye's viewport.
/// @param uv_out A pointer to an array of (at least) 3 elements, with each
///     element being a Point2f representing a point in render texture eye
///     Viewport Space in [0,1]^2 with (0, 0) in the lower left corner of the
///     eye's viewport and (1, 1) in the upper right corner of the eye's
///     viewport.
///     `*uv_out[0]` is the corrected position of `uv_in` for the red channel
///     `*uv_out[1]` is the corrected position of `uv_in` for the green channel
///     `*uv_out[2]` is the corrected position of `uv_in` for the blue channel
void gvr_compute_distorted_point(const gvr_context* gvr, const gvr_eye eye,
                                 const gvr_vec2f uv_in, gvr_vec2f uv_out[3]);

/// @}

/////////////////////////////////////////////////////////////////////////////
// Parameters
/////////////////////////////////////////////////////////////////////////////
/// @defgroup Parameters Parameters
/// @brief Functions for getting and setting system parameters.
/// @{

/// Sets a bool parameter to a new value.
/// The new parameter value is applied when the next frame is drawn.
///
/// @param gvr Pointer to the gvr instance for which to set the value.
/// @param param The id of the parameter we want to set.
/// @param value The new value we want to set the parameter to.
/// @return Whether the parameter was updated successfully. In particular,
///     false is returned if param is invalid.
bool gvr_set_bool_parameter(gvr_context* gvr,
                            const gvr_bool_parameter param,
                            bool value);

/// Gets the current value of a bool parameter.
///
/// @param gvr Pointer to the gvr instance from which to get the value.
/// @param param The id of the parameter we want to get.
/// @return The current value of the parameter. False is returned if param
///     is invalid.
bool gvr_get_bool_parameter(const gvr_context* gvr,
                            const gvr_bool_parameter param);

/// Gets the current monotonic system time.
///
/// @return The current monotonic system time.
gvr_clock_time_point gvr_get_time_point_now();

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
namespace gvr {

/// Convenience C++ wrapper for the opaque gvr_render_params_list type. This
/// class will automatically release the wrapped gvr_render_params_list upon
/// destruction. It can only be created via a `GvrApi` instance, and its
/// validity is tied to the lifetime of that instance.
class RenderParamsList {
 public:
  RenderParamsList(RenderParamsList&& other)
      : context_(nullptr), params_list_(nullptr) {
    std::swap(context_, other.context_);
    std::swap(params_list_, other.params_list_);
  }

  RenderParamsList& operator=(RenderParamsList&& other) {
    std::swap(context_, other.context_);
    std::swap(params_list_, other.params_list_);
    return *this;
  }

  ~RenderParamsList() {
    if (params_list_) {
      gvr_destroy_render_params_list(&params_list_);
    }
  }

  /// For more information, see gvr_get_recommended_render_params_list().
  void SetToRecommendedRenderParams() {
    gvr_get_recommended_render_params_list(context_, params_list_);
  }

  /// For more information, see gvr_get_screen_render_params_list().
  void SetToScreenRenderParams() {
    gvr_get_screen_render_params_list(context_, params_list_);
  }

  /// For more information, see gvr_set_render_params().
  void SetRenderParams(size_t index, const RenderParams& render_params) {
    gvr_set_render_params(context_, params_list_, index, &render_params);
  }

  /// For more information, see gvr_get_render_params().
  RenderParams GetRenderParams(size_t index) const {
    return gvr_get_render_params(params_list_, index);
  }

  /// For more information, see gvr_get_render_params_list_size().
  size_t GetSize() const {
    return gvr_get_render_params_list_size(params_list_);
  }

 private:
  friend class GvrApi;

  explicit RenderParamsList(gvr_context* context)
      : context_(context),
        params_list_(gvr_create_render_params_list(context)) {}

  const gvr_context* context_;
  gvr_render_params_list* params_list_;

  // Disallow copy and assign.
  RenderParamsList(const RenderParamsList&);
  void operator=(const RenderParamsList&);
};

/// Convenience C++ wrapper for gvr_framebuffer_spec, an opaque framebuffer
/// specification. Frees the underlying gvr_framebuffer_spec on destruction.
class FramebufferSpec {
 public:
  FramebufferSpec(FramebufferSpec&& other) {
    std::swap(spec_, other.spec_);
  }

  FramebufferSpec& operator=(FramebufferSpec&& other) {
    std::swap(spec_, other.spec_);
    return *this;
  }
  ~FramebufferSpec() {
    gvr_framebuffer_spec_destroy(&spec_);
  }

  /// Gets the framebuffer's size. The default value is the recommended render
  /// target size. For more information, see gvr_framebuffer_spec_get_size().
  Sizei GetSize() const {
    return gvr_framebuffer_spec_get_size(spec_);
  }
  /// Sets the framebuffer's size. For more information, see
  /// gvr_framebuffer_spec_set_size().
  /// @param size The size. Width and height much both be greater than zero.
  void SetSize(const Sizei& size) {
    gvr_framebuffer_spec_set_size(spec_, size);
  }
  /// Sets the framebuffer's size to the passed width and height. For more
  /// information, see gvr_framebuffer_spec_set_size().
  /// @param width The width in pixels. Must be greater than 0.
  /// @param height The height in pixels. Must be greater than 0.
  void SetSize(int32_t width, int32_t height) {
    gvr_sizei size{width, height};
    gvr_framebuffer_spec_set_size(spec_, size);
  }

  /// Gets the number of samples per pixel in the framebuffer. For more
  /// information, see gvr_framebuffer_spec_get_samples().
  int GetSamples() const {
    return gvr_framebuffer_spec_get_samples(spec_);
  }
  /// Sets the number of samples per pixel. For more information, see
  /// gvr_framebuffer_spec_set_samples().
  void SetSamples(int num_samples) {
    gvr_framebuffer_spec_set_samples(spec_, num_samples);
  }

 private:
  friend class GvrApi;
  friend class OffscreenFramebufferHandle;

  explicit FramebufferSpec(gvr_context* gvr) {
    spec_ = gvr_framebuffer_spec_create(gvr);
  }

  gvr_framebuffer_spec* spec_;
};

/// Convenience C++ wrapper for a GVR offscreen framebuffer handle.  Note that
/// this wrapper does *not* automatically free the framebuffer resources upon
/// destruction; GvrApi will do that automatically when the context is
/// destroyed. The client may explicitly free the framebuffer by calling
/// Reset(). Also note that this wrapper can only be created via a `GvrApi`
/// instance, and its validity is tied to the lifetime of that instance.
class OffscreenFramebufferHandle {
 public:
  OffscreenFramebufferHandle(OffscreenFramebufferHandle&& other)
      : context_(nullptr), handle_(0) {
    std::swap(context_, other.context_);
    std::swap(handle_, other.handle_);
  }

  OffscreenFramebufferHandle& operator=(OffscreenFramebufferHandle&& other) {
    // Self-assignment is lame, but guard against it anyway.
    if (this != &other) {
      Reset();
      std::swap(context_, other.context_);
      std::swap(handle_, other.handle_);
    }
    return *this;
  }

  ~OffscreenFramebufferHandle() {
    // It's dangerous to assume that context_ is still valid at this point, so
    // we rely instead on the context_ for framebuffer resource cleanup.
  }

  /// Optional, explicit framebuffer release to recover resources. Note that the
  /// framebuffer will be automatically released when the associated context is
  /// destroyed. For more information, see gvr_release_offscreen_framebuffer().
  void Reset() {
    if (context_ && handle_) {
      gvr_release_offscreen_framebuffer(context_, handle_);
    }
    context_ = nullptr;
    handle_ = 0;
  }

  /// For more information, see gvr_set_active_offscreen_framebuffer().
  void SetActive() { gvr_set_active_offscreen_framebuffer(context_, handle_); }

  /// For more information, see gvr_set_default_framebuffer_active().
  void SetDefaultFramebufferActive() {
    gvr_set_default_framebuffer_active(context_);
  }

  /// For more information, see gvr_resize_offscreen_framebuffer().
  void Resize(Sizei size) {
    gvr_resize_offscreen_framebuffer(context_, handle_, size);
  }

  /// For more information, see gvr_get_offscreen_framebuffer_size().
  Sizei GetSize() const {
    return gvr_get_offscreen_framebuffer_size(context_, handle_);
  }

  /// For more information, see gvr_get_offscreen_framebuffer_resource_id().
  int GetResourceId() const {
    return gvr_get_offscreen_framebuffer_resource_id(context_, handle_);
  }

 private:
  friend class GvrApi;

  OffscreenFramebufferHandle(gvr_context* context,
                             const gvr_framebuffer_spec* spec)
      : context_(context),
        handle_(gvr_create_offscreen_framebuffer(context, spec)) {}

  gvr_context* context_;
  int handle_;

  // Disallow copy and assign.
  OffscreenFramebufferHandle(const OffscreenFramebufferHandle&);
  void operator=(const OffscreenFramebufferHandle&);
};

/// This is a convenience C++ wrapper for the Google VR C API.
///
/// This wrapper strategy prevents ABI compatibility issues between compilers
/// by ensuring that the interface between client code and the implementation
/// code in libgvr.so is a pure C interface. The translation from C++ calls
/// to C calls provided by this wrapper runs entirely in the client's binary
/// and is compiled by the client's compiler.
///
/// Methods in this class are only documented insofar as the C++ wrapping logic
/// is concerned; for information about the method itself, please refer to the
/// corresponding function in the C API.
///
/// Example API usage:
///
///     // Functionality supplied by the application in the example below has
///     // the "app-" prefix.
///     #ifdef __ANDROID__
///     // On Android, the gvr_context should almost always be obtained from the
///     // Java GvrLayout object via
///     // GvrLayout.getGvrApi().getNativeGvrContext().
///     std::unique_ptr<GvrApi> gvr = GvrApi::WrapNonOwned(gvr_context);
///     #else
///     std::unique_ptr<GvrApi> gvr = GvrApi::Create();
///     #endif
///
///     gvr->InitializeGl();
///
///     gvr::RenderParamsList render_params_list =
///         gvr->CreateEmptyRenderParamsList();
///     render_params_list.SetToRecommendedRenderParams();
///     const gvr::RenderParams left_eye_params =
///         render_params_list.GetRenderParams(0);
///     const gvr::RenderParams right_eye_params =
///         render_params_list.GetRenderParams(1);
///
///     while (client_app_should_render) {
///       // A client app should be ready for the render target size to change
///       // whenever a new QR code is scanned, or a new viewer is paired.
///       gvr::Sizei render_target_size =
///           gvr->GetRecommendedRenderTargetSize();
///       int offscreen_texture_id =
///           AppInitializeOrResizeOffscreenBuffer(render_target_size);
///
///       // This function will depend on your render loop's implementation.
///       gvr::ClockTimePoint next_vsync = AppGetNextVsyncTime();
///
///       const gvr::HeadPose head_pose =
///           gvr->GetHeadPoseInStartSpace(next_vsync);
///       const gvr::Mat4f head_matrix = head_pose.object_from_reference_matrix;
///       const gvr::Mat4f left_eye_matrix = MatrixMultiply(
///           gvr->GetEyeFromHeadMatrix(kLeftEye), head_matrix);
///       const gvr::Mat4f right_eye_matrix = MatrixMultiply(
///           gvr->GetEyeFromHeadMatrix(kRightEye), head_matrix);
///
///       // A client app does its rendering here.
///       AppSetRenderTarget(offscreen_texture_id);
///
///       AppDoSomeRenderingForEye(
///           left_eye_params.eye_viewport_bounds, left_eye_matrix);
///       AppDoSomeRenderingForEye(
///           right_eye_params.eye_viewport_bounds, right_eye_matrix);
///       AppSetRenderTarget(primary_display);
///
///       gvr->DistortToScreen(
///           frame_texture_id, render_params_list, &head_pose, &next_vsync);
///       AppSwapBuffers();
///     }
///
class GvrApi {
 public:
#ifdef __ANDROID__
  /// Instantiates and returns a GvrApi instance that owns a gvr_context.
  ///
  /// @param env The JNIEnv associated with the current thread.
  /// @param app_context The Android application context. This must be the
  ///     application context, NOT an Activity context (Note: from any Android
  ///     Activity in your app, you can call getApplicationContext() to
  ///     retrieve the application context).
  /// @param class_loader The class loader to use when loading Java classes.
  ///     This must be your app's main class loader (usually accessible through
  ///     activity.getClassLoader() on any of your Activities).
  /// @return unique_ptr to the created GvrApi instance, nullptr on failure.
  static std::unique_ptr<GvrApi> Create(JNIEnv* env, jobject app_context,
                                        jobject class_loader) {
    gvr_context* context = gvr_create(env, app_context, class_loader);
    if (!context) {
      return nullptr;
    }
    return std::unique_ptr<GvrApi>(new GvrApi(context, true /* owned */));
  }
#else
  /// Instantiates and returns a GvrApi instance that owns a gvr_context.
  ///
  /// @return unique_ptr to the created GvrApi instance, nullptr on failure.
  static std::unique_ptr<GvrApi> Create() {
    gvr_context* context = gvr_create();
    if (!context) {
      return nullptr;
    }
    return std::unique_ptr<GvrApi>(new GvrApi(context, true /* owned */));
  }
#endif  // #ifdef __ANDROID__

  /// Instantiates a GvrApi instance that wraps a *non-owned* gvr_context.
  ///
  /// Ownership of the provided `context` remains with the caller, and they
  /// are responsible for ensuring proper disposal of the context.
  ///
  /// @param context Pointer to a non-null, non-owned gvr_context instance.
  /// @return unique_ptr to the created GvrApi instance. Never null.
  static std::unique_ptr<GvrApi> WrapNonOwned(gvr_context* context) {
    return std::unique_ptr<GvrApi>(new GvrApi(context, false /* owned */));
  }

  ~GvrApi() {
    if (context_ && owned_) {
      gvr_destroy(&context_);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Rendering
  /////////////////////////////////////////////////////////////////////////////

  /// For more information, see gvr_initialize_gl().
  void InitializeGl() { gvr_initialize_gl(context_); }

  /// Constructs a C++ wrapper for a gvr_render_params_list object.
  /// For more information, see gvr_create_render_params_list() in the C API
  /// and the `RenderParamsList` definition in gvr_types.h.
  ///
  /// @return A new, empty RenderParamsList instance.
  ///     Note: The validity of the returned object is closely tied to the
  ///     lifetime of the member gvr_context. The caller is responsible for
  ///     ensuring correct usage accordingly.
  RenderParamsList CreateEmptyRenderParamsList() const {
    return RenderParamsList(context_);
  }

  /// For more information, see gvr_get_recommended_render_target_size().
  Sizei GetRecommendedRenderTargetSize() const {
    return gvr_get_recommended_render_target_size(context_);
  }

  /// For more information, see gvr_get_screen_target_size().
  Sizei GetScreenTargetSize() const {
    return gvr_get_screen_target_size(context_);
  }

  /// For more information, see gvr_distort_to_screen().
  void DistortToScreen(int texture_id,
                       const RenderParamsList& render_params_list,
                       const HeadPose* rendered_head_pose_in_start_space,
                       const ClockTimePoint* texture_presentation_time) {
    gvr_distort_to_screen(context_, texture_id, render_params_list.params_list_,
                          rendered_head_pose_in_start_space,
                          texture_presentation_time);
  }

  /// For more information, see gvr_distort_offscreen_framebuffer_to_screen().
  void DistortOffscreenFramebufferToScreen(
      const OffscreenFramebufferHandle& offscreen_fbo_handle,
      const RenderParamsList& render_params_list,
      const HeadPose* rendered_head_pose_in_start_space,
      const ClockTimePoint* texture_presentation_time) {
    gvr_distort_offscreen_framebuffer_to_screen(
        context_, offscreen_fbo_handle.handle_, render_params_list.params_list_,
        rendered_head_pose_in_start_space, texture_presentation_time);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Offscreen Framebuffers
  /////////////////////////////////////////////////////////////////////////////

  FramebufferSpec CreateFramebufferSpec() {
    return FramebufferSpec(context_);
  }

  /// Constructs a C++ wrapper for a GVR offscreen framebuffer handle.
  /// For more information, see gvr_create_offscreen_framebuffer() and the
  /// `OffscreenFramebufferHandle` type definition.
  ///
  /// WARNING: Framebuffer creation and usage should occur only *after*
  /// calling `InitializeGl()`.
  ///
  /// @param spec Framebuffer specification.
  /// @return A managed warpper for an offscreen framebuffer handle.
  ///     Note: The validity of the returned object is closely tied to the
  ///     lifetime of the member gvr_context. The caller is responsible for
  ///     ensuring correct usage accordingly.
  OffscreenFramebufferHandle CreateOffscreenFramebuffer(
      const FramebufferSpec& spec) {
    return OffscreenFramebufferHandle(context_, spec.spec_);
  }

  /// For more information, see gvr_set_default_framebuffer_active().
  void SetDefaultFramebufferActive() {
    gvr_set_default_framebuffer_active(context_);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Head tracking
  /////////////////////////////////////////////////////////////////////////////

  /// For more information, see gvr_get_head_pose_in_start_space().
  HeadPose GetHeadPoseInStartSpace(ClockTimePoint time) const {
    return gvr_get_head_pose_in_start_space(context_, time);
  }

  /// For more information, see gvr_pause_tracking().
  void PauseTracking() { gvr_pause_tracking(context_); }

  /// For more information, see gvr_resume_tracking().
  void ResumeTracking() { gvr_resume_tracking(context_); }

  /// For more information, see gvr_reset_tracking().
  void ResetTracking() { gvr_reset_tracking(context_); }

  // For more information, see gvr_recenter_tracking().
  void RecenterTracking() { gvr_recenter_tracking(context_); }

  /////////////////////////////////////////////////////////////////////////////
  // Head mounted display.
  /////////////////////////////////////////////////////////////////////////////

  /// For more information, see gvr_set_default_viewer_profile().
  bool SetDefaultViewerProfile(const char* viewer_profile_uri) {
    return gvr_set_default_viewer_profile(context_, viewer_profile_uri);
  }

  /// For more information, see gvr_refresh_viewer_profile().
  void RefreshViewerProfile() { gvr_refresh_viewer_profile(context_); }

  /// For more information, see gvr_get_viewer_vendor().
  const char* GetViewerVendor() const {
    return gvr_get_viewer_vendor(context_);
  }

  /// For more information, see gvr_get_viewer_model().
  const char* GetViewerModel() const { return gvr_get_viewer_model(context_); }

  /// For more information, see gvr_get_eye_from_head_matrix().
  Mat4f GetEyeFromHeadMatrix(Eye eye) const {
    return gvr_get_eye_from_head_matrix(context_, eye);
  }

  /// For more information, see gvr_get_window_bounds().
  Recti GetWindowBounds() const { return gvr_get_window_bounds(context_); }

  /// For more information, see gvr_compute_distorted_point().
  std::array<Vec2f, 3> ComputeDistortedPoint(Eye eye, const Vec2f& uv_in) {
    std::array<Vec2f, 3> uv_out = {{{}}};
    gvr_compute_distorted_point(context_, eye, uv_in, uv_out.data());
    return uv_out;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Parameters
  /////////////////////////////////////////////////////////////////////////////

  /// For more information, see gvr_set_bool_parameter().
  bool SetBoolParameter(BoolParameterId param_id, bool value) {
    return gvr_set_bool_parameter(context_, param_id, value);
  }

  /// For more information, see gvr_get_bool_parameter().
  bool GetBoolParameter(BoolParameterId param_id) const {
    return gvr_get_bool_parameter(context_, param_id);
  }

  /// For more information, see gvr_get_time_point_now().
  static ClockTimePoint GetTimePointNow() { return gvr_get_time_point_now(); }

 private:
  GvrApi(gvr_context* context, bool owned) : context_(context), owned_(owned) {}

  gvr_context* context_;

  // Whether context_ is owned by the GvrApi instance. If owned, the context
  // will be released upon destruction.
  const bool owned_;

  // Disallow copy and assign.
  GvrApi(const GvrApi&);
  void operator=(const GvrApi&);
};

}  // namespace gvr
#endif  // #ifdef __cplusplus

#endif  // VR_GVR_CAPI_INCLUDE_GVR_H_
