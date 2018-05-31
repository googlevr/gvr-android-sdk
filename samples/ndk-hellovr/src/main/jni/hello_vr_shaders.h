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

#ifndef HELLOVR_APP_SRC_MAIN_JNI_HELLO_VR_SHADERS_H_  // NOLINT
#define HELLOVR_APP_SRC_MAIN_JNI_HELLO_VR_SHADERS_H_  // NOLINT

// Each shader has two variants: a single-eye ES 2.0 variant, and a multiview
// ES 3.0 variant.  The multiview vertex shaders use transforms defined by
// arrays of mat4 uniforms, using gl_ViewID_OVR to determine the array index.
static constexpr const char* kReticleVertexShaders[] = {R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    varying vec2 v_Coords;

    void main() {
      v_Coords = a_Position.xy;
      gl_Position = u_MVP * a_Position;
    })glsl",

                                                        R"glsl(#version 300 es
    #extension GL_OVR_multiview2 : enable

    layout(num_views=2) in;
    uniform mat4 u_MVP[2];
    in vec4 a_Position;
    out vec2 v_Coords;

    void main() {
      v_Coords = a_Position.xy;
      gl_Position = u_MVP[gl_ViewID_OVR] * a_Position;
    })glsl"};

static constexpr const char* kReticleFragmentShaders[] = {R"glsl(
    precision mediump float;

    varying vec2 v_Coords;

    void main() {
      float r = length(v_Coords);
      float alpha = smoothstep(0.5, 0.6, r) * (1.0 - smoothstep(0.8, 0.9, r));
      if (alpha == 0.0) discard;
      gl_FragColor = vec4(alpha);
    })glsl",

                                                          R"glsl(#version 300 es
    precision mediump float;

    in vec2 v_Coords;
    out vec4 FragColor;

    void main() {
      float r = length(v_Coords);
      float alpha = smoothstep(0.5, 0.6, r) * (1.0 - smoothstep(0.8, 0.9, r));
      if (alpha == 0.0) discard;
      FragColor = vec4(alpha);
    })glsl"};

#endif  // HELLOVR_APP_SRC_MAIN_JNI_HELLO_VR_SHADERS_H_  // NOLINT
