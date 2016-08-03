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

#if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)
#include <array>
#include <memory>
#include <vector>
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
///     gvr_buffer_viewport_list* viewport_list =
///         gvr_buffer_viewport_list_create(gvr);
///     gvr_get_recommended_buffer_viewports(gvr, viewport_list);
///     gvr_buffer_viewport* left_eye_vp = gvr_buffer_viewport_create(gvr);
///     gvr_buffer_viewport* right_eye_vp = gvr_buffer_viewport_create(gvr);
///     gvr_buffer_viewport_list_get_item(viewport_list, 0, left_eye_vp);
///     gvr_buffer_viewport_list_get_item(viewport_list, 1, right_eye_vp);
///
///     while (client_app_should_render) {
///       // A client app should be ready for the render target size to change
///       // whenever a new QR code is scanned, or a new viewer is paired.
///       gvr_sizei render_target_size =
///           gvr_get_recommended_render_target_size(gvr);
///       gvr_swap_chain_resize_buffer(swap_chain, 0, target_size);
///
///       // This function will depend on your render loop's implementation.
///       gvr_clock_time_point next_vsync = AppGetNextVsyncTime();
///
///       const gvr_mat4f head_pose =
///           gvr_get_head_pose_in_start_space(gvr, next_vsync);
///       const gvr_mat4f left_eye_pose = MatrixMultiply(
///           gvr_get_eye_from_head_matrix(gvr, GVR_LEFT_EYE), head_pose);
///       const gvr::Mat4f right_eye_pose = MatrixMultiply(
///           gvr_get_eye_from_head_matrix(gvr, GVR_RIGHT_EYE), head_pose);
///
///       // Insert client rendering code here.
///
///       AppSetRenderTarget(offscreen_texture_id);
///
///       AppDoSomeRenderingForEye(
///           gvr_buffer_viewport_get_source_uv(left_eye_vp),
///           left_eye_matrix);
///       AppDoSomeRenderingForEye(
///           gvr_buffer_viewport_get_source_uv(right_eye_vp),
///           right_eye_matrix);
///       AppSetRenderTarget(primary_display);
///
///       gvr_frame_submit(&frame, viewport_list, head_matrix);
///     }
///
///     // Cleanup memory.
///     gvr_buffer_viewport_list_destroy(&viewport_list);
///     gvr_buffer_viewport_destroy(&left_eye_vp);
///     gvr_buffer_viewport_destroy(&right_eye_vp);
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
/// The instance must remain valid as long as any GVR object is in use. When
/// the application no longer needs to use the GVR SDK, call gvr_destroy().
///
///
/// On Android, the gvr_context should *almost always* be obtained from the Java
/// GvrLayout object, rather than explicitly created here. The GvrLayout should
/// live in the app's View hierarchy, and its use is required to ensure
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
/// @return Pointer to the created gvr instance, NULL on failure.
gvr_context* gvr_create(JNIEnv* env, jobject context, jobject class_loader);
#else
/// @return Pointer to the created gvr instance, NULL on failure.
gvr_context* gvr_create();
#endif  // #ifdef __ANDROID__

/// Gets the current GVR runtime version.
///
/// Note: This runtime version may differ from the version against which the
/// client app is compiled.
///
/// @param version The gvr_version to populate.
gvr_version gvr_get_version();

/// Gets a string representation of the current GVR runtime version. This is of
/// the form "MAJOR.MINOR.PATCH".
///
/// Note: This runtime version may differ from the version against which the
/// client app is compiled.
///
/// @return The version as a static char pointer.
const char* gvr_get_version_string();

/// Destroys a gvr_context instance.  The parameter will be nulled by this
/// operation.  Once this function is called, the behavior of any subsequent
/// call to a GVR SDK function that references objects created from this
/// context is undefined.
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

/// Gets whether asynchronous reprojection is currently enabled.
///
/// If enabled, frames will be collected by the rendering system and
/// asynchronously re-projected in sync with the scanout of the display. This
/// feature may not be available on every platform, and requires a
/// high-priority render thread with special extensions to function properly.
///
/// Note: On Android, this feature can be enabled solely via the GvrLayout Java
/// instance which (indirectly) owns this gvr_context. The corresponding
/// method call is GvrLayout.setAsyncReprojectionEnabled().
///
/// @param gvr Pointer to the gvr instance.
/// @return Whether async reprojection is enabled. Defaults to false.
bool gvr_get_async_reprojection_enabled(const gvr_context* gvr);

/// Creates a new, empty list of render parameters. Render parameters
/// are used internally to inform distortion rendering, and are a function
/// of intrinisic screen and viewer parameters.
///
/// The caller should populate the returned viewport using one of:
///   - gvr_get_recommended_buffer_viewports()
///   - gvr_get_screen_buffer_viewports()
///   - gvr_buffer_viewport_list_set()
///
/// @param gvr Pointer the gvr instance from which to allocate the viewport
/// list.
///
/// @return A handle to an allocated gvr_buffer_viewport_list. The caller is
///     responsible for calling gvr_buffer_viewport_list_destroy() on the
///     returned handle.
gvr_buffer_viewport_list* gvr_buffer_viewport_list_create(
    const gvr_context* gvr);

/// Destroys a gvr_buffer_viewport_list instance. The parameter will be nulled
/// by this operation.
///
/// @param viewport_list Pointer to a pointer to the viewport list instance to
///     be destroyed and nulled.
void gvr_buffer_viewport_list_destroy(gvr_buffer_viewport_list** viewport_list);

/// Returns the size of a given render viewport list.
///
/// @param viewport_list Pointer to a render viewport list.
///
/// @return The number of entries in the viewport list.
size_t gvr_buffer_viewport_list_get_size(
    const gvr_buffer_viewport_list* viewport_list);

/// Gets the recommended buffer viewport configuration, populating a previously
/// allocated gvr_buffer_viewport_list object. The updated values include the
/// per-eye recommended viewport and field of view for the target.
///
/// When the recommended viewports are used for distortion rendering, this
/// method should always be called after calling refresh_viewer_profile(). That
/// will ensure that the populated viewports reflect the currently paired
/// viewer.
///
/// @param gvr Pointer to the gvr instance from which to get the viewports.
/// @param viewport_list Pointer to a previously allocated viewport list. This
///     will be populated with the recommended buffer viewports and resized if
///     necessary.
void gvr_get_recommended_buffer_viewports(
    const gvr_context* gvr, gvr_buffer_viewport_list* viewport_list);

/// Gets the screen (non-distorted) buffer viewport configuration, populating a
/// previously allocated gvr_buffer_viewport_list object. The updated values
/// include the per-eye recommended viewport and field of view for the target.
///
/// @param gvr Pointer to the gvr instance from which to get the viewports.
/// @param viewport_list Pointer to a previously allocated viewport list. This
///     will be populated with the screen buffer viewports and resized if
///     necessary.
void gvr_get_screen_buffer_viewports(const gvr_context* gvr,
                                     gvr_buffer_viewport_list* viewport_list);

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

/// Performs postprocessing, including lens distortion, on the contents of the
/// passed texture and shows the result on the screen. Lens distortion is
/// determined by the parameters of the viewer encoded in its QR code. The
/// passed texture is not modified.
///
/// If the application does not call gvr_initialize_gl() before calling this
/// function, the results are undefined.
///
/// @deprecated This function exists only to support legacy rendering pathways
///     for Cardboard devices. It is incompatible with the low-latency
///     experiences supported by async reprojection. Use the swap chain API
///     instead.
///
/// @param gvr Pointer to the gvr instance which will do the distortion.
/// @param texture_id The OpenGL ID of the texture that contains the next frame
///     to be displayed.
/// @param viewport_list Rendering parameters.
/// @param rendered_head_pose_in_start_space This parameter is ignored.
/// @param target_presentation_time This parameter is ignored.
void gvr_distort_to_screen(gvr_context* gvr, int32_t texture_id,
                           const gvr_buffer_viewport_list* viewport_list,
                           gvr_mat4f rendered_head_pose_in_start_space,
                           gvr_clock_time_point target_presentation_time);

/// Creates a gvr_buffer_viewport instance.
gvr_buffer_viewport* gvr_buffer_viewport_create(gvr_context* gvr);

/// Frees a gvr_buffer_viewport instance and clears the pointer.
void gvr_buffer_viewport_destroy(gvr_buffer_viewport** viewport);

/// Gets the UV coordinates specifying where the output buffer is sampled.
///
/// @param viewport The buffer viewport.
/// @return UV coordinates as a rectangle.
gvr_rectf gvr_buffer_viewport_get_source_uv(
    const gvr_buffer_viewport* viewport);

/// Sets the UV coordinates specifying where the output buffer should be
/// sampled when compositing the final distorted image.
///
/// @param viewport The buffer viewport.
/// @param uv The new UV coordinates for sampling.
void gvr_buffer_viewport_set_source_uv(gvr_buffer_viewport* viewport,
                                       gvr_rectf uv);

/// Retrieves the field of view for the referenced buffer region.
///
/// @param viewport The buffer viewport.
/// @return The field of view of the rendered image.
gvr_rectf gvr_buffer_viewport_get_source_fov(
    const gvr_buffer_viewport* viewport);

/// Sets the field of view for the referenced buffer region.
///
/// @param viewport The buffer viewport.
/// @param The field of view to use when compositing the rendered image.
void gvr_buffer_viewport_set_source_fov(gvr_buffer_viewport* viewport,
                                        gvr_rectf fov);

/// Gets the target logical eye for the specified viewport.
///
/// @param viewport The buffer viewport.
/// @return Index of the target logical eye for this viewport.
int32_t gvr_buffer_viewport_get_target_eye(const gvr_buffer_viewport* viewport);

/// Sets the target logical eye for the specified viewport.
///
/// @param viewport The buffer viewport.
/// @param index Index of the target logical eye.
void gvr_buffer_viewport_set_target_eye(gvr_buffer_viewport* viewport,
                                        int32_t index);

/// Compares two gvr_buffer_viewport instances and returns true if they specify
/// specify the same view mapping.
///
/// @param a Instance of a buffer viewport.
/// @param b Another instance of a buffer viewport.
/// @return True if the passed viewports are the same.
bool gvr_buffer_viewport_equal(const gvr_buffer_viewport* a,
                               const gvr_buffer_viewport* b);

/// Retrieves the buffer viewport from a given viewport list entry. This
/// includes the eye viewport bounds, the fov, and the eye type.
///
/// @param viewport_list Pointer to the previously allocated viewport list.
/// @param index Index of the viewport entry to query. The `viewport_list`
///    size must be greater than the provided (0-based) index.
/// @param viewport The buffer viewport.
void gvr_buffer_viewport_list_get_item(
    const gvr_buffer_viewport_list* viewport_list, size_t index,
    gvr_buffer_viewport* viewport);

/// Updates a buffer viewport entry from the given eye parameters. Data from the
/// device -- as well as the provided parameters -- will be used to populate
/// the internal viewport distortion parameters, which are used for rendering
/// the distortion.
///
/// Note: This method should be used rarely and only for custom, non-standard
/// rendering flows. The typical flow will use
/// gvr_get_recommended_buffer_viewports() to populate the necessary values for
/// distortion rendering.
///
/// @param viewport_list Pointer to a previously allocated viewport list.
/// @param index Index of the buffer viewport entry to update. If the
///     `viewport_list` size is equal to the index, a new viewport entry will be
///     added. The `viewport_list` size must *not* be less than the index value.
/// @param viewport A pointer to the buffer viewport object.
void gvr_buffer_viewport_list_set_item(gvr_buffer_viewport_list* viewport_list,
                                       size_t index,
                                       const gvr_buffer_viewport* viewport);

/// @}

/////////////////////////////////////////////////////////////////////////////
// Swapchain
/////////////////////////////////////////////////////////////////////////////
/// @defgroup swap_chain Swap chain
/// @brief Functions to create a swap chain, manipulate it and submit frames
///     for lens distortion and presentation on the screen.
/// @{

/// Creates a default buffer specification.
gvr_buffer_spec* gvr_buffer_spec_create(gvr_context* gvr);

/// Destroy the buffer specification and null the pointer.
void gvr_buffer_spec_destroy(gvr_buffer_spec** spec);

/// Gets the size of the buffer to be created.
///
/// @param spec Buffer specification.
/// @return Size of the pixel buffer. The default is equal to the recommended
///     render target size at the time when the specification was created.
gvr_sizei gvr_buffer_spec_get_size(const gvr_buffer_spec* spec);

/// Sets the size of the buffer to be created.
///
/// @param spec Buffer specification.
/// @param size The size. Width and height must both be greater than zero.
///     Otherwise, the application is aborted.
void gvr_buffer_spec_set_size(gvr_buffer_spec* spec, gvr_sizei size);

/// Gets the number of samples per pixel in the buffer to be created.
///
/// @param spec Buffer specification.
/// @return Value >= 1 giving the number of samples. 1 means multisampling is
///     disabled. Negative values and 0 are never returned.
int32_t gvr_buffer_spec_get_samples(const gvr_buffer_spec* spec);

/// Sets the number of samples per pixel in the buffer to be created.
///
/// @param spec Buffer specification.
/// @param num_samples The number of samples. Negative values are an error.
///     The values 0 and 1 are treated identically and indicate that
//      multisampling should be disabled.
void gvr_buffer_spec_set_samples(gvr_buffer_spec* spec, int32_t num_samples);

/// Sets the color format for the buffer to be created. Default format is
/// GVR_COLOR_FORMAT_RGBA_8888.
///
/// @param spec Buffer specification.
/// @param color_format The color format for the buffer. Valid formats are in
///     the gvr_color_format_type enum.
void gvr_buffer_spec_set_color_format(gvr_buffer_spec* spec,
                                      int32_t color_format);

/// Sets the depth and stencil format for the buffer to be created. Currently,
/// only packed stencil formats are supported. Default format is
/// GVR_DEPTH_STENCIL_FORMAT_DEPTH_16.
///
/// @param spec Buffer specification.
/// @param color_format The depth and stencil format for the buffer. Valid
///     formats are in the gvr_depth_stencil_format_type enum.
void gvr_buffer_spec_set_depth_stencil_format(gvr_buffer_spec* spec,
                                              int32_t depth_stencil_format);

/// Creates a swap chain from the given buffer specifications.
/// This is a potentially time-consuming operation. All frames within the
/// swapchain will be allocated. Once rendering is stopped, call
/// gvr_swap_chain_destroy() to free GPU resources. The passed gvr_context must
/// not be destroyed until then.
///
/// Note: Currently, swapchain creation supports *only one* specified buffer.
/// This restriction will be removed in the next GVR SDK release.
///
/// @param gvr GVR instance for which a swap chain will be created.
/// @param buffers Array of pixel buffer specifications. Each frame in the
///     swap chain will be composed of these buffers.
/// @param count Number of buffer specifications in the array.
/// @return Opaque handle to the newly created swap chain.
gvr_swap_chain* gvr_swap_chain_create(gvr_context* gvr,
                                      const gvr_buffer_spec** buffers,
                                      int32_t count);

/// Destroys the swap chain and nulls the pointer.
void gvr_swap_chain_destroy(gvr_swap_chain** swap_chain);

/// Gets the number of buffers in each frame of the swap chain.
int32_t gvr_swap_chain_get_buffer_count(const gvr_swap_chain* swap_chain);

/// Retrieves the size of the specified pixel buffer. Note that if the buffer
/// was resized while the current frame was acquired, the return value will be
/// different than the value obtained from the equivalent function for the
/// current frame.
///
/// @param swap_chain The swap chain.
/// @param index Index of the pixel buffer.
/// @return Size of the specified pixel buffer in frames that will be returned
///     from gvr_swap_chain_acquire_frame().
gvr_sizei gvr_swap_chain_get_buffer_size(gvr_swap_chain* swap_chain,
                                         int32_t index);

/// Resizes the specified pixel buffer to the given size. The frames are resized
/// when they are unused, so the currently acquired frame will not be resized
/// immediately.
///
/// @param swap_chain The swap chain.
/// @param index Index of the pixel buffer to resize.
/// @param size New size for the specified pixel buffer.
void gvr_swap_chain_resize_buffer(gvr_swap_chain* swap_chain, int32_t index,
                                  gvr_sizei size);

/// Acquires a frame from the swap chain for rendering. Buffers that are part of
/// the frame can then be bound with gvr_frame_bind_buffer(). Once the frame
/// is finished and all its constituent buffers are ready, call
/// gvr_frame_submit() to display it while applying lens distortion.
///
/// Note: If frame submission is asynchronous, e.g., when async reprojection is
/// enabled, it's possible that no free frames will be available for
/// acquisition. Under such circumstances, this method will fail and return
/// NULL. The client should re-attempt acquisition after a reasonable delay,
/// e.g., several milliseconds.
///
/// @param swap_chain The swap chain.
/// @return Handle to the acquired frame. NULL if no frames are available.
gvr_frame* gvr_swap_chain_acquire_frame(gvr_swap_chain* swap_chain);

/// Binds a pixel buffer that is part of the frame to the OpenGL framebuffer.
///
/// @param frame Frame handle acquired from the swap chain.
/// @param index Index of the pixel buffer to bind.
void gvr_frame_bind_buffer(gvr_frame* frame, int32_t index);

/// Unbinds any buffers bound from this frame and binds the default OpenGL
/// framebuffer.
void gvr_frame_unbind(gvr_frame* frame);

/// Returns the dimensions of the pixel buffer with the specified index. Note
/// that a frame that was acquired before resizing a swap chain buffer will not
/// be resized until it is submitted to the swap chain.
///
/// @param frame Frame handle.
/// @param index Index of the pixel buffer to inspect.
/// @return Dimensions of the specified pixel buffer.
gvr_sizei gvr_frame_get_buffer_size(const gvr_frame* frame, int32_t index);

/// Gets the name (ID) of the framebuffer object associated with the specified
/// buffer. The OpenGL state is not modified.
///
/// @param frame Frame handle.
/// @param index Index of a pixel buffer.
/// @return OpenGL object name (ID) of a framebuffer object which can be used
///     to render into the buffer. The ID is valid only until the frame is
///     submitted.
int32_t gvr_frame_get_framebuffer_object(const gvr_frame* frame, int32_t index);

/// Submits the frame for distortion and display on the screen. The passed
/// pointer is nulled to prevent reuse.
///
/// @param frame The frame to submit.
/// @param list Buffer view configuration to be used for this frame.
/// @param world_to_head_transform World-to-head transform with which the
///     frame was rendered.
void gvr_frame_submit(gvr_frame** frame, const gvr_buffer_viewport_list* list,
                      gvr_mat4f world_to_head_transform);

/// Resets the OpenGL framebuffer binding to the default.
void gvr_set_default_framebuffer_active(gvr_context* gvr);

/// @}

/////////////////////////////////////////////////////////////////////////////
// Head tracking
/////////////////////////////////////////////////////////////////////////////
/// @defgroup Headtracking Head tracking
/// @brief Functions for managing head tracking.
/// @{

/// Gets the current monotonic system time.
///
/// @return The current monotonic system time.
gvr_clock_time_point gvr_get_time_point_now();

/// Gets the transformation from start space to head space.  The head space
/// is a space where the head is at the origin and faces the -Z direction.
///
/// @param gvr Pointer to the gvr instance from which to get the pose.
/// @param time The time at which to get the head pose. The time should be in
///     the future. If the time is not in the future, it will be clamped to now.
/// @return A matrix representation of the head pose in start space.
gvr_mat4f gvr_get_head_pose_in_start_space(
    const gvr_context* gvr, const gvr_clock_time_point time);

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
/// @return A pointer to the vendor name. May be NULL if no viewer is paired.
///     WARNING: This method guarantees the validity of the returned pointer
///     only until the next use of the `gvr` context. The string should be
///     copied immediately if persistence is required.
const char* gvr_get_viewer_vendor(const gvr_context* gvr);

/// Gets the name of the viewer model.
///
/// @param gvr Pointer to the gvr instance from which to get the name.
/// @return A pointer to the model name. May be NULL if no viewer is paired.
///     WARNING: This method guarantees the validity of the returned pointer
///     only until the next use of the `gvr` context. The string should be
///     copied immediately if persistence is required.
const char* gvr_get_viewer_model(const gvr_context* gvr);

/// Gets the transformation matrix to convert from Head Space to Eye Space for
/// the given eye.
///
/// @param gvr Pointer to the gvr instance from which to get the matrix.
/// @param eye Selected gvr_eye type.
/// @return Transformation matrix from Head Space to selected Eye Space.
gvr_mat4f gvr_get_eye_from_head_matrix(const gvr_context* gvr,
                                       const int32_t eye);

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
/// @param eye The gvr_eye type (left or right).
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
void gvr_compute_distorted_point(const gvr_context* gvr, const int32_t eye,
                                 const gvr_vec2f uv_in, gvr_vec2f uv_out[3]);

#ifdef __cplusplus
}  // extern "C"
#endif

#if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)
namespace gvr {

/// Convenience C++ wrapper for the opaque gvr_buffer_viewport type.
/// The constructor allocates memory, so when used in tight loops, instances
/// should be reused.
class BufferViewport {
 public:
  BufferViewport(BufferViewport&& other)
      : viewport_(nullptr) {
    std::swap(viewport_, other.viewport_);
  }

  BufferViewport& operator=(BufferViewport&& other) {
    std::swap(viewport_, other.viewport_);
    return *this;
  }

  ~BufferViewport() {
    if (viewport_) gvr_buffer_viewport_destroy(&viewport_);
  }

  /// For more information, see gvr_buffer_viewport_get_source_fov().
  Rectf GetSourceFov() const {
    return gvr_buffer_viewport_get_source_fov(viewport_);
  }

  /// For more information, see gvr_buffer_viewport_set_source_fov().
  void SetSourceFov(const Rectf& fov) {
    gvr_buffer_viewport_set_source_fov(viewport_, fov);
  }

  /// For more information, see gvr_buffer_viewport_get_source_uv().
  Rectf GetSourceUv() const {
    return gvr_buffer_viewport_get_source_uv(viewport_);
  }

  /// For more information, see gvr_buffer_viewport_set_source_uv().
  void SetSourceUv(const Rectf& uv) {
    gvr_buffer_viewport_set_source_uv(viewport_, uv);
  }

  /// For more information, see gvr_buffer_viewport_get_target_eye().
  int32_t GetTargetEye() const {
    return gvr_buffer_viewport_get_target_eye(viewport_);
  }

  /// For more information, see gvr_buffer_viewport_set_target_eye().
  void SetTargetEye(int32_t index) {
    return gvr_buffer_viewport_set_target_eye(viewport_, index);
  }

  /// For more information, see gvr_buffer_viewport_equal().
  bool operator==(const BufferViewport& other) const {
    return gvr_buffer_viewport_equal(viewport_, other.viewport_) ? true : false;
  }
  bool operator!=(const BufferViewport& other) const {
    return !(*this == other);
  }

  /// Returns a BufferViewport that owns the passed C pointer. The pointer is
  /// then cleared.
  static BufferViewport Wrap(gvr_buffer_viewport** viewport) {
    BufferViewport result(*viewport);
    *viewport = nullptr;
    return result;
  }

 private:
  friend class GvrApi;
  friend class BufferViewportList;

  explicit BufferViewport(gvr_buffer_viewport* viewport)
      : viewport_(viewport) {}
  explicit BufferViewport(gvr_context* gvr)
      : viewport_(gvr_buffer_viewport_create(gvr)) {}

  gvr_buffer_viewport* viewport_;
};

/// Convenience C++ wrapper for the opaque gvr_buffer_viewport_list type. This
/// class will automatically release the wrapped gvr_buffer_viewport_list upon
/// destruction. It can only be created via a `GvrApi` instance, and its
/// validity is tied to the lifetime of that instance.
class BufferViewportList {
 public:
  BufferViewportList(BufferViewportList&& other)
      : context_(nullptr), viewport_list_(nullptr) {
    std::swap(context_, other.context_);
    std::swap(viewport_list_, other.viewport_list_);
  }

  BufferViewportList& operator=(BufferViewportList&& other) {
    std::swap(context_, other.context_);
    std::swap(viewport_list_, other.viewport_list_);
    return *this;
  }

  ~BufferViewportList() {
    if (viewport_list_) {
      gvr_buffer_viewport_list_destroy(&viewport_list_);
    }
  }

  /// For more information, see gvr_get_recommended_buffer_viewports().
  void SetToRecommendedBufferViewports() {
    gvr_get_recommended_buffer_viewports(context_, viewport_list_);
  }

  /// For more information, see gvr_get_screen_buffer_viewports().
  void SetToScreenBufferViewports() {
    gvr_get_screen_buffer_viewports(context_, viewport_list_);
  }

  /// For more information, see gvr_buffer_viewport_list_set_item().
  void SetBufferViewport(size_t index, const BufferViewport& viewport) {
    gvr_buffer_viewport_list_set_item(viewport_list_, index,
                                      viewport.viewport_);
  }

  /// For more information, see gvr_buffer_viewport_list_get_item().
  void GetBufferViewport(size_t index, BufferViewport* viewport) const {
    gvr_buffer_viewport_list_get_item(viewport_list_, index,
                                      viewport->viewport_);
  }

  /// For more information, see gvr_buffer_viewport_list_get_size().
  size_t GetSize() const {
    return gvr_buffer_viewport_list_get_size(viewport_list_);
  }

 private:
  friend class Frame;
  friend class GvrApi;
  friend class SwapChain;

  explicit BufferViewportList(gvr_context* context)
      : context_(context),
        viewport_list_(gvr_buffer_viewport_list_create(context)) {}

  const gvr_context* context_;
  gvr_buffer_viewport_list* viewport_list_;

  // Disallow copy and assign.
  BufferViewportList(const BufferViewportList&) = delete;
  void operator=(const BufferViewportList&) = delete;
};

/// Convenience C++ wrapper for gvr_buffer_spec, an opaque pixel buffer
/// specification. Frees the underlying gvr_buffer_spec on destruction.
class BufferSpec {
 public:
  BufferSpec(BufferSpec&& other)
      : spec_(nullptr) {
    std::swap(spec_, other.spec_);
  }

  BufferSpec& operator=(BufferSpec&& other) {
    std::swap(spec_, other.spec_);
    return *this;
  }
  ~BufferSpec() {
    if (spec_) gvr_buffer_spec_destroy(&spec_);
  }

  /// Gets the buffer's size. The default value is the recommended render
  /// target size. For more information, see gvr_buffer_spec_get_size().
  Sizei GetSize() const {
    return gvr_buffer_spec_get_size(spec_);
  }

  /// Sets the buffer's size. For more information, see
  /// gvr_buffer_spec_set_size().
  void SetSize(const Sizei& size) {
    gvr_buffer_spec_set_size(spec_, size);
  }

  /// Sets the buffer's size to the passed width and height. For more
  /// information, see gvr_buffer_spec_set_size().
  ///
  /// @param width The width in pixels. Must be greater than 0.
  /// @param height The height in pixels. Must be greater than 0.
  void SetSize(int32_t width, int32_t height) {
    gvr_sizei size{width, height};
    gvr_buffer_spec_set_size(spec_, size);
  }

  /// Gets the number of samples per pixel in the buffer. For more
  /// information, see gvr_buffer_spec_get_samples().
  int32_t GetSamples() const { return gvr_buffer_spec_get_samples(spec_); }

  /// Sets the number of samples per pixel. For more information, see
  /// gvr_buffer_spec_set_samples().
  void SetSamples(int32_t num_samples) {
    gvr_buffer_spec_set_samples(spec_, num_samples);
  }

  /// Sets the color format for this buffer. For more information, see
  /// gvr_buffer_spec_set_color_format().
  void SetColorFormat(ColorFormat color_format) {
    gvr_buffer_spec_set_color_format(spec_, color_format);
  }

  /// Sets the depth and stencil format for this buffer. For more
  /// information, see gvr_buffer_spec_set_depth_stencil_format().
  void SetDepthStencilFormat(DepthStencilFormat depth_stencil_format) {
    gvr_buffer_spec_set_depth_stencil_format(spec_, depth_stencil_format);
  }

 private:
  friend class GvrApi;
  friend class SwapChain;
  friend class GvrApiTest;

  explicit BufferSpec(gvr_context* gvr) {
    spec_ = gvr_buffer_spec_create(gvr);
  }

  gvr_buffer_spec* spec_;
};

/// Convenience C++ wrapper for gvr_frame, which represents a single frame
/// acquired for rendering from the swap chain.
class Frame {
 public:
  Frame(Frame&& other) : frame_(nullptr) {
    std::swap(frame_, other.frame_);
  }

  Frame& operator=(Frame&& other) {
    std::swap(frame_, other.frame_);
    return *this;
  }

  ~Frame() {
    // The swap chain owns the frame, so no clean-up is required.
  }

  /// For more information, see gvr_frame_get_buffer_size().
  Sizei GetBufferSize(int32_t index) const {
    return gvr_frame_get_buffer_size(frame_, index);
  }

  /// For more information, see gvr_frame_bind_buffer().
  void BindBuffer(int32_t index) {
    gvr_frame_bind_buffer(frame_, index);
  }

  /// For more information, see gvr_frame_unbind().
  void Unbind() {
    gvr_frame_unbind(frame_);
  }

  /// For more information, see gvr_frame_get_framebuffer_object().
  int32_t GetFramebufferObject(int32_t index) {
    return gvr_frame_get_framebuffer_object(frame_, index);
  }

  /// For more information, see gvr_frame_submit().
  void Submit(const BufferViewportList& viewport_list,
              const Mat4f& world_to_head_transform) {
    gvr_frame_submit(&frame_, viewport_list.viewport_list_,
                     world_to_head_transform);
  }

  /// Returns whether the wrapped gvr_frame reference is valid.
  bool is_valid() const { return frame_ != nullptr; }

  explicit operator bool const() { return is_valid(); }

 private:
  friend class SwapChain;

  explicit Frame(gvr_frame* frame) : frame_(frame) {}
  gvr_frame* frame_;
};

/// Convenience C++ wrapper for gvr_swap_chain, which represents a queue of
/// frames. The GvrApi object must outlive any SwapChain objects created from
/// it.
class SwapChain {
 public:
  SwapChain(SwapChain&& other)
      : swap_chain_(nullptr) {
    std::swap(swap_chain_, other.swap_chain_);
  }

  SwapChain& operator=(SwapChain&& other) {
    std::swap(swap_chain_, other.swap_chain_);
    return *this;
  }

  ~SwapChain() {
    if (swap_chain_) gvr_swap_chain_destroy(&swap_chain_);
  }

  /// For more information, see gvr_swap_chain_get_buffer_count().
  int32_t GetBufferCount() const {
    return gvr_swap_chain_get_buffer_count(swap_chain_);
  }

  /// For more information, see gvr_swap_chain_get_buffer_size().
  Sizei GetBufferSize(int32_t index) const {
    return gvr_swap_chain_get_buffer_size(swap_chain_, index);
  }

  /// For more information, see gvr_swap_chain_resize_buffer().
  void ResizeBuffer(int32_t index, Sizei size) {
    gvr_swap_chain_resize_buffer(swap_chain_, index, size);
  }

  /// For more information, see gvr_swap_chain_acquire_frame().
  /// Note that if Frame acquisition fails, the returned Frame may not be valid.
  /// The caller should inspect the returned Frame's validity before using,
  /// and reschedule frame acquisition upon failure.
  Frame AcquireFrame() {
    Frame result(gvr_swap_chain_acquire_frame(swap_chain_));
    return result;
  }

 private:
  friend class GvrApi;

  SwapChain(gvr_context* gvr, const std::vector<BufferSpec>& specs) {
    std::vector<const gvr_buffer_spec*> c_specs;
    for (const auto& spec : specs)
      c_specs.push_back(spec.spec_);
    swap_chain_ = gvr_swap_chain_create(gvr, c_specs.data(),
                                        static_cast<int32_t>(c_specs.size()));
  }

  gvr_swap_chain* swap_chain_;

  // Disallow copy and assign.
  SwapChain(const SwapChain&);
  void operator=(const SwapChain&);
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
///     gvr::BufferViewportList viewport_list =
///         gvr->CreateEmptyBufferViewportList();
///     gvr->GetRecommendedBufferViewports(&viewport_list);
///     gvr::BufferViewport left_eye_viewport = gvr->CreateBufferViewport();
///     gvr::BufferViewport right_eye_viewport = gvr->CreateBufferViewport();
///     viewport_list.Get(0, &left_eye_view);
///     viewport_list.Get(1, &right_eye_view);
///
///     std::vector<gvr::BufferSpec> specs;
///     specs.push_back(gvr->CreateBufferSpec());
///     specs[0].SetSamples(app_samples);
///     gvr::SwapChain swap_chain = gvr->CreateSwapChain(specs);
///
///     while (client_app_should_render) {
///       // A client app should be ready for the render target size to change
///       // whenever a new QR code is scanned, or a new viewer is paired.
///       gvr::Sizei render_target_size =
///           gvr->GetRecommendedRenderTargetSize();
///       swap_chain.ResizeBuffer(0, render_target_size);
///       gvr::Frame frame = swap_chain.AcquireFrame();
///       while (!frame) {
///         std::this_thread::sleep_for(2ms);
///         frame = swap_chain.AcquireFrame();
///       }
///
///       // This function will depend on your render loop's implementation.
///       gvr::ClockTimePoint next_vsync = AppGetNextVsyncTime();
///
///       const gvr::Mat4f head_pose =
///           gvr->GetHeadPoseInStartSpace(next_vsync);
///       const gvr::Mat4f left_eye_pose = MatrixMultiply(
///           gvr->GetEyeFromHeadMatrix(kLeftEye), head_pose);
///       const gvr::Mat4f right_eye_pose = MatrixMultiply(
///           gvr->GetEyeFromHeadMatrix(kRightEye), head_pose);
///
///       frame.BindBuffer(0);
///       // App does its rendering to the current framebuffer here.
///       AppDoSomeRenderingForEye(
///           left_eye_viewport.GetSourceUv(), left_eye_pose);
///       AppDoSomeRenderingForEye(
///           right_eye_viewport.GetSourceUv(), right_eye_pose);
///       frame.Unbind();
///       frame.Submit(viewport_list, head_matrix);
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

  /// Gets the underlying gvr_context instance.
  ///
  /// @return The wrapped gvr_context.
  gvr_context* GetContext() { return context_; }

  /// Gets the underlying gvr_context instance.
  ///
  /// @return The wrapped gvr_context.
  const gvr_context* GetContext() const { return context_; }

  /////////////////////////////////////////////////////////////////////////////
  // Rendering
  /////////////////////////////////////////////////////////////////////////////

  /// For more information, see gvr_initialize_gl().
  void InitializeGl() { gvr_initialize_gl(context_); }

  /// For more information, see gvr_get_async_reprojection_enabled().
  bool GetAsyncReprojectionEnabled() const {
    return gvr_get_async_reprojection_enabled(context_);
  }

  /// Constructs a C++ wrapper for a gvr_buffer_viewport object.  For more
  /// information, see gvr_buffer_viewport_create().
  ///
  /// @return A new BufferViewport instance with memory allocated for an
  ///     underlying gvr_buffer_viewport.
  BufferViewport CreateBufferViewport() const {
    return BufferViewport(context_);
  }

  /// Constructs a C++ wrapper for a gvr_buffer_viewport_list object.
  /// For more information, see gvr_buffer_viewport_list_create().
  ///
  /// @return A new, empty BufferViewportList instance.
  ///     Note: The validity of the returned object is closely tied to the
  ///     lifetime of the member gvr_context. The caller is responsible for
  ///     ensuring correct usage accordingly.
  BufferViewportList CreateEmptyBufferViewportList() const {
    return BufferViewportList(context_);
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
  void DistortToScreen(int32_t texture_id,
                       const BufferViewportList& viewport_list,
                       const Mat4f& rendered_head_pose_in_start_space_matrix,
                       const ClockTimePoint& texture_presentation_time) {
    gvr_distort_to_screen(context_, texture_id, viewport_list.viewport_list_,
                          rendered_head_pose_in_start_space_matrix,
                          texture_presentation_time);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Offscreen Framebuffers
  /////////////////////////////////////////////////////////////////////////////

  BufferSpec CreateBufferSpec() {
    return BufferSpec(context_);
  }

  SwapChain CreateSwapchain(const std::vector<BufferSpec>& specs) {
    return SwapChain(context_, specs);
  }

  /// For more information, see gvr_set_default_framebuffer_active().
  void SetDefaultFramebufferActive() {
    gvr_set_default_framebuffer_active(context_);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Head tracking
  /////////////////////////////////////////////////////////////////////////////

  /// For more information see gvr_get_head_pose_in_start_space.
  ///
  /// @param time_point The time at which to calculate the head pose in start
  ///     space.
  /// @return The matrix representation of the head pose in start space.
  Mat4f GetHeadPoseInStartSpace(const ClockTimePoint& time_point) {
      return gvr_get_head_pose_in_start_space(context_, time_point);
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
#endif  // #if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)

#endif  // VR_GVR_CAPI_INCLUDE_GVR_H_
