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
#include "util.h"  // NOLINT

#include <string.h>  // Needed for strtok_r and strstr
#include <unistd.h>
#include <cmath>
#include <random>
#include <sstream>
#include <string>

namespace ndk_hello_vr_beta {

namespace {

// Loads a png file from assets folder and then assigns it to the OpenGL target.
// This method must be called from the renderer thread since it will result in
// OpenGL calls to assign the image to the texture target.
//
// @param env The JNIEnv to use.
// @param target, openGL texture target to load the image into.
// @param path, path to the file, relative to the assets folder.
// @return true if png is loaded correctly, otherwise false.
bool LoadPngFromAssetManager(JNIEnv* env, jobject java_asset_mgr, int target,
                             const std::string& path) {
  jclass bitmap_factory_class =
      env->FindClass("android/graphics/BitmapFactory");
  jclass asset_manager_class =
      env->FindClass("android/content/res/AssetManager");
  jclass gl_utils_class = env->FindClass("android/opengl/GLUtils");
  jmethodID decode_stream_method = env->GetStaticMethodID(
      bitmap_factory_class, "decodeStream",
      "(Ljava/io/InputStream;)Landroid/graphics/Bitmap;");
  jmethodID open_method = env->GetMethodID(
      asset_manager_class, "open", "(Ljava/lang/String;)Ljava/io/InputStream;");
  jmethodID tex_image_2d_method = env->GetStaticMethodID(
      gl_utils_class, "texImage2D", "(IILandroid/graphics/Bitmap;I)V");

  jstring j_path = env->NewStringUTF(path.c_str());
  jobject image_stream =
      env->CallObjectMethod(java_asset_mgr, open_method, j_path);
  jobject image_obj = env->CallStaticObjectMethod(
      bitmap_factory_class, decode_stream_method, image_stream);
  if (j_path) {
    env->DeleteLocalRef(j_path);
  }
  if (env->ExceptionOccurred() != nullptr) {
    LOGE("Java exception while loading image");
    env->ExceptionClear();
    image_obj = nullptr;
    return false;
  }

  env->CallStaticVoidMethod(gl_utils_class, tex_image_2d_method, target, 0,
                            image_obj, 0);
  return true;
}

// Loads obj file from assets folder from the app.
//
// This sample uses the .obj format since .obj is straightforward to parse and
// the sample is intended to be self-contained, but a real application
// should probably use a library to load a more modern format, such as FBX or
// glTF.
//
// @param mgr, AAssetManager pointer.
// @param file_name, name of the obj file.
// @param out_vertices, output vertices.
// @param out_normals, output normals.
// @param out_uv, output texture UV coordinates.
// @param out_indices, output triangle indices.
// @return true if obj is loaded correctly, otherwise false.
bool LoadObjFile(AAssetManager* mgr, const std::string& file_name,
                 std::vector<GLfloat>* out_vertices,
                 std::vector<GLfloat>* out_normals,
                 std::vector<GLfloat>* out_uv,
                 std::vector<GLushort>* out_indices) {
  std::vector<GLfloat> temp_positions;
  std::vector<GLfloat> temp_normals;
  std::vector<GLfloat> temp_uvs;
  std::vector<GLushort> vertex_indices;
  std::vector<GLushort> normal_indices;
  std::vector<GLushort> uv_indices;

  // If the file hasn't been uncompressed, load it to the internal storage.
  // Note that AAsset_openFileDescriptor doesn't support compressed
  // files (.obj).
  AAsset* asset =
      AAssetManager_open(mgr, file_name.c_str(), AASSET_MODE_STREAMING);
  if (asset == nullptr) {
    LOGE("Error opening asset %s", file_name.c_str());
    return false;
  }

  off_t file_size = AAsset_getLength(asset);
  std::string file_buffer;
  file_buffer.resize(file_size);
  int ret = AAsset_read(asset, &file_buffer.front(), file_size);
  AAsset_close(asset);

  if (ret < 0 || ret == EOF) {
    LOGE("Failed to open file: %s", file_name.c_str());
    return false;
  }

  std::stringstream file_string_stream(file_buffer);

  while (file_string_stream && !file_string_stream.eof()) {
    char line_header[128];
    file_string_stream.getline(line_header, 128);

    if (line_header[0] == '\0') {
      continue;
    } else if (line_header[0] == 'v' && line_header[1] == 'n') {
      // Parse vertex normal.
      GLfloat normal[3];
      int matches = sscanf(line_header, "vn %f %f %f\n", &normal[0], &normal[1],
                           &normal[2]);
      if (matches != 3) {
        LOGE("Format of 'vn float float float' required for each normal line");
        return false;
      }

      temp_normals.push_back(normal[0]);
      temp_normals.push_back(normal[1]);
      temp_normals.push_back(normal[2]);
    } else if (line_header[0] == 'v' && line_header[1] == 't') {
      // Parse texture uv.
      GLfloat uv[2];
      int matches = sscanf(line_header, "vt %f %f\n", &uv[0], &uv[1]);
      if (matches != 2) {
        LOGE("Format of 'vt float float' required for each texture uv line");
        return false;
      }

      temp_uvs.push_back(uv[0]);
      temp_uvs.push_back(uv[1]);
    } else if (line_header[0] == 'v') {
      // Parse vertex.
      GLfloat vertex[3];
      int matches = sscanf(line_header, "v %f %f %f\n", &vertex[0], &vertex[1],
                           &vertex[2]);
      if (matches != 3) {
        LOGE("Format of 'v float float float' required for each vertex line");
        return false;
      }

      temp_positions.push_back(vertex[0]);
      temp_positions.push_back(vertex[1]);
      temp_positions.push_back(vertex[2]);
    } else if (line_header[0] == 'f') {
      // Actual faces information starts from the second character.
      char* face_line = &line_header[1];

      unsigned int vertex_index[4];
      unsigned int normal_index[4];
      unsigned int texture_index[4];

      std::vector<char*> per_vertex_info_list;
      char* per_vertex_info_list_c_str;
      char* face_line_iter = face_line;
      while ((per_vertex_info_list_c_str =
                  strtok_r(face_line_iter, " ", &face_line_iter))) {
        // Divide each faces information into individual positions.
        per_vertex_info_list.push_back(per_vertex_info_list_c_str);
      }

      bool is_normal_available = false;
      bool is_uv_available = false;
      for (int i = 0; i < per_vertex_info_list.size(); ++i) {
        char* per_vertex_info;
        int per_vertex_info_count = 0;

        const bool is_vertex_normal_only_face =
            strstr(per_vertex_info_list[i], "//") != nullptr;

        char* per_vertex_info_iter = per_vertex_info_list[i];
        while ((per_vertex_info = strtok_r(per_vertex_info_iter, "/",
                                           &per_vertex_info_iter))) {
          // write only normal and vert values.
          switch (per_vertex_info_count) {
            case 0:
              // Write to vertex indices.
              vertex_index[i] = atoi(per_vertex_info);  // NOLINT
              break;
            case 1:
              // Write to texture indices.
              if (is_vertex_normal_only_face) {
                normal_index[i] = atoi(per_vertex_info);  // NOLINT
                is_normal_available = true;
              } else {
                texture_index[i] = atoi(per_vertex_info);  // NOLINT
                is_uv_available = true;
              }
              break;
            case 2:
              // Write to normal indices.
              if (!is_vertex_normal_only_face) {
                normal_index[i] = atoi(per_vertex_info);  // NOLINT
                is_normal_available = true;
                break;
              }
              [[clang::fallthrough]];
              // Fallthrough to error case because if there's no texture coords,
              // there should only be 2 indices per vertex (position and
              // normal).
            default:
              // Error formatting.
              LOGE(
                  "Format of 'f int/int/int int/int/int int/int/int "
                  "(int/int/int)' "
                  "or 'f int//int int//int int//int (int//int)' required for "
                  "each face");
              return false;
          }
          per_vertex_info_count++;
        }
      }

      const int vertices_count = per_vertex_info_list.size();
      for (int i = 2; i < vertices_count; ++i) {
        vertex_indices.push_back(vertex_index[0] - 1);
        vertex_indices.push_back(vertex_index[i - 1] - 1);
        vertex_indices.push_back(vertex_index[i] - 1);

        if (is_normal_available) {
          normal_indices.push_back(normal_index[0] - 1);
          normal_indices.push_back(normal_index[i - 1] - 1);
          normal_indices.push_back(normal_index[i] - 1);
        }

        if (is_uv_available) {
          uv_indices.push_back(texture_index[0] - 1);
          uv_indices.push_back(texture_index[i - 1] - 1);
          uv_indices.push_back(texture_index[i] - 1);
        }
      }
    }
  }

  const bool is_normal_available = !normal_indices.empty();
  const bool is_uv_available = !uv_indices.empty();

  if (is_normal_available && normal_indices.size() != vertex_indices.size()) {
    LOGE("Obj normal indices does not equal to vertex indices.");
    return false;
  }

  if (is_uv_available && uv_indices.size() != vertex_indices.size()) {
    LOGE("Obj UV indices does not equal to vertex indices.");
    return false;
  }

  for (unsigned int i = 0; i < vertex_indices.size(); i++) {
    unsigned int vertex_index = vertex_indices[i];
    out_vertices->push_back(temp_positions[vertex_index * 3]);
    out_vertices->push_back(temp_positions[vertex_index * 3 + 1]);
    out_vertices->push_back(temp_positions[vertex_index * 3 + 2]);
    out_indices->push_back(i);

    if (is_normal_available) {
      unsigned int normal_index = normal_indices[i];
      out_normals->push_back(temp_normals[normal_index * 3]);
      out_normals->push_back(temp_normals[normal_index * 3 + 1]);
      out_normals->push_back(temp_normals[normal_index * 3 + 2]);
    }

    if (is_uv_available) {
      unsigned int uv_index = uv_indices[i];
      out_uv->push_back(temp_uvs[uv_index * 2]);
      out_uv->push_back(temp_uvs[uv_index * 2 + 1]);
    }
  }

  return true;
}

float GetLength(const gvr::Vec3f& vector) {
  return std::sqrt(vector.x * vector.x + vector.y * vector.y +
                   vector.z * vector.z);
}

gvr::Vec3f GetNormalized(const gvr::Vec3f& vector) {
  float length = GetLength(vector);
  return {vector.x / length, vector.y / length, vector.z / length};
}

float DotProduct(const gvr::Vec3f& u, const gvr::Vec3f& v) {
  return u.x * v.x + u.y * v.y + u.z * v.z;
}

float GetProjectedVectorScale(const gvr::Vec3f& u, const gvr::Vec3f& v) {
  return DotProduct(u, v) / DotProduct(v, v);
}

}  // anonymous namespace

std::array<float, 32> MatrixPairToGLArray(const gvr::Mat4f matrix[]) {
  std::array<float, 32> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix[0].m[i][j];
      result[16 + j * 4 + i] = matrix[1].m[i][j];
    }
  }
  return result;
}

gvr::Vec3f MatrixVectorMul(const gvr::Mat4f& matrix, const gvr::Vec3f& vec) {
  const auto* m = matrix.m;
  return {
      m[0][0] * vec.x + m[0][1] * vec.y + m[0][2] * vec.z,
      m[1][0] * vec.x + m[1][1] * vec.y + m[1][2] * vec.z,
      m[2][0] * vec.x + m[2][1] * vec.y + m[2][2] * vec.z,
  };
}

gvr::Vec3f MatrixPointMul(const gvr::Mat4f& matrix, const gvr::Vec3f& point) {
  const auto* m = matrix.m;
  return {
      m[0][0] * point.x + m[0][1] * point.y + m[0][2] * point.z + m[0][3],
      m[1][0] * point.x + m[1][1] * point.y + m[1][2] * point.z + m[1][3],
      m[2][0] * point.x + m[2][1] * point.y + m[2][2] * point.z + m[2][3],
  };
}

gvr::Vec3f GetMatrixTranslation(const gvr::Mat4f& matrix) {
  return {matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]};
}

gvr::Mat4f GetTranslationMatrix(const gvr::Vec3f& translation) {
  return {{{1.0f, 0.0f, 0.0f, translation.x},
           {0.0f, 1.0f, 0.0f, translation.y},
           {0.0f, 0.0f, 1.0f, translation.z},
           {0.0f, 0.0f, 0.0f, 1.0f}}};
}

gvr::Mat4f MatrixMul(const gvr::Mat4f& matrix1, const gvr::Mat4f& matrix2) {
  gvr::Mat4f result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
      }
    }
  }
  return result;
}

gvr::Mat4f GetAxisAngleRotationMatrix(const gvr::Vec3f& axis, float radians) {
  gvr::Vec3f n = GetNormalized(axis);
  float c = std::cos(radians);
  float one_minus_c = 1.0 - c;
  float s = std::sin(radians);
  gvr::Mat4f result = {};
  result.m[0][0] = one_minus_c * n.x * n.x + c;
  result.m[0][1] = one_minus_c * n.y * n.x - s * n.z;
  result.m[0][2] = one_minus_c * n.z * n.x + s * n.y;
  result.m[1][0] = one_minus_c * n.x * n.y + s * n.z;
  result.m[1][1] = one_minus_c * n.y * n.y + c;
  result.m[1][2] = one_minus_c * n.z * n.y - s * n.x;
  result.m[2][0] = one_minus_c * n.x * n.z - s * n.y;
  result.m[2][1] = one_minus_c * n.y * n.z + s * n.x;
  result.m[2][2] = one_minus_c * n.z * n.z + c;
  result.m[3][3] = 1.0f;
  return result;
}

gvr::Mat4f GetOrthoInverse(const gvr::Mat4f& mat) {
  gvr::Mat4f result = {};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) result.m[i][j] = mat.m[j][i];
  }
  result.m[0][3] = -mat.m[0][3];
  result.m[1][3] = -mat.m[1][3];
  result.m[2][3] = -mat.m[2][3];
  result.m[3][3] = 1.0f;
  return result;
}

gvr::Vec3f PositionFromHeadSpace(const gvr::Mat4f& mat) {
  gvr::Mat4f inverse = GetOrthoInverse(mat);
  gvr::Vec3f mul = MatrixVectorMul(inverse, GetMatrixTranslation(mat));
  return {-mul.x, -mul.y, -mul.z};
}

gvr::Mat4f ProjectionMatrixFromView(const gvr::Rectf& fov, float z_near,
                                    float z_far) {
  gvr::Mat4f result = {};
  const float x_left = -std::tan(fov.left * M_PI / 180.0f) * z_near;
  const float x_right = std::tan(fov.right * M_PI / 180.0f) * z_near;
  const float y_bottom = -std::tan(fov.bottom * M_PI / 180.0f) * z_near;
  const float y_top = std::tan(fov.top * M_PI / 180.0f) * z_near;
  const float zero = 0.0f;

  assert(x_left < x_right && y_bottom < y_top && z_near < z_far &&
         z_near > zero && z_far > zero);
  const float X = (2 * z_near) / (x_right - x_left);
  const float Y = (2 * z_near) / (y_top - y_bottom);
  const float A = (x_right + x_left) / (x_right - x_left);
  const float B = (y_top + y_bottom) / (y_top - y_bottom);
  const float C = (z_near + z_far) / (z_near - z_far);
  const float D = (2 * z_near * z_far) / (z_near - z_far);

  result.m[0][0] = X;
  result.m[0][2] = A;
  result.m[1][1] = Y;
  result.m[1][2] = B;
  result.m[2][2] = C;
  result.m[2][3] = D;
  result.m[3][2] = -1;

  return result;
}

gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  const float x2 = quat.qx * quat.qx;
  const float y2 = quat.qy * quat.qy;
  const float z2 = quat.qz * quat.qz;
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

  return {{{m11, m12, m13, 0.0f},
           {m21, m22, m23, 0.0f},
           {m31, m32, m33, 0.0f},
           {0.0f, 0.0f, 0.0f, 1.0f}}};
}

float RandomUniformFloat(float min, float max) {
  static std::random_device random_device;  // NOLINT
  static std::mt19937 random_generator(random_device());
  static std::uniform_real_distribution<float> random_distribution(0.0f, 1.0f);
  return random_distribution(random_generator) * (max - min) + min;
}

void CheckGLError(const char* label) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    LOGW("GL error @ %s: %d", label, gl_error);
    // Crash immediately to make OpenGL errors obvious.
    abort();
  }
}

gvr::Sizei HalfPixelCount(const gvr::Sizei& in) {
  // Scale each dimension by sqrt(2)/2 ~= 7/10ths.
  gvr::Sizei out;
  out.width = (7 * in.width) / 10;
  out.height = (7 * in.height) / 10;
  return out;
}

bool DoesRayIntersectSphere(const gvr::Vec3f& ray_origin,
                            const gvr::Vec3f& ray_direction,
                            const gvr::Vec3f& sphere_center, float radius) {
  gvr::Vec3f ray_origin_to_sphere = sphere_center - ray_origin;
  float project = GetProjectedVectorScale(ray_origin_to_sphere, ray_direction);
  if (project < 0.0f) {
    return false;
  }
  gvr::Vec3f projected = ray_direction * project;
  return GetLength(ray_origin + projected - sphere_center) < radius;
}

TexturedMesh::TexturedMesh()
    : vertices_(), uv_(), indices_(), position_attrib_(0), uv_attrib_(0) {}

bool TexturedMesh::Initialize(AAssetManager* asset_mgr,
                              const std::string& obj_file_path,
                              GLuint position_attrib, GLuint uv_attrib) {
  position_attrib_ = position_attrib;
  uv_attrib_ = uv_attrib;
  // We don't use normals for anything so we discard them.
  std::vector<GLfloat> normals;
  if (!LoadObjFile(asset_mgr, obj_file_path, &vertices_, &normals, &uv_,
                   &indices_)) {
    return false;
  }
  return true;
}

void TexturedMesh::Draw() const {
  glEnableVertexAttribArray(position_attrib_);
  glVertexAttribPointer(position_attrib_, 3, GL_FLOAT, false, 0,
                        vertices_.data());
  glEnableVertexAttribArray(uv_attrib_);
  glVertexAttribPointer(uv_attrib_, 2, GL_FLOAT, false, 0, uv_.data());

  glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_SHORT,
                 indices_.data());
}

Texture::Texture() : texture_id_(0) {}

Texture::~Texture() {
  if (texture_id_ != 0) {
    glDeleteTextures(1, &texture_id_);
  }
}

bool Texture::Initialize(JNIEnv* env, jobject java_asset_mgr,
                         const std::string& texture_path) {
  glGenTextures(1, &texture_id_);
  Bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  if (!LoadPngFromAssetManager(env, java_asset_mgr, GL_TEXTURE_2D,
                               texture_path)) {
    LOGE("Couldn't load texture.");
    return false;
  }
  glGenerateMipmap(GL_TEXTURE_2D);
  return true;
}

void Texture::Bind() const {
  HELLOVRBETA_CHECK(texture_id_ != 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
}

}  // namespace ndk_hello_vr_beta
