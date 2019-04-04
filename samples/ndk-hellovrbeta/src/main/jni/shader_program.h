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

#ifndef HELLOVRBETA_APP_SRC_MAIN_JNI_SHADER_PROGRAM_H_  // NOLINT
#define HELLOVRBETA_APP_SRC_MAIN_JNI_SHADER_PROGRAM_H_  // NOLINT

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "vr/gvr/capi/include/gvr.h"

namespace ndk_hello_vr_beta {

/**
 * Helper to manage shader programs.
 */
class ShaderProgram {
 public:
  void Use() const { glUseProgram(program_); }

 protected:
  void Link(const char* vertex, const char* fragment);

  GLuint program_ = 0;
};

class TexturedShaderProgram : public ShaderProgram {
 public:
  void Link();

  GLuint GetPositionAttribute() const;
  GLuint GetUVAttribute() const;

  void SetModelViewProjection(const gvr::Mat4f& model,
                              const gvr::Mat4f view_projection[2]) const;

 protected:
  GLuint mode_view_projection_;
};

class TexturedAlphaShaderProgram : public TexturedShaderProgram {
 public:
  void Link();

  void SetAlpha(float alpha) const;

 protected:
  GLuint alpha_;
};

class ControllerShaderProgram : public TexturedAlphaShaderProgram {
 public:
  void Link();

  void SetBatteryUVRect(const gvr::Rectf& uv) const;
  void SetBatteryOffset(const gvr::Vec2f& offset) const;

 protected:
  GLuint battery_uv_rect_;
  GLuint battery_offset_;
};

}  // namespace ndk_hello_vr_beta

#endif  // HELLOVRBETA_APP_SRC_MAIN_JNI_SHADER_PROGRAM_H_  // NOLINT
