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

#ifndef CONTROLLER_PAINT_APP_SRC_MAIN_JNI_UTILS_H_  // NOLINT
#define CONTROLLER_PAINT_APP_SRC_MAIN_JNI_UTILS_H_

#include <android/asset_manager.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <jni.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>

#include <array>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_controller.h"

#ifdef __ANDROID__
  #define LOG_TAG "ControllerDemoCPP"
  #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
  #define LOGW(...) \
      __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
  #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
  #define LOGD(...)
  #define LOGW(...)
  #define LOGE(...)
#endif  // #ifdef __ANDROID__

#define CHECK(condition) if (!(condition)) { \
        LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
        abort(); }
#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK((a) != (b))

// Assorted utilities and boilerplate code.
class Utils {
 public:
  // Obtains the ClassLoader associated to a given Android Activity.
  static jobject GetClassLoaderFromActivity(JNIEnv* env, jobject activity);

  // Multiplies matrices.
  static gvr::Mat4f MatrixMul(const gvr::Mat4f& m1, const gvr::Mat4f& m2);

  // Multiplies a matrix by a vector.
  static std::array<float, 3> MatrixVectorMul(const gvr::Mat4f& matrix,
                                              const std::array<float, 3>& vec);

  // Adds two vectors together.
  static std::array<float, 3> VecAdd(
      float scale_a, const std::array<float, 3>& a,
      float scale_b, const std::array<float, 3>& b);

  // Computes the norm of a vector.
  static float VecNorm(const std::array<float, 3>& vec);

  // Normalizes a vector (returns a vector with the same direction as
  // the input vector, but with unit length). Result is undefined if
  // the input vector has magnitude zero or almost zero.
  static std::array<float, 3> VecNormalize(const std::array<float, 3>& vec);

  // Computes the cross product between the two given vectors.
  static std::array<float, 3> VecCrossProd(
      const std::array<float, 3>& a, const std::array<float, 3>& b);

  // Sets up the viewport and scissor regions in preparation for rendering,
  // according to the given RenderTextureParams object.
  static void SetUpViewportAndScissor(const gvr::Sizei& framebuf_size,
                                      const gvr::BufferViewport& params);

  // Compiles a shader of the given type (GL_VERTEX_SHADER or
  // GL_FRAGMENT_SHADER) and returns its handle. Aborts on failure.
  static int BuildShader(int type, const char* source);

  // Links a vertex shader and a fragment shader together to produce a program.
  // Returns the handle of that program.
  static int BuildProgram(int vertex_shader, int frag_shader);

  // Calculates a perspective matrix from the given view parameters.
  static gvr::Mat4f PerspectiveMatrixFromView(const gvr::Rectf& fov,
                                              float near_clip, float far_clip);

  // Converts a row-major matrix to a column-major, GL-compatible matrix array.
  static std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix);

  // Loads a texture from the given asset file. Returns the handle of the
  // texture.
  static int LoadRawTextureFromAsset(
      AAssetManager* asset_mgr, const char* asset_path, int width, int height);

  // Converts a controller quaternion to a rotation matrix.
  static gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat);

  // Convertes a color from hexadecimal notation to a GL-friendly float array.
  static std::array<float, 4> ColorFromHex(int hex);
};

#endif  // CONTROLLER_PAINT_APP_SRC_MAIN_JNI_UTILS_H_  // NOLINT
