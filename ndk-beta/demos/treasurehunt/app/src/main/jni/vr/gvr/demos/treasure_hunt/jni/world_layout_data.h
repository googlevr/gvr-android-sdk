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

#ifndef TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
#define TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT

/*
 * Copyright 2014 Google Inc. All Rights Reserved.

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

/**
 * Contains vertex, normal and color data.
 */

#include <array>

class WorldLayoutData {
 public:
  std::array<float, 108> CUBE_COORDS;
  std::array<float, 144> CUBE_COLORS;
  std::array<float, 144> CUBE_FOUND_COLORS;
  std::array<float, 108> CUBE_NORMALS;
  std::array<float, 72> FLOOR_COORDS;
  std::array<float, 72> FLOOR_NORMALS;
  std::array<float, 96> FLOOR_COLORS;

  WorldLayoutData() {
    CUBE_COORDS = {{
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
      -1.0f, -1.0f, -1.0f,
      }};

    CUBE_COLORS = {{
      // front, green
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,

      // right, blue
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,

      // back, also green
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,
      0.0f, 0.5273f, 0.2656f, 1.0f,

      // left, also blue
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,

      // top, red
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,

      // bottom, also red
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      0.8359375f, 0.17578125f, 0.125f, 1.0f,
      }};

    CUBE_FOUND_COLORS = {{
      // front, yellow
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,

      // right, yellow
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,

      // back, yellow
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,

      // left, yellow
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,

      // top, yellow
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,

      // bottom, yellow
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      1.0f, 0.6523f, 0.0f, 1.0f,
      }};

    CUBE_NORMALS = {{
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
      0.0f, -1.0f, 0.0f
      }};

    // The grid lines on the floor are rendered procedurally and large polygons
    // cause floating point precision problems on some architectures. So we
    // split the floor into 4 quadrants.
    FLOOR_COORDS = {{
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
      }};

    FLOOR_NORMALS = {{
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      }};

    FLOOR_COLORS = {{
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      }};
  }
};

#endif  // TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
