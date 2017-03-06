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

#ifndef VR_GVR_CAPI_INCLUDE_GVR_AUDIO_SURROUND_H_
#define VR_GVR_CAPI_INCLUDE_GVR_AUDIO_SURROUND_H_

#include <stdint.h>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_types.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/// @defgroup AudioSurround Surround Renderer API
/// @brief This is the GVR Binaural Surround Renderer C API which reads in
/// PCM buffers of surround sound as well as ambisonic soundfield content
/// to render binaural stereo. In contrast to GVR's Spatial Audio API, the
/// Surround Renderer API is designed to be integrated into media players to
/// enable head-tracked binaural audio in VR movie and 360 video experiences. It
/// accepts input and output buffers of arbitrary frame size. However note that
/// the binaural output is only generated when the number of input frames exceed
/// the process_num_frames size specified during construction.

/// @{

typedef struct gvr_audio_surround_context_ gvr_audio_surround_context;

/// Creates and initializes a gvr_audio_surround_context. Note that
/// the returned instance must be deleted with
/// gvr_audio_surround_destroy.
///
/// @param surround_format Input surround format.
/// @param num_input_channels Number of input channels. This must match with
///     the selected surround rendering mode.
/// @param frames_per_processing Number of frames required to trigger internal
///     processing. Best performance is achieved for power of two sizes.
/// @param sample_rate_hz Sample rate of audio buffers.
gvr_audio_surround_context* gvr_audio_surround_create(
    gvr_audio_surround_format_type surround_format, int32_t num_input_channels,
    int32_t frames_per_processing, int sample_rate_hz);

/// Destroys a gvr_audio_surround_context that was previously created
/// with gvr_audio_surround_create.
///
/// @param api Pointer to a pointer to a gvr_audio_surround_context.
///     The pointer will be set to NULL after destruction.
void gvr_audio_surround_destroy(gvr_audio_surround_context** api);

/// Returns the number of samples the input buffer is currently able to consume.
///
/// @param api Pointer to a gvr_audio_surround_context.
/// @return Number of available samples in input buffer.
int64_t gvr_audio_surround_get_available_input_size_samples(
    gvr_audio_surround_context* api);

/// Adds interleaved audio data to the renderer. If enough data has been
/// provided for an output buffer to be generated then it will be immediately
/// available via gvr_audio_surround_get_interleaved_output(). The
/// input data is copied into an internal buffer which allows the caller to
/// re-use the input buffer immediately. The available space in the internal
/// buffer can be obtained via
/// gvr_audio_surround_get_available_output_size_samples().
///
/// @param api Pointer to a gvr_audio_surround_context.
/// @param input_buffer_ptr Pointer to interleaved input data.
/// @param num_samples Size of interleaved input data in samples.
/// @return The number of consumed samples.
int64_t gvr_audio_surround_add_interleaved_input(
    gvr_audio_surround_context* api, const int16_t* input_buffer_ptr,
    int64_t num_samples);

/// Returns the number of samples available in the output buffer.
///
/// @param api Pointer to a gvr_audio_surround_context.
/// @return Number of available samples in output buffer.
int64_t gvr_audio_surround_get_available_output_size_samples(
    gvr_audio_surround_context* api);

/// Gets a processed output buffer in interleaved format.
///
/// @param api Pointer to a gvr_audio_surround_context.
/// @param output_buffer_ptr Pointer to allocated interleaved output buffer.
/// @param num_samples Size of output buffer in samples.
/// @return The number of consumed samples.
int64_t gvr_audio_surround_get_interleaved_output(
    gvr_audio_surround_context* api, int16_t* output_buffer_ptr,
    int64_t num_samples);

/// Removes all buffered input and processed output buffers from the buffer
/// queues.
///
/// @param api Pointer to a gvr_audio_surround_context.
void gvr_audio_surround_clear(gvr_audio_surround_context* api);

/// Triggers the processing of data that has been input but not yet processed.
/// Note after calling this method, all processed output must be consumed via
/// gvr_audio_surround_get_interleaved_output() before adding new input
/// buffers.
///
/// @param api Pointer to a gvr_audio_surround_context.
/// @return Whether any data was processed.
bool gvr_audio_surround_trigger_processing(gvr_audio_surround_context* api);

/// Updates the head rotation.
///
/// @param api Pointer to a gvr_audio_surround_context.
/// @param w W component of quaternion.
/// @param x X component of quaternion.
/// @param y Y component of quaternion.
/// @param z Z component of quaternion.
void gvr_audio_surround_set_head_rotation(gvr_audio_surround_context* api,
                                          float w, float x, float y, float z);

/// @}

#ifdef __cplusplus
}  // extern "C"
#endif

// Convenience C++ wrapper.
#if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)

#include <memory>
#include <string>

namespace gvr {
/// This is a convenience C++ wrapper for the Audio Surround C API.
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
///
/// THREADING: this class is thread-safe and reentrant after initialized
/// with Init().
class AudioSurroundApi {
 public:
  /// Creates an (uninitialized) ControllerApi object. You must initialize
  /// it by calling Init() before interacting with it.
  AudioSurroundApi() : context_(nullptr) {}

  ~AudioSurroundApi() {
    if (context_) {
      gvr_audio_surround_destroy(&context_);
    }
  }

  /// Creates and initializes a gvr_audio_context.
  /// For more information, see gvr_audio_surround_create().
  bool Init(AudioSurroundFormat surround_format, int32_t num_input_channels,
            int32_t frames_per_buffer, int sample_rate_hz) {
    context_ = gvr_audio_surround_create(surround_format, num_input_channels,
                                         frames_per_buffer, sample_rate_hz);
    return context_ != nullptr;
  }

  /// Returns the number of samples the input buffer is currently able to
  /// consume.
  /// For more information, see
  /// gvr_audio_surround_get_available_input_size_samples().
  int64_t GetAvailableInputSizeSamples() const {
    return gvr_audio_surround_get_available_input_size_samples(context_);
  }

  /// Adds interleaved audio data to the renderer.
  /// For more information, see
  /// gvr_audio_surround_add_interleaved_input().
  int64_t AddInterleavedInput(const int16_t* input_buffer_ptr,
                              int64_t num_samples) {
    return gvr_audio_surround_add_interleaved_input(context_, input_buffer_ptr,
                                                    num_samples);
  }

  /// Returns the number of samples available in the output buffer.
  /// For more information, see
  /// gvr_audio_surround_get_available_output_size_samples().
  int64_t GetAvailableOutputSizeSamples() const {
    return gvr_audio_surround_get_available_output_size_samples(context_);
  }

  /// Gets a processed output buffer in interleaved format.
  /// For more information, see
  /// gvr_audio_surround_get_interleaved_output().
  int64_t GetInterleavedOutput(int16_t* output_buffer_ptr,
                               int64_t num_samples) {
    return gvr_audio_surround_get_interleaved_output(
        context_, output_buffer_ptr, num_samples);
  }

  /// Removes all buffered input and processed output buffers from the buffer
  /// queues.
  /// For more information, see gvr_audio_surround_clear().
  void Clear() { gvr_audio_surround_clear(context_); }

  /// Triggers the processing of data that has been input but not yet processed.
  /// For more information, see
  /// gvr_audio_surround_trigger_processing().
  bool TriggerProcessing() {
    return gvr_audio_surround_trigger_processing(context_);
  }

  /// Updates the head rotation.
  /// For more information, see gvr_audio_surround_set_head_rotation().
  void SetHeadRotation(float w, float x, float y, float z) {
    gvr_audio_surround_set_head_rotation(context_, w, x, y, z);
  }

  /// @name Wrapper manipulation
  /// @{
  /// Creates a C++ wrapper for a C object and takes ownership.
  explicit AudioSurroundApi(gvr_audio_surround_context* context)
      : context_(context) {}

  /// Returns the wrapped C object. Does not affect ownership.
  gvr_audio_surround_context* cobj() { return context_; }
  const gvr_audio_surround_context* cobj() const { return context_; }

  /// Returns the wrapped C object and transfers its ownership to the caller.
  /// The wrapper becomes invalid and should not be used.
  gvr_audio_surround_context* Release() {
    auto result = context_;
    context_ = nullptr;
    return result;
  }
  /// @}

 private:
  gvr_audio_surround_context* context_;

  // Disallow copy and assign:
  AudioSurroundApi(const AudioSurroundApi&);
  void operator=(const AudioSurroundApi&);
};

}  // namespace gvr
#endif  // #if defined(__cplusplus) && !defined(GVR_NO_CPP_WRAPPER)

#endif  // VR_GVR_CAPI_INCLUDE_GVR_AUDIO_SURROUND_H_
