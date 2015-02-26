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

package com.google.vrtoolkit.cardboard.samples.treasurehunt;

/**
 * Contains vertex, normal and color data.
 */
public final class WorldLayoutData {

  public static final float[] CUBE_COORDS = new float[] {
      // Front face
      -1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,

      // Right face
      1.0f, 1.0f, 1.0f,
      1.0f, -1.0f, 1.0f,
      1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, 1.0f, -1.0f,

      // Back face
      1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,

      // Left face
      -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f,

      // Top face
      -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, -1.0f,

      // Bottom face
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f,
  };

  public static final float[] CUBE_COLORS = new float[] {
      // front, green
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,

      // right, blue
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,

      // back, also green
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,
      0f, 0.5273f, 0.2656f, 1.0f,

      // left, also blue
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,

      // top, red
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,

      // bottom, also red
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
      0.8359375f,  0.17578125f,  0.125f, 1.0f,
  };

  public static final float[] CUBE_FOUND_COLORS = new float[] {
      // front, yellow
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,

      // right, yellow
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,

      // back, yellow
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,

      // left, yellow
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,

      // top, yellow
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,

      // bottom, yellow
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
      1.0f,  0.6523f, 0.0f, 1.0f,
  };

  public static final float[] CUBE_NORMALS = new float[] {
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
  };

  public static final float[] FLOOR_COORDS = new float[] {
      200f, 0, -200f,
      -200f, 0, -200f,
      -200f, 0, 200f,
      200f, 0, -200f,
      -200f, 0, 200f,
      200f, 0, 200f,
  };

  public static final float[] FLOOR_NORMALS = new float[] {
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
  };

  public static final float[] FLOOR_COLORS = new float[] {
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
  };
}
