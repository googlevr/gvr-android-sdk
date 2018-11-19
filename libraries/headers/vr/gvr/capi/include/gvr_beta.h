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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_BETA_H_
#define VR_GVR_CAPI_INCLUDE_GVR_BETA_H_

#include "vr/gvr/capi/include/gvr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup Beta GVR Beta APIs
///
/// @brief These are experimental APIs that are likely to change between
/// releases.
///
/// This file contains functions that are part of the Daydream Labs APIs. These
/// APIs are public, but there is no guarantee about how long they will be
/// supported. They may also be using functionality that is not available for
/// all users of a given platform.

/// @{

// ************************************************************************** //
// *                           6DOF Controllers                             * //
// ************************************************************************** //

/// Configuration for a specific controller
typedef enum {
  /// Used when controller configuration is unknown.
  GVR_BETA_CONTROLLER_CONFIGURATION_UNKNOWN = 0,
  /// 3DOF controller. This controller may have simulated position data.
  GVR_BETA_CONTROLLER_CONFIGURATION_3DOF = 1,
  /// 6DOF controller.
  GVR_BETA_CONTROLLER_CONFIGURATION_6DOF = 2,
} gvr_beta_controller_configuration_type;

/// Tracking state for 6DOF controllers.
typedef enum {
  /// Indicates whether the controller's tracking status is unknown.
  GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_TRACKING_UNKNOWN = (1U << 0),
  /// Indicates whether the controller is tracking in full 6DOF mode.
  GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_TRACKING_NOMINAL = (1U << 1),
  /// Indicates whether the controller is occluded. For optically tracked
  /// controllers, occlusion happens for a brief time as the user blocks the
  /// tracking system with their hand or the other controller. The position of
  /// the controller will be clamped to the last known value.
  GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_OCCLUDED = (1U << 2),
  /// Indicates whether the controller is out of field of view of the tracking
  /// system. The position of the controller is no longer accurate and should be
  /// ignored.
  GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_OUT_OF_FOV = (1U << 3),
} gvr_beta_controller_tracking_status_flags;

/// Returns the configuration of the given controller.
///
/// @param controller_context Controller API context.
/// @param state The controller to query.
/// @return the current controller configuration of type
/// |gvr_beta_controller_configuration_type|.
int32_t gvr_beta_controller_get_configuration_type(
    const gvr_controller_context* controller_context,
    const gvr_controller_state* state);

/// Returns the tracking status of the controller. The status is an OR'd
/// combination of GVR_BETA_CONTROLLER_TRACKING_STATUS_FLAG_* flags.
///
/// @param state The controller state to get the tracking status from.
/// @return The set of flags representing the current the tracking status.
int32_t gvr_beta_controller_state_get_tracking_status(
    const gvr_controller_state* state);

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VR_GVR_CAPI_INCLUDE_GVR_BETA_H_
