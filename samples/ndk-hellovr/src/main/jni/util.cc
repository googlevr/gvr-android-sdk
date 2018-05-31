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
#include <sstream>
#include <string>

namespace ndk_hello_vr {

namespace {

// A small utility class to run a function at the end of the current scope.
class RunAtEndOfScope {
 public:
  RunAtEndOfScope(std::function<void()> function) : function_(function) {}
  ~RunAtEndOfScope() { function_(); }

 private:
  std::function<void()> function_;
};

// TODO(b/71900928): see if there's a way to share these utility functions
// among all the samples.

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
  RunAtEndOfScope cleanup_j_path([&] {
    if (j_path) {
      env->DeleteLocalRef(j_path);
    }
  });

  jobject image_stream =
      env->CallObjectMethod(java_asset_mgr, open_method, j_path);
  jobject image_obj = env->CallStaticObjectMethod(
      bitmap_factory_class, decode_stream_method, image_stream);
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

}  // anonymous namespace

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

TexturedMesh::TexturedMesh()
    : vertices_(), uv_(), indices_(), position_attrib_(0), uv_attrib_(0) {}

bool TexturedMesh::Initialize(JNIEnv* env, AAssetManager* asset_mgr,
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
  HELLOVR_CHECK(texture_id_ != 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
}

}  // namespace ndk_hello_vr
