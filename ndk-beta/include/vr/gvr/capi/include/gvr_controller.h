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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_CONTROLLER_H_
#define VR_GVR_CAPI_INCLUDE_GVR_CONTROLLER_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

#include <stdint.h>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup Controller Controller API
/// @brief This is the Controller C API, which allows access to a VR controller.
///
/// If you are writing C++ code, you might prefer to use the C++ wrapper rather
/// than implement this C API directly.
///
/// Typical initialization example:
///
///     // Get your gvr_context* pointer from GvrLayout:
///     gvr_context* gvr = ......;  // (get from GvrLayout in Java)
///
///     // Set up the API options:
///     gvr_controller_api_options options;
///     gvr_controller_init_default_options(&options);
///
///     // Enable non-default options, if needed:
///     options.enable_gyro = true;
///
///     // Create and init:
///     gvr_controller_context* context =
///         gvr_controller_create_and_init(options, gvr);
///     if (!context) {
///       // Handle error.
///       return;
///     }
///
///     // Resume:
///     gvr_controller_resume(api);
///
/// Usage:
///
///     void DrawFrame() {
///       gvr_controller_state state;
///       gvr_controller_read_state(context, &state);
///       // ... process controller state ...
///     }
///
///     // When your application gets paused:
///     void OnPause() {
///       gvr_controller_pause(context);
///     }
///
///     // When your application gets resumed:
///     void OnResume() {
///       gvr_controller_resume(context);
///     }
///
/// To conserve battery, be sure to call gvr_controller_pause and
/// gvr_controller_resume when your app gets paused and resumed, respectively.
///
/// THREADING: unless otherwise noted, all functions are thread-safe, so
/// you can operate on the same gvr_controller_context object from multiple
/// threads.
/// @{

/// Represents a Daydream Controller API object, used to invoke the
/// Daydream Controller API.
typedef struct gvr_controller_context_ gvr_controller_context;

/// Initializes the default API options in *options.
///
/// @param options Pointer to an options struct which will be modified by
///     this method.
void gvr_controller_init_default_options(gvr_controller_api_options* options);

/// Creates and initializes a gvr_controller_context instance which can be used
/// to invoke the Daydream Controller API functions. Important: after creation
/// the API will be in the paused state (the controller will be inactive).
/// You must call gvr_controller_resume() explicitly (typically, in your Android
/// app's onResume() callback).
///
/// @param options The API options.
/// @param context The GVR Context object to sync with (optional).
///     This can be nullptr. If provided, the context's state will
///     be synchronized with the controller's state where possible. For
///     example, when the user recenters the controller, this will
///     automatically recenter head tracking as well.
///     WARNING: the caller is responsible for making sure the pointer
///     remains valid for the lifetime of this object.
/// @return A pointer to the initialized API, or NULL if an error occurs.
gvr_controller_context* gvr_controller_create_and_init(
    const gvr_controller_api_options& options, gvr_context* context);

#ifdef __ANDROID__
/// Creates and initializes a gvr_controller_context instance with an explicit
/// Android context and class loader.
///
/// @param env The JNI Env associated with the current thread.
/// @param android_context The Android application context. This must be the
///     application context, NOT an Activity context (Note: from any Android
///     Activity in your app, you can call getApplicationContext() to
///     retrieve the application context).
/// @param class_loader The class loader to use when loading Java
///     classes. This must be your app's main class loader (usually
///     accessible through activity.getClassLoader() on any of your Activities).
/// @param options The API options.
/// @param context The GVR Context object to sync with (optional).
///     This can be nullptr. If provided, the context's state will
///     be synchronized with the controller's state where possible. For
///     example, when the user recenters the controller, this will
///     automatically recenter head tracking as well.
///     WARNING: the caller is responsible for making sure the pointer
///     remains valid for the lifetime of this object.
/// @return A pointer to the initialized API, or NULL if an error occurs.
gvr_controller_context* gvr_controller_create_and_init_android(
    JNIEnv *env, jobject android_context, jobject class_loader,
    const gvr_controller_api_options& options, gvr_context* context);
#endif  // #ifdef __ANDROID__

/// Destroys a gvr_controller_context that was previously created with
/// gvr_controller_init.
///
/// @param api Pointer to a pointer to a gvr_controller_context. The pointer
///     will be set to NULL after destruction.
void gvr_controller_destroy(gvr_controller_context** api);

/// Pauses the controller, possibly releasing resources.
/// Call this when your app/game loses focus.
/// Calling this when already paused is a no-op.
/// Thread-safe (call from any thread).
///
/// @param api Pointer to a pointer to a gvr_controller_context.
void gvr_controller_pause(gvr_controller_context* api);

/// Resumes the controller. Call this when your app/game regains focus.
/// Calling this when already resumed is a no-op.
/// Thread-safe (call from any thread).
///
/// @param api Pointer to a pointer to a gvr_controller_context.
void gvr_controller_resume(gvr_controller_context* api);

/// Reads the controller state. Reading the controller state is not a
/// const getter: it has side-effects. In particular, some of the
/// gvr_controller_state fields (the ones documented as "transient") represent
/// one-time events and will be true for only one read operation, and false
/// in subsequente reads.
///
/// @param api Pointer to a pointer to a gvr_controller_context.
/// @param out_state A caller-allocated pointer where the controller's state
///     is to be written.
void gvr_controller_read_state(gvr_controller_context* api,
                               gvr_controller_state* out_state);

/// Convenience to convert an API status code to string. The returned pointer
/// is static and valid throughout the lifetime of the application.
///
/// @param status The status to convert to string.
/// @return A pointer to a string that describes the value.
const char* gvr_controller_api_status_to_string(
    gvr_controller_api_status status);

/// Convenience to convert an connection state to string. The returned pointer
/// is static and valid throughout the lifetime of the application.
///
/// @param state The state to convert to string.
/// @return A pointer to a string that describes the value.
const char* gvr_controller_connection_state_to_string(
    gvr_controller_connection_state state);

/// Convenience to convert an connection state to string. The returned pointer
/// is static and valid throughout the lifetime of the application.
///
/// @param button The state to convert to string.
/// @return A pointer to a string that describes the value.
const char* gvr_controller_button_to_string(gvr_controller_button button);

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif


// Convenience C++ wrapper.
#ifdef __cplusplus

#include <memory>

namespace gvr {
/// This is a convenience C++ wrapper for the Controller C API.
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
/// Typical C++ initialization example:
///
///     std::unique_ptr<ControllerApi> controller_api(new ControllerApi);
///
///     // Your GVR context pointer (which can be obtained from GvrLayout)
///     gvr_context* context = .....;  // (get it from GvrLayout)
///
///     // Set up the options:
///     ControllerApiOptions options;
///     ControllerApi::InitDefaultOptions(&options);
///
///     // Enable non-default options, if you need them:
///     options.enable_gestures = true;
///     options.enable_accel = true;
///
///     // Init the ControllerApi object:
///     bool success = controller_api->Init(options, context);
///     if (!success) {
///       // Handle failure.
///       // Do NOT call other methods (like Resume, etc) if init failed.
///       return;
///     }
///
///     // Resume the ControllerApi (if your app is on the foreground).
///     controller_api->Resume();
///
/// Usage example:
///
///     void DrawFrame() {
///       ControllerState state;
///       controller_api->ReadState(&state);
///       // ... process controller state ...
///     }
///
///     // When your application gets paused:
///     void OnPause() {
///       controller_api->Pause();
///     }
///
///     // When your application gets resumed:
///     void OnResume() {
///       controller_api->Resume();
///     }
///
/// To conserve battery, be sure to call Pause() and Resume() when your app
/// gets paused and resumed, respectively. This will allow the underlying
/// logic to unbind from the VR service and let the controller sleep when
/// no apps are using it.
///
/// THREADING: this class is thread-safe and reentrant after initialized
/// with Init().
class ControllerApi {
 public:
  /// Creates an (uninitialized) ControllerApi object. You must initialize
  /// it by calling Init() before interacting with it.
  ControllerApi() : context_(nullptr) {}

  // Deprecated factory-style create method.
  // TODO(btco): remove this once no one is using it.
  static std::unique_ptr<ControllerApi> Create() {
    return std::unique_ptr<ControllerApi>(new ControllerApi);
  }

  /// Initializes the ControllerApiOptions with the default options.
  static void InitDefaultOptions(ControllerApiOptions* options) {
    gvr_controller_init_default_options(options);
  }

  /// Initializes the controller API.
  ///
  /// This method must be called exactly once in the lifetime of this object.
  /// Other methods in this class may only be called after Init() returns true.
  /// Note: this does not cause the ControllerApi to be resumed. You must call
  /// Resume() explicitly in order to start using the controller.
  ///
  /// For more information see gvr_controller_create_and_init().
  ///
  /// @return True if initialization was successful, false if it failed.
  ///     Initialization may fail, for example, because invalid options were
  ///     supplied.
  bool Init(const ControllerApiOptions& options, gvr_context* context) {
    context_ = gvr_controller_create_and_init(options, context);
    return context_ != nullptr;
  }

#ifdef __ANDROID__
  /// Overload of Init() with explicit Android context and class loader
  /// (for Android only). For more information, see:
  /// gvr_controller_create_and_init_android().
  bool Init(JNIEnv *env, jobject android_context, jobject class_loader,
            const ControllerApiOptions& options, gvr_context* context) {
    context_ = gvr_controller_create_and_init_android(
        env, android_context, class_loader, options, context);
    return context_ != nullptr;
  }
#endif  // #ifdef __ANDROID__

  /// Convenience overload that calls Init without a gvr_context.
  // TODO(btco): remove once it is no longer being used.
  bool Init(const ControllerApiOptions& options) {
    return Init(options, nullptr);
  }

  /// Pauses the controller.
  /// For more information, see gvr_controller_pause().
  void Pause() {
    gvr_controller_pause(context_);
  }

  /// Resumes the controller.
  /// For more information, see gvr_controller_resume().
  void Resume() {
    gvr_controller_resume(context_);
  }

  /// Reads the current state of the controller.
  /// For more information, see gvr_controller_read_state().
  void ReadState(ControllerState* out_state) {
    gvr_controller_read_state(context_, out_state);
  }

  /// Destroys this ControllerApi instance.
  ~ControllerApi() {
    if (context_) gvr_controller_destroy(&context_);
  }

  /// Convenience functions to convert enums to strings.
  /// For more information, see the corresponding functions in the C API.
  static const char* ToString(ControllerApiStatus status) {
    return gvr_controller_api_status_to_string(status);
  }

  static const char* ToString(ControllerConnectionState state) {
    return gvr_controller_connection_state_to_string(state);
  }

  static const char* ToString(ControllerButton button) {
    return gvr_controller_button_to_string(button);
  }

 private:
  gvr_controller_context* context_;

  // Disallow copy and assign:
  ControllerApi(const ControllerApi&);
  void operator=(const ControllerApi&);
};

}  // namespace gvr
#endif  // #ifdef __cplusplus

#endif  // VR_GVR_CAPI_INCLUDE_GVR_CONTROLLER_H_
