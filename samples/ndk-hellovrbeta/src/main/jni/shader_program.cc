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

#include "shader_program.h"  // NOLINT
#include "util.h"            // NOLINT

namespace ndk_hello_vr_beta {

namespace {

// Multiview vertex shaders use transforms defined by arrays of mat4 uniforms,
// using gl_ViewID_OVR to determine the array index.

// Simple shaders to render .obj files without any lighting.
constexpr const char* kTexturedMeshVertexShader =
    // The following shader is for multiview rendering.
    R"glsl(#version 300 es
    #extension GL_OVR_multiview2 : enable

    layout(num_views=2) in;

    uniform mat4 u_MVP[2];
    in vec4 a_Position;
    in vec2 a_UV;
    out vec2 v_UV;

    void main() {
      mat4 mvp = u_MVP[gl_ViewID_OVR];
      v_UV = a_UV;
      gl_Position = mvp * a_Position;
    })glsl";

constexpr const char* kTexturedMeshFragmentShader =
    R"glsl(#version 300 es

    precision mediump float;
    in vec2 v_UV;
    out vec4 FragColor;
    uniform sampler2D u_Texture;

    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      FragColor = texture(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
    })glsl";

constexpr const char* kTexturedMeshAlphaFragmentShader =
    R"glsl(#version 300 es

    precision mediump float;
    in vec2 v_UV;
    out vec4 FragColor;
    uniform float a_Alpha;
    uniform sampler2D u_Texture;

    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      FragColor = texture(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
      FragColor.a = FragColor.a * a_Alpha;
    })glsl";

/**
 * Converts a string into an OpenGL ES shader.
 *
 * @param type The type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
 * @param shader_source The source code of the shader.
 * @return The shader object handler, or 0 if there's an error.
 */
GLuint LoadGLShader(GLenum type, const char* shader_source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shader_source, nullptr);
  glCompileShader(shader);

  // Get the compilation status.
  GLint compile_status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

  // If the compilation failed, delete the shader and show an error.
  if (compile_status == 0) {
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len == 0) {
      return 0;
    }

    std::vector<char> info_string(info_len);
    glGetShaderInfoLog(shader, info_string.size(), nullptr, info_string.data());
    LOGE("Could not compile shader of type %d: %s", type, info_string.data());
    glDeleteShader(shader);
    return 0;
  } else {
    return shader;
  }
}

}  // anonymous namespace

void ShaderProgram::Link(const char* vertex, const char* fragment) {
  program_ = glCreateProgram();

  const int vertex_shader = LoadGLShader(GL_VERTEX_SHADER, vertex);
  const int fragment_shader = LoadGLShader(GL_FRAGMENT_SHADER, fragment);

  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);
  glUseProgram(program_);
}

void TexturedShaderProgram::Link() {
  ShaderProgram::Link(kTexturedMeshVertexShader, kTexturedMeshFragmentShader);
  mode_view_projection_ = glGetUniformLocation(program_, "u_MVP");
}

void TexturedShaderProgram::SetModelViewProjection(
    const gvr::Mat4f& model, const gvr::Mat4f view_projection[2]) const {
  gvr::Mat4f model_view_projection[2] = {MatrixMul(view_projection[0], model),
                                         MatrixMul(view_projection[1], model)};
  glUniformMatrix4fv(mode_view_projection_, 2, GL_FALSE,
                     MatrixPairToGLArray(model_view_projection).data());
}

GLuint TexturedShaderProgram::GetPositionAttribute() const {
  return glGetAttribLocation(program_, "a_Position");
}

GLuint TexturedShaderProgram::GetUVAttribute() const {
  return glGetAttribLocation(program_, "a_UV");
}

void ControllerShaderProgram::Link() {
  ShaderProgram::Link(kTexturedMeshVertexShader,
                      kTexturedMeshAlphaFragmentShader);
  mode_view_projection_ = glGetUniformLocation(program_, "u_MVP");
  alpha_ = glGetUniformLocation(program_, "a_Alpha");
  SetAlpha(1.0f);
}

void ControllerShaderProgram::SetAlpha(float alpha) const {
  glUniform1f(alpha_, alpha);
}

}  // namespace ndk_hello_vr_beta
