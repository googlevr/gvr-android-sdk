/*
 * Copyright 2016 Google Inc. All rights reserved.
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

#include "vr/gvr/demos/controller_paint/jni/utils.h"

void Utils::SetUpViewportAndScissor(const gvr::Sizei& framebuf_size,
                                    const gvr::BufferViewport& params) {
  const gvr::Rectf& rect = params.GetSourceUv();
  int left = static_cast<int>(rect.left * framebuf_size.width);
  int bottom = static_cast<int>(rect.bottom * framebuf_size.width);
  int width = static_cast<int>((rect.right - rect.left) * framebuf_size.width);
  int height =
      static_cast<int>((rect.top - rect.bottom) * framebuf_size.height);
  glViewport(left, bottom, width, height);
  glEnable(GL_SCISSOR_TEST);
  glScissor(left, bottom, width, height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  CHECK(glGetError() == GL_NO_ERROR);
}

gvr::Mat4f Utils::MatrixMul(const gvr::Mat4f& m1, const gvr::Mat4f& m2) {
  gvr::Mat4f result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += m1.m[i][k] * m2.m[k][j];
      }
    }
  }
  return result;
}

std::array<float, 3> Utils::MatrixVectorMul(const gvr::Mat4f& matrix,
                                            const std::array<float, 3>& vec) {
  // Use homogeneous coordinates for the multiplication.
  std::array<float, 4> vec_h = {{ vec[0], vec[1], vec[2], 1.0f }};
  std::array<float, 4> result;
  for (int i = 0; i < 4; ++i) {
    result[i] = 0;
    for (int k = 0; k < 4; ++k) {
      result[i] += matrix.m[i][k] * vec_h[k];
    }
  }
  // Convert back from homogeneous coordinates.
  float rw = 1.0f / result[3];
  return {{ rw * result[0], rw * result[1], rw * result[2] }};
}


std::array<float, 3> Utils::VecAdd(
    float scale_a, const std::array<float, 3>& a,
    float scale_b, const std::array<float, 3>& b) {
  std::array<float, 3> result;
  for (int i = 0; i < 3; ++i) {
    result[i] = scale_a * a[i] + scale_b * b[i];
  }
  return result;
}

float Utils::VecNorm(const std::array<float, 3>& vec) {
  float sum = 0.0f;
  for (int i = 0; i < 3; ++i) {
    sum += vec[i] * vec[i];
  }
  return static_cast<float>(sqrt(sum));
}

std::array<float, 3> Utils::VecNormalize(const std::array<float, 3>& vec) {
  std::array<float, 3> result;
  float scale = 1.0f / VecNorm(vec);
  for (int i = 0; i < 3; ++i) {
    result[i] = vec[i] * scale;
  }
  return result;
}

std::array<float, 3> Utils::VecCrossProd(
    const std::array<float, 3>& a, const std::array<float, 3>& b) {
  std::array<float, 3> result;
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
  return result;
}

jobject Utils::GetClassLoaderFromActivity(JNIEnv* env, jobject activity) {
  jclass activity_class = env->GetObjectClass(activity);
  CHECK(activity_class);
  jmethodID get_classloader_mid = env->GetMethodID(
      activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
  CHECK(get_classloader_mid);
  jobject class_loader = env->CallObjectMethod(activity, get_classloader_mid);
  CHECK(class_loader);
  env->DeleteLocalRef(activity_class);
  return class_loader;
}

int Utils::BuildShader(int type, const char* source) {
  int shader = glCreateShader(type);
  CHECK(shader);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  int status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  CHECK_NE(0, status);
  return shader;
}

int Utils::BuildProgram(int vertex_shader, int frag_shader) {
  int program = glCreateProgram();
  CHECK(program);
  glAttachShader(program, vertex_shader);
  glAttachShader(program, frag_shader);
  glLinkProgram(program);
  int status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  CHECK_NE(0, status);
  return program;
}

gvr::Mat4f Utils::PerspectiveMatrixFromView(const gvr::Rectf& fov,
                                            float near_clip, float far_clip) {
  gvr::Mat4f result;
  const float x_left = -tan(fov.left * M_PI / 180.0f) * near_clip;
  const float x_right = tan(fov.right * M_PI / 180.0f) * near_clip;
  const float y_bottom = -tan(fov.bottom * M_PI / 180.0f) * near_clip;
  const float y_top = tan(fov.top * M_PI / 180.0f) * near_clip;
  const float zero = 0.0f;

  CHECK(x_left < x_right && y_bottom < y_top && near_clip < far_clip &&
         near_clip > zero && far_clip > zero);
  const float X = (2 * near_clip) / (x_right - x_left);
  const float Y = (2 * near_clip) / (y_top - y_bottom);
  const float A = (x_right + x_left) / (x_right - x_left);
  const float B = (y_top + y_bottom) / (y_top - y_bottom);
  const float C = (near_clip + far_clip) / (near_clip - far_clip);
  const float D = (2 * near_clip * far_clip) / (near_clip - far_clip);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
    }
  }
  result.m[0][0] = X;
  result.m[0][2] = A;
  result.m[1][1] = Y;
  result.m[1][2] = B;
  result.m[2][2] = C;
  result.m[2][3] = D;
  result.m[3][2] = -1;

  return result;
}

std::array<float, 16> Utils::MatrixToGLArray(const gvr::Mat4f& matrix) {
  // Note that this performs a *tranpose* to a column-major matrix array, as
  // expected by GL.
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

int Utils::LoadRawTextureFromAsset(
    AAssetManager* asset_mgr, const char* asset_path, int width, int height) {
  const int bytes_per_pixel = 3;  // RGB
  AAsset* asset = AAssetManager_open(asset_mgr, asset_path, AASSET_MODE_BUFFER);
  CHECK(asset);

  int length = static_cast<int>(AAsset_getLength(asset));
  CHECK(length == width * height * bytes_per_pixel);

  const uint8_t* source_buf = reinterpret_cast<const uint8_t*>(
      AAsset_getBuffer(asset));
  CHECK(source_buf);

  GLuint tex_id;
  glGenTextures(1, &tex_id);
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, source_buf);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  CHECK(glGetError() == GL_NO_ERROR);

  AAsset_close(asset);
  return tex_id;
}

gvr::Mat4f Utils::ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  gvr::Mat4f result;
  const float x = quat.qx;
  const float x2 = quat.qx * quat.qx;
  const float y = quat.qy;
  const float y2 = quat.qy * quat.qy;
  const float z = quat.qz;
  const float z2 = quat.qz * quat.qz;
  const float w = quat.qw;
  const float xy = quat.qx * quat.qy;
  const float xz = quat.qx * quat.qz;
  const float xw = quat.qx * quat.qw;
  const float yz = quat.qy * quat.qz;
  const float yw = quat.qy * quat.qw;
  const float zw = quat.qz * quat.qw;

  const float m11 = 1.0f - 2.0f * y2 - 2.0f * z2;
  const float m12 = 2.0f * (xy - zw);
  const float m13 = 2.0f * (xz + yw);
  const float m21 = 2.0f * (xy + zw);
  const float m22 = 1.0f - 2.0f * x2 - 2.0f * z2;
  const float m23 = 2.0f * (yz - xw);
  const float m31 = 2.0f * (xz - yw);
  const float m32 = 2.0f * (yz + xw);
  const float m33 = 1.0f - 2.0f * x2 - 2.0f * y2;

  return {
    m11,  m12,  m13,  0.0f,
    m21,  m22,  m23,  0.0f,
    m31,  m32,  m33,  0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
}

std::array<float, 4> Utils::ColorFromHex(int hex) {
  int a = (hex & 0xff000000) >> 24;
  int r = (hex & 0xff0000) >> 16;
  int g = (hex & 0xff00) >> 8;
  int b = (hex & 0xff);
  return { r / 256.0f, g / 256.0f, b / 256.0f, a / 256.0f };
}

