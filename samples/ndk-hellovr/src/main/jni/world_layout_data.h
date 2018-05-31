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

#ifndef HELLOVR_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
#define HELLOVR_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT

#include <array>

// TODO(b/72458816): remove this file (it'll no longer be needed once all
// geometry is loaded from OBJ files).
// Contains vertex, normal and other data.
class WorldLayoutData {
 public:
  const std::array<float, 18> reticle_coords;

  WorldLayoutData() :
    reticle_coords({{
        -1.f, 1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        1.f, 1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        1.f, -1.f, 0.0f,
        1.f, 1.f, 0.0f,
      }}) {}
};
#endif  // HELLOVR_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
