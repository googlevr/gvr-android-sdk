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

#ifndef TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTSHADERS_H_  // NOLINT
#define TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTSHADERS_H_  // NOLINT

// Each shader has two variants: a single-eye ES 2.0 variant, and a multiview
// ES 3.0 variant.  The multiview vertex shaders use transforms defined by
// arrays of mat4 uniforms, using gl_ViewID_OVR to determine the array index.

static const char* kDiffuseLightingVertexShaders[] = {
    R"glsl(
    uniform mat4 u_Model;
    uniform mat4 u_MVP;
    uniform mat4 u_MVMatrix;
    uniform vec3 u_LightPos;
    attribute vec4 a_Position;
    attribute vec4 a_Color;
    attribute vec3 a_Normal;
    varying vec4 v_Color;
    varying vec3 v_Grid;

    void main() {
      v_Grid = vec3(u_Model * a_Position);
      vec3 modelViewVertex = vec3(u_MVMatrix * a_Position);
      vec3 modelViewNormal = vec3(u_MVMatrix * vec4(a_Normal, 0.0));
      float distance = length(u_LightPos - modelViewVertex);
      vec3 lightVector = normalize(u_LightPos - modelViewVertex);
      float diffuse = max(dot(modelViewNormal, lightVector), 0.5);
      diffuse = diffuse * (1.0 / (1.0 + (0.00001 * distance * distance)));
      v_Color = vec4(a_Color.rgb * diffuse, a_Color.a);
      gl_Position = u_MVP * a_Position;
    })glsl",

    R"glsl(#version 300 es
    #extension GL_OVR_multiview2 : enable

    layout(num_views=2) in;

    uniform mat4 u_Model;
    uniform mat4 u_MVP[2];
    uniform mat4 u_MVMatrix[2];
    uniform vec3 u_LightPos[2];
    in vec4 a_Position;
    in vec4 a_Color;
    in vec3 a_Normal;
    out vec4 v_Color;
    out vec3 v_Grid;

    void main() {
      mat4 mvp = u_MVP[gl_ViewID_OVR];
      mat4 modelview = u_MVMatrix[gl_ViewID_OVR];
      vec3 lightpos = u_LightPos[gl_ViewID_OVR];
      v_Grid = vec3(u_Model * a_Position);
      vec3 modelViewVertex = vec3(modelview * a_Position);
      vec3 modelViewNormal = vec3(modelview * vec4(a_Normal, 0.0));
      float distance = length(lightpos - modelViewVertex);
      vec3 lightVector = normalize(lightpos - modelViewVertex);
      float diffuse = max(dot(modelViewNormal, lightVector), 0.5);
      diffuse = diffuse * (1.0 / (1.0 + (0.00001 * distance * distance)));
      v_Color = vec4(a_Color.rgb * diffuse, a_Color.a);
      gl_Position = mvp * a_Position;
    })glsl"
};

static const char* kGridFragmentShaders[] = {
    R"glsl(
    precision mediump float;
    varying vec4 v_Color;
    varying vec3 v_Grid;

    void main() {
      float depth = gl_FragCoord.z / gl_FragCoord.w;
      if ((mod(abs(v_Grid.x), 1.0) < 0.01) ||
          (mod(abs(v_Grid.z), 1.0) < 0.01)) {
        gl_FragColor = max(0.0, (9.0-depth) / 9.0) *
                       vec4(1.0, 1.0, 1.0, 1.0) +
                       min(1.0, depth / 9.0) * v_Color;
      } else {
        gl_FragColor = v_Color;
      }
    })glsl",

    R"glsl(#version 300 es

    precision mediump float;
    in vec4 v_Color;
    in vec3 v_Grid;
    out vec4 FragColor;

    void main() {
      float depth = gl_FragCoord.z / gl_FragCoord.w;
      if ((mod(abs(v_Grid.x), 1.0) < 0.01) ||
          (mod(abs(v_Grid.z), 1.0) < 0.01)) {
        FragColor = max(0.0, (9.0-depth) / 9.0) *
                       vec4(1.0, 1.0, 1.0, 1.0) +
                       min(1.0, depth / 9.0) * v_Color;
      } else {
        FragColor = v_Color;
      }
    })glsl"
};

static const char* kPassthroughFragmentShaders[] = {
    R"glsl(
    precision mediump float;
    varying vec4 v_Color;

    void main() {
      gl_FragColor = v_Color;
    })glsl",

    R"glsl(#version 300 es

    precision mediump float;
    in vec4 v_Color;
    out vec4 FragColor;

    void main() {
      FragColor = v_Color;
    })glsl"
};

static const char* kReticleVertexShaders[] = { R"glsl(
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
    })glsl"
};

static const char* kReticleFragmentShaders[] = { R"glsl(
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
    })glsl"
};

#endif  // TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTSHADERS_H_ // NOLINT
