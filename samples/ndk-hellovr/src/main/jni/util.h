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

#ifndef HELLOVR_APP_SRC_MAIN_JNI_UTIL_H_  // NOLINT
#define HELLOVR_APP_SRC_MAIN_JNI_UTIL_H_  // NOLINT

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

#define LOG_TAG "HelloVrApp"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define HELLOVR_CHECK(condition)                                           \
  if (!(condition)) {                                                      \
    LOGE("*** CHECK FAILED at %s:%d: %s", __FILE__, __LINE__, #condition); \
    abort();                                                               \
  }

namespace ndk_hello_vr {
/**
 * Converts a string into an OpenGL ES shader.
 *
 * @param type The type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
 * @param shader_source The source code of the shader.
 * @return The shader object handler, or 0 if there's an error.
 */
GLuint LoadGLShader(GLenum type, const char* shader_source);

class TexturedMesh {
 public:
  TexturedMesh();

  // Initializes the mesh from a .obj file.
  //
  // @return True if initialization was successful.
  bool Initialize(JNIEnv* env, AAssetManager* asset_mgr,
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

}  // namespace ndk_hello_vr

#endif  // HELLOVR_APP_SRC_MAIN_JNI_UTIL_H_  // NOLINT
