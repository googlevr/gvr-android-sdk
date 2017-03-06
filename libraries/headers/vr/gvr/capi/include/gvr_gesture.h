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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_GESTURE_H_
#define VR_GVR_CAPI_INCLUDE_GVR_GESTURE_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

#if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)
#include <memory>
#endif

#include <stdint.h>
#include "vr/gvr/capi/include/gvr_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup Gesture Gesture C API
/// @brief
/// The Gesture C API allows clients to recognize touchpad and button
/// gestures from a controller. Disclaimer: This API is experimental and is
/// subject to changes.
///
/// If you are writing C++ code, you might prefer to use the C++ wrapper rather
/// than implement this C API directly.
///
/// Example API usage:
///
/// Initialization:
///
///     // Create the gesture context.
///     gvr_gesture_context* context = gvr_gesture_context_create();
///
/// Usage:
///
///     // Get the controller state from client.
///     gvr_controller_state* controller_state = ...
///
///     // Detect the gestures.
///     gvr_gesture_update(context, controller_state);
///
///     // Get the number of detected gestures.
///     int num_gestures = gvr_gesture_get_count(context);
///
///     for (int i = 0; i < num_gestures; i ++) {
///       gvr_gesture* gesture_ptr = gvr_gesture_get(context, i);
///       switch (gvr_gesture_get_type(gesture_ptr)) {
///         case GVR_GESTURE_SWIPE:
///           // Handle swipe gesture.
///           break;
///         case GVR_GESTURE__SCROLL_START:
///           // Handle the start of a sequence of scroll gestures.
///           break;
///         case GVR_GESTURE__SCROLL_UPDATE:
///           // Handle an update in a sequence of scroll gestures.
///           break;
///         case GVR_GESTURE__SCROLL_END:
///           // Handle the end of a sequence of scroll gestures.
///           break;
///         default:
///           // Unexpected gesture type.
///           break;
///        }
///     }
///
/// @{

/// Gesture types.
typedef enum {
  /// Finger moves quickly across the touch pad. This is a transient state that
  /// is only true for a single frame.
  GVR_GESTURE_SWIPE = 1,
  /// Finger starts scrolling on touch pad. This is a transient state that is
  /// only true for a single frame.
  GVR_GESTURE_SCROLL_START = 2,
  /// Finger is in the process of scrolling.
  GVR_GESTURE_SCROLL_UPDATE = 3,
  /// Finger stops scrolling. This is a transient state that is only true for a
  /// single frame.
  GVR_GESTURE_SCROLL_END = 4
} gvr_gesture_type;

/// Gesture directions.
typedef enum {
  /// Finger moves up on the touch pad.
  GVR_GESTURE_DIRECTION_UP = 1,
  /// Finger moves down on the touch pad.
  GVR_GESTURE_DIRECTION_DOWN = 2,
  /// Finger moves left on the touch pad.
  GVR_GESTURE_DIRECTION_LEFT = 3,
  /// Finger moves right on the touch pad.
  GVR_GESTURE_DIRECTION_RIGHT = 4
} gvr_gesture_direction;

/// Opaque handle to gesture context.
typedef struct gvr_gesture_context_ gvr_gesture_context;

/// Opaque handle to gesture detector.
typedef struct gvr_gesture_ gvr_gesture;

/// Creates and initializes a gesture context instance which can be used to
/// invoke the gesture API functions.
///
/// @return A pointer to the created gesture context instance.
gvr_gesture_context* gvr_gesture_context_create();

/// Destroys the gesture context.
///
/// @param context A pointer to a pointer to the gesture context instance. The
/// pointer will be set to null after destruction.
void gvr_gesture_context_destroy(gvr_gesture_context** context);

/// Restarts gesture detection. This should be used whenever the user does not
/// lift up the finger from the controller touchpad, but the gesture detection
/// needs to be restarted (e.g. the controller reticle moves from a
/// non-scrollable region to a scrollable region).
///
/// @param context A pointer to the gesture context instance.
void gvr_gesture_restart(gvr_gesture_context* context);

/// Updates the gesture context based on controller state.
///
/// @param controller_state A pointer to the controller state instance.
/// @param context A pointer to the gesture context instance.
void gvr_gesture_update(const gvr_controller_state* controller_state,
                        gvr_gesture_context* context);

/// Returns the number of gestures detected in previous gvr_gesture_update().
///
/// @param context A pointer to the gesture context instance.
/// @return The number of detected gestures.
int gvr_gesture_get_count(const gvr_gesture_context* context);

/// Returns the gesture with the given index.
/// The returned pointer remains valid only until the list is modified i.e. by
/// calling gvr_gesture_update() or gvr_gesture_context_destroy().
///
/// @param context A pointer to the gesture context instance.
/// @param index The index of the gesture to be returned.
/// @return A pointer to the gesture instance at the given index.
const gvr_gesture* gvr_gesture_get(const gvr_gesture_context* context,
                                   int index);

/// Returns the type (swipe/scroll_start/scroll_update/scroll_end) of given
/// gesture.
///
/// @param gesture A pointer to the gesture instance.
/// @return The type of the given gesture.
gvr_gesture_type gvr_gesture_get_type(const gvr_gesture* gesture);

/// Returns the direction (up/down/left/right) of given gesture.
///
/// @param gesture A pointer to the gesture instance.
/// @return The direction of the given gesture.
gvr_gesture_direction gvr_gesture_get_direction(const gvr_gesture* gesture);

/// Returns the velocity (in normalized distance / second) of the given gesture,
/// where (0,0) is the top-left of the touchpad and (1,1) is the bottom right.
///
/// @param gesture A pointer to the gesture instance.
/// @return The velocity of the given gesture.
gvr_vec2f gvr_gesture_get_velocity(const gvr_gesture* gesture);

/// Returns the displacement (in touchpad unit) of given gesture.
///
/// @param gesture A pointer to the gesture instance.
/// @return The displacement of the given gesture.
gvr_vec2f gvr_gesture_get_displacement(const gvr_gesture* gesture);

/// Returns whether a long press on controller button has been detected.
///
/// @param controller_state A pointer to the controller state instance.
/// @param context A pointer to the gesture context instance.
/// @param button A controller button type.
/// @return Whether the given button has been long pressed.
bool gvr_get_button_long_press(const gvr_controller_state* controller_state,
                               const gvr_gesture_context* context,
                               gvr_controller_button button);

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif

// Convenience C++ wrapper.
#if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)

namespace gvr {

typedef gvr_gesture_direction GestureDirection;
typedef gvr_gesture_type GestureType;
typedef gvr_gesture Gesture;
typedef gvr_gesture_context GestureContext;

/// This is a convenience C++ wrapper for the Gesture C API.
///
/// This wrapper strategy prevents ABI compatibility issues between compilers
/// by ensuring that the interface between client code and the implementation
/// code in libgvr_gesture.so is a pure C interface. The translation from C++
/// calls to C calls provided by this wrapper runs entirely in the client's
/// binary and is compiled by the client's compiler.
///
/// Methods in this class are only documented insofar as the C++ wrapping logic
/// is concerned; for information about the method itself, please refer to the
/// corresponding function in the C API.
///
/// Example API usage:
///
/// Initialization:
///
///     GestureApi gesture_api;
///
/// Usage:
///
///     // Get the controller state from client
///     ControllerState* controller_state = ...
///
///     // Detect the gestures.
///     gesture_api.Update(controller_state);
///
///     // Get the number of detected gestures
///     int num_gestures = gesture_api.GetGestureCount();
///
///     for (int i = 0; i < num_gestures; i ++) {
///       Gesture* gesture_ptr = gesture_api.GetGesture(i);
///       switch (gesture_api.GetGestureType(gesture_ptr)) {
///         case GVR_GESTURE_SWIPE:
///           // Handle swipe gesture.
///           break;
///         case GVR_GESTURE__SCROLL_START:
///           // Handle the start of a sequence of scroll gestures.
///           break;
///         case GVR_GESTURE__SCROLL_UPDATE:
///           // Handle an update in a sequence of scroll gestures.
///           break;
///         case GVR_GESTURE__SCROLL_END:
///           // Handle the end of a sequence of scroll gestures.
///           break;
///         default:
///           // Unexpected gesture type.
///           break;
///        }
///     }
///
class GestureApi {
 public:
  /// Creates a GestureApi object. It is properly initialized and can handle
  /// gesture detection immediately.
  GestureApi() { gesture_context_ = gvr_gesture_context_create(); }

  /// Destroys this GestureApi instance.
  ~GestureApi() {
    if (gesture_context_) gvr_gesture_context_destroy(&gesture_context_);
  }

  /// Restarts gesture detection.
  /// For more information, see gvr_gesture_restart().
  void Restart() { gvr_gesture_restart(gesture_context_); }

  /// Updates gesture context based on current controller state.
  /// For more information, see gvr_gesture_update().
  void Update(const ControllerState* controller_state) {
    gvr_gesture_update(controller_state->cobj(), gesture_context_);
  }

  /// Returns the number of gestures detected.
  /// For more information, see gvr_gesture_get_count().
  int GetGestureCount() { return gvr_gesture_get_count(gesture_context_); }

  /// Returns the gesture at given index.
  /// For more information, see gvr_gesture_get().
  const Gesture* GetGesture(int index) {
    return gvr_gesture_get(gesture_context_, index);
  }

  /// Returns the type of the given gesture.
  /// For more information, see gvr_gesture_get_type().
  GestureType GetGestureType(const Gesture* gesture) {
    return gvr_gesture_get_type(gesture);
  }

  /// Returns the direction of current gesture.
  /// For more information, see gvr_gesture_get_direction().
  GestureDirection GetGestureDirection(const Gesture* gesture) {
    return gvr_gesture_get_direction(gesture);
  }

  /// Returns gesture velocity.
  /// For more information, see gvr_gesture_get_velocity().
  gvr_vec2f GetVelocity(const Gesture* gesture) {
    return gvr_gesture_get_velocity(gesture);
  }

  /// Returns gesture displacement.
  /// For more information, see gvr_gesture_get_displacement().
  gvr_vec2f GetDisplacement(const Gesture* gesture) {
    return gvr_gesture_get_displacement(gesture);
  }

  /// Returns whether long press on the given controller button is detected.
  /// For more information, see gvr_get_button_long_press().
  bool GetButtonLongPress(const ControllerState* controller_state,
                          ControllerButton button) {
    return gvr_get_button_long_press(controller_state->cobj(), gesture_context_,
                                     button);
  }

 private:
  GestureContext* gesture_context_;

  // Disallows copy and assign:
  GestureApi(const GestureApi&);
  void operator=(const GestureApi&);
};

}  // namespace gvr

#endif  // #if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)

#endif  // VR_GVR_CAPI_INCLUDE_GVR_GESTURE_H_
