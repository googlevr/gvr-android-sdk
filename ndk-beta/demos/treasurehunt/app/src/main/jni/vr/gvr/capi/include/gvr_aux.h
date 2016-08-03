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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_AUX_H_
#define VR_GVR_CAPI_INCLUDE_GVR_AUX_H_

#include "vr/gvr/capi/include/gvr_aux_types.h"
#include "vr/gvr/capi/include/gvr_types.h"

#ifdef __cplusplus
#include <vector>
#include <cmath>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup Distortion Google VR Auxillary Library Distortion Commands.
/// @brief This is auxillary functionality and intended for use with the
/// Google VR Base C API, specifically dealing with distortion -- i.e., the
/// calculation of distortion and its inversion.  At present this includes:
///
/// @{

/// Calculates a distortion factor for a given provided radius.  The distortion
/// factor is how much distortion would be applied to that radius, if that
/// radius were to be distorted.  It is used to distort() a radius, below.
///
/// @param radius The radius to calculate a distortion factor for, in tan
///     angle units.
/// @param coefficients The coefficients to be used in the distortion.
/// @param num_coefficients The number of coeffficients included.
///
/// @return The distortion factor of the radius given the provided coefficients.
float gvr_aux_distortion_factor(
     float radius, const float* coefficients, int32_t num_coeffiricents);

/// Calculates distortion of a radius, given a set of coefficients.  Distortion
/// is calculated by applying the distortion_factor() for the provided radius to
/// that radius.
///
/// @param radius The radius to be distorted, in tan-angle units.
/// @param coefficients The coefficients to be used in the distortion.
/// @param num_coefficients The number of coefficients included.
///
/// @return The distorted radius calculated with the provided coefficients, in
///     tan-angle units.
float gvr_aux_distort(
    float radius, const float* coefficients, int32_t num_coefficients);

/// Calculates the inverse of a distorted radius (i.e., given a distorted
/// radius and information about the coefficients used to distort it, calculate
/// the original radius to which the distortion was applied.  This function
/// uses the secant method to solve for the inverse distortion.  It is accurate
/// but quite slow.
///
/// @param radius The distorted radius to be inverted, in tan-angle units.
/// @param coefficients The coefficients that were used in the distortion.
/// @param num_coefficients The number of coefficients included.
///
/// @return The original (pre-distortion) radius, in tan-angle units.
float gvr_aux_distort_inverse(
    float radius, const float* coefficients, int32_t num_coefficients);

/// Calculates a set of coefficients to describe the inverse distortion, given
/// a set of forward distortion coefficients, a max radius, and a number of
/// of samples.  Uses the Least Squares Method to effect the solution, and the
/// distort() function to sample.
///
/// @param forward_coefficients The coefficients used for forward distortion.
/// @param num_forward_coefficients The number of coefficients used for forward
///     distortion.
/// @param max_radius The maximum supported radius in tan-angle units in lens
///     space (after applying barrel distortion).  Should be set to the inverse
///     distortion of tan(max expected FOV half angle) to create an inverse
///     that will be usable for inputs up to tan(max expected FOV half hangle).
/// @param num_samples The number of samples that the Least Squares Method
///     should operate over.  More samples will be slower, but more accurate.
///     We use a hundred samples in our calculations.
/// @param inverse_coefficients Where the calculated inverse coefficients will
///     be written.
/// @param num_inverse_coefficients The number of inverse coefficients to be
///     calculated.  Does not need to match the number of forward coefficients.
///     We recommended six coefficients for Cardboard V2, as more can become
///     numerically unstable.  The number of inverse coefficients requested
///     should be strictly less than the number of samples.
///
/// @return True if the distortion was successfully calculated.  False
///     otherwise.  Distortion calculation could fail if the number of samples
///     is not sufficient for the number of inverse coefficients requested.
bool gvr_aux_invert_distortion(
    const float* forward_coefficients, int32_t num_forward_coefficients,
    float max_radius, int32_t num_samples,
    float* inverse_coefficients /* output */, int32_t num_inverse_coefficients);

/// Calculates distortion data for a given device/phone combination, with a
/// given set of render preferences.  See gvr_aux_headset_descriptor,
/// gvr_render_prefs and gvr_distortion_data for more info on this structures.
///
/// @param headset_descriptor A description of the device to be considered.
///     Includes the profile params URI, user params, and phone params.
/// @param render_prefs The user-specified render preferences, including data
///     about the type of distortion to use, and the polynomial degree.
// TODO(b/29998683): Revisit need for this parameter.
/// @param distortion_data The distortion data for the given device descriptor
///     and render preferences.  Includes recommended render parameters, and
///     the forward coefficients that will be used for distortion.
// TODO(b/30000067): Is this where the mesh gets returned?
void gvr_aux_get_distortion_data(
    const gvr_aux_headset_descriptor headset_descriptor,
    const gvr_aux_render_prefs render_prefs,
    gvr_aux_distortion_data* distortion_data /* output */);
/// @}


#ifdef __cplusplus
}  // extern "C"
#endif

#if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)
namespace gvr_aux {
/// Convenience C++ wrapper for the distortion calculations.
class Distortion {
 public:
  /// Constructs a distortion with a particular set of coefficients.
  explicit Distortion(std::vector<float>(coefficients))
     : coefficients_(std::move(coefficients)) {}

  /// Returns the distortion factor (how much the radius will be distorted by)
  /// for the given radius.  See gvr_aux_distortion_factor for more info.
  float DistortionFactor(float radius) {
    return gvr_aux_distortion_factor(
        radius, coefficients_.data(),
        static_cast<int32_t>(coefficients_.size()));
  }

  /// Distorts a radius (in tan-angle space), with the stored coefficients.
  /// See gvr_aux_distort for more info on the distortion.
  float Distort(float radius) {
    return gvr_aux_distort(radius, coefficients_.data(),
                           static_cast<int32_t>(coefficients_.size()));
  }

  /// Given a distorted radius (in tan-angle space), calculate the radius that
  /// -- when distorted with the stored coefficients -- returns the given
  /// distorted radius.  See gvr_aux_distort_inverse for more info on this
  /// inverse distortion.
  float DistortInverse(float radius) {
    return gvr_aux_distort_inverse(
        radius, coefficients_.data(),
        static_cast<int32_t>(coefficients_.size()));
  }

  /// Return a distortion with coefficients calculated to invert the distortion
  /// described by this class.
  Distortion Invert(
      float max_half_fov, int32_t num_samples, int32_t num_coefficients) {
    float max_radius = DistortInverse(std::tan(max_half_fov));
    std::vector<float> inverse_coefficients;
    inverse_coefficients.resize(num_coefficients);
    gvr_aux_invert_distortion(
        coefficients_.data(), static_cast<int32_t>(coefficients_.size()),
        max_radius, num_samples, inverse_coefficients.data(),
        static_cast<int32_t>(inverse_coefficients.size()));
    Distortion inversion(inverse_coefficients);
    return inversion;
  }

  /// Return the distortion coefficients being used by this class.
  const std::vector<float>& GetCoefficients() {
    return coefficients_;
  }

 private:
  /// Coefficients for this distortion.
  std::vector<float> coefficients_;
};

}  // namespace gvr_aux
#endif  // #ifdef __cplusplus

#endif  // VR_GVR_CAPI_INCLUDE_GVR_AUX_H_
