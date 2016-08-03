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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_AUX_TYPES_H_
#define VR_GVR_CAPI_INCLUDE_GVR_AUX_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#include "vr/gvr/capi/include/gvr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO(b/30000064): Do a future proofing review of this API.
/// @defgroup types Google VR Auxillary Types
/// @brief Various types used in the Google VR NDK.
/// @{

// Represents a device description, including, device parameters, phone
// parameters, and any user parameters or ovverides.  Used specifically to
// request information on distortion calculations that would be made given
// the specific configuration specified.

// TODO(b/29999167): Handle non-radial distortion types, specifically builtin
// and no lens distortion, per design.

/// Enumeration of distortion types that can be calculated.
typedef enum {
  /// Represents a pincushion distortion.  This is the inverse of the standard
  /// forward-distortion used for most HMDs.
  GVR_PINCUSHION = 0,

  /// Represents a barrel distortion.  This is the standard forward-distortion
  /// used for most HMDs.
  GVR_BARREL = 1,

  /// The number of distortion types supported.
  GVR_DISTORTION_TYPE_COUNT = 2
} gvr_aux_distortion_type;

/// Parameters representing a particular phone, for use in calculations which
/// require the specification of a device-phone combination (i.e., the
/// calculation of distorion).
typedef struct gvr_aux_phone_params_ {
  /// The absolute display height in pixels.
  int32_t height_pixels;

  /// Absolute display width in pixels.
  int32_t width_pixels;

  /// Physical screen pixels per inch in X.
  float x_dpi;

  /// Physical screen pixels per inch in Y.
  float y_dpi;
} gvr_aux_phone_params;

/// The description of a particular viewing headset, which is a combination of
/// the HMD (e.g., Cardboard V2 with) and viewing device (e.g., Nexus 6P).
typedef struct gvr_aux_headset_descriptor_ {
  /// The profile paramters for the head mounted device (HMD) as a URI.
  const char* profile_params;

  /// The parameters of a particular phone to be used in relevant calculations
  /// (i.e., distortion).
  gvr_aux_phone_params phone_params;

  // TODO(b/29998683): Revisit what needs to go in here, and add comment or
  // remove.  Maybe change type. Pairable?  Pluggable.  Alignable?  Touchable?
  char* user_params;
} gvr_aux_headset_descriptor;

// TODO(b/30000067): Use this struct to return the distorted mesh.  This struct
// may need some adaptation -- i.e., remove slice and row pitch?  Also revisit
// the comments for those members if we do need them.
/// The description of a vertex buffer.
typedef struct gvr_aux_vertex_buffer {
  /// The actual data in the vertex buffer, as a series of floats.
  float* data;

  /// Offset into the data where the vertices start.
  int32_t offset;

  /// Number of bytes in the data for a single vertex, including any padding.
  /// May be negative if the image is stored bottom-up.
  int32_t stride;

  /// Distance between two entries in an array. (directx)
  /// Scan line pitch in bytes (Khronos).
  int32_t rowpitch;

  /// Distance to the next slice.
  /// Must be a multiple of row_pitch. (khronos0).
  int32_t slicepitch;
} gvr_aux_vertex_buffer;

/// Render preferences specified by the user.
typedef struct gvr_aux_render_prefs_ {
  /// The type of distortion the user wants.  This should be set using
  /// the gvr_aux_distortion_type enum, but is stored as an int32 to keep
  /// the enum size and padding constant across compilers regardless of
  /// future changes in the api.
  int32_t distortion_type;

  /// The degree of the polynomial that should be used for distortion.  This
  /// is the client's request for a particular polynomial degree.  It will be
  /// filled if possible.  Forward distortion polynomial degree is limited to
  /// the degree provide in the device profile -- i.e., we cannot add a higher
  /// degree of precision than we are given.
  // TODO(b/30000368): Sanity check that this value is reasonable.
  int32_t polynomial_degree;
} gvr_aux_render_prefs;

/// Distortion and rendering data returned to the user.
// TODO(b/30000067): This should contain the mesh data?  Review.
typedef struct gvr_aux_distortion_data_ {
  /// The viewport list to use for rendering with this head mounted device,
  /// phone, and render preference combination.
  gvr_buffer_viewport_list* viewport_list;

  /// Pointer to an array of coefficients that should be used to approximate
  /// distortion.   Memory should be available for one floating point value for
  /// each coefficient requested.
  float* coefficients;

  /// The number of coefficients that have been provided above.
  int32_t num_coefficients;
} gvr_aux_distortion_data;

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VR_GVR_CAPI_INCLUDE_GVR_AUX_TYPES_H_
