/*
 * Copyright 2017 Google Inc. All Rights Reserved.
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

#ifndef HELLOVRBETA_APP_SRC_MAIN_JNI_UTIL_H_  // NOLINT
#define HELLOVRBETA_APP_SRC_MAIN_JNI_UTIL_H_  // NOLINT

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <errno.h>
#include <jni.h>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "vr/gvr/capi/include/gvr.h"

#define LOG_TAG "HelloVrBetaApp"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define HELLOVRBETA_CHECK(condition)                                       \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }

namespace ndk_hello_vr_beta {

inline gvr::Vec3f operator+(const gvr::Vec3f& l, const gvr::Vec3f& r) {
  return {l.x + r.x, l.y + r.y, l.z + r.z};
}

inline gvr::Vec3f operator-(const gvr::Vec3f& l, const gvr::Vec3f& r) {
  return {l.x - r.x, l.y - r.y, l.z - r.z};
}

inline gvr::Vec3f operator*(const gvr::Vec3f& l, float scale) {
  return {l.x * scale, l.y * scale, l.z * scale};
}

template <typename T>
T Lerp(const T& a, const T& b, float t) {
  return a + (b - a) * t;
}

// Flattens a pair of mat4's into an array of 32 floats, useful when feeding
// uniform values to OpenGL for multiview.
std::array<float, 32> MatrixPairToGLArray(const gvr::Mat4f matrix[]);

// Multiplies a vector by a matrix.
gvr::Vec3f MatrixVectorMul(const gvr::Mat4f& matrix, const gvr::Vec3f& vec);
// Multiplies a point by a matrix.
gvr::Vec3f MatrixPointMul(const gvr::Mat4f& matrix, const gvr::Vec3f& point);

// Gets the translation components from a matrix.
gvr::Vec3f GetMatrixTranslation(const gvr::Mat4f& matrix);

// Gets a matrix without rotation for a given translation.
gvr::Mat4f GetTranslationMatrix(const gvr::Vec3f& translation);

// Multiplies two matrices.
gvr::Mat4f MatrixMul(const gvr::Mat4f& matrix1, const gvr::Mat4f& matrix2);

gvr::Mat4f GetAxisAngleRotationMatrix(const gvr::Vec3f& axis, float radians);

// Position from head space.
gvr::Vec3f PositionFromHeadSpace(const gvr::Mat4f& mat);

// Get the inverse for matrices with a orthogonal transformation.
gvr::Mat4f GetOrthoInverse(const gvr::Mat4f& mat);

// Given a field of view in degrees, computes the corresponding projection
// matrix.
gvr::Mat4f ProjectionMatrixFromView(const gvr::Rectf& fov, float z_near,
                                    float z_far);

// Converts the quaternion describing the controller's orientation to a
// rotation matrix.
gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat);

// Generates a random floating point number between |min| and |max|.
float RandomUniformFloat(float min, float max);

// Checks for OpenGL errors, and crashes if one has occurred.  Note that this
// can be an expensive call, so real applications should call this rarely.
void CheckGLError(const char* label);

// Computes a texture size that has approximately half as many pixels. This is
// equivalent to scaling each dimension by approximately sqrt(2)/2.
gvr::Sizei HalfPixelCount(const gvr::Sizei& in);

bool DoesRayIntersectSphere(const gvr::Vec3f& ray_origin,
                            const gvr::Vec3f& ray_direction,
                            const gvr::Vec3f& sphere_center, float radius);

class TexturedMesh {
 public:
  TexturedMesh();

  // Initializes the mesh from a .obj file.
  //
  // @return True if initialization was successful.
  bool Initialize(AAssetManager* asset_mgr,
                  const std::string& obj_file_path, GLuint position_attrib,
                  GLuint uv_attrib);

  // Draws the mesh. The u_MVP uniform should be set before calling this using
  // glUniformMatrix4fv(), and a texture should be bound to GL_TEXTURE0.
  void Draw() const;

 private:
  std::vector<GLfloat> vertices_;
  std::vector<GLfloat> uv_;
  std::vector<GLushort> indices_;
  GLuint position_attrib_;
  GLuint uv_attrib_;
};

class Texture {
 public:
  Texture();

  ~Texture();

  // Initializes the texture.
  //
  // After this is called the texture will be bound, replacing any previously
  // bound texture.
  //
  // @return True if initialization was successful.
  bool Initialize(JNIEnv* env, jobject java_asset_mgr,
                  const std::string& texture_path);

  // Binds the texture, replacing any previously bound texture.
  void Bind() const;

 private:
  GLuint texture_id_;
};

}  // namespace ndk_hello_vr_beta

#endif  // HELLOVRBETA_APP_SRC_MAIN_JNI_UTIL_H_  // NOLINT
