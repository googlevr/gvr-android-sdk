/* Copyright 2017 Google Inc. All rights reserved.
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

#ifndef TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
#define TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT

#include <array>

// Contains vertex, normal and other data.
class WorldLayoutData {
 public:
  const std::array<float, 108> cube_coords;
  const std::array<float, 108> cube_colors;
  const std::array<float, 3> cube_found_color;
  const std::array<float, 108> cube_normals;
  const std::array<float, 72> floor_coords;
  const std::array<float, 18> reticle_coords;

  WorldLayoutData() :
    cube_coords({{
        // Front face
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,

        // Right face
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,

        // Back face
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        // Left face
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,

        // Top face
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f,  1.0f,
         1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f,  1.0f,
         1.0f, 1.0f,  1.0f,
         1.0f, 1.0f, -1.0f,

        // Bottom face
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f}}),
    cube_colors({{
        // front, green
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,

        // right, blue
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,

        // back, also green
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,
        0.0f, 0.5273f, 0.2656f,

        // left, also blue
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,
        0.0f, 0.3398f, 0.9023f,

        // top, red
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,

        // bottom, also red
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f,
        0.8359375f, 0.17578125f, 0.125f}}),
    cube_found_color({{ 1.0f, 0.6523f, 0.0f }}),  // yellow
    cube_normals({{
        // Front face
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        // Right face
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        // Back face
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,

        // Left face
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        // Top face
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        // Bottom face
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f}}),
    // The grid lines on the floor are rendered procedurally and large polygons
    // cause floating point precision problems on some architectures. So we
    // split the floor into 4 quadrants.
    floor_coords({{
        // +X, +Z quadrant
        200.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 200.0f,
        200.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 200.0f,
        200.0f, 0.0f, 200.0f,

        // -X, +Z quadrant
        0.0f, 0.0f, 0.0f,
        -200.0f, 0.0f, 0.0f,
        -200.0f, 0.0f, 200.0f,
        0.0f, 0.0f, 0.0f,
        -200.0f, 0.0f, 200.0f,
        0.0f, 0.0f, 200.0f,

        // +X, -Z quadrant
        200.0f, 0.0f, -200.0f,
        0.0f, 0.0f, -200.0f,
        0.0f, 0.0f, 0.0f,
        200.0f, 0.0f, -200.0f,
        0.0f, 0.0f, 0.0f,
        200.0f, 0.0f, 0.0f,

        // -X, -Z quadrant
        0.0f, 0.0f, -200.0f,
        -200.0f, 0.0f, -200.0f,
        -200.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -200.0f,
        -200.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
      }}),
    reticle_coords({{
        -1.f, 1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        1.f, 1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        1.f, -1.f, 0.0f,
        1.f, 1.f, 0.0f,
      }}) {}
};
#endif  // TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
