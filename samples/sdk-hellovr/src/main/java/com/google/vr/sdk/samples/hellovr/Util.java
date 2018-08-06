/*
 * Copyright 2018 Google Inc. All Rights Reserved.
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

package com.google.vr.sdk.samples.hellovr;

import static android.opengl.GLU.gluErrorString;

import android.opengl.GLES20;
import android.opengl.Matrix;
import android.text.TextUtils;
import android.util.Log;

/** Utility functions. */
/* package */ class Util {
  private static final String TAG = "Util";

  /** Debug builds should fail quickly. Release versions of the app should have this disabled. */
  private static final boolean HALT_ON_GL_ERROR = true;

  /** Class only contains static methods. */
  private Util() {}

  /**
   * Checks GLES20.glGetError and fails quickly if the state isn't GL_NO_ERROR.
   *
   * @param label Label to report in case of error.
   */
  public static void checkGlError(String label) {
    int error = GLES20.glGetError();
    int lastError;
    if (error != GLES20.GL_NO_ERROR) {
      do {
        lastError = error;
        Log.e(TAG, label + ": glError " + gluErrorString(lastError));
        error = GLES20.glGetError();
      } while (error != GLES20.GL_NO_ERROR);

      if (HALT_ON_GL_ERROR) {
        throw new RuntimeException("glError " + gluErrorString(lastError));
      }
    }
  }

  /**
   * Builds a GL shader program from vertex & fragment shader code. The vertex and fragment shaders
   * are passed as arrays of strings in order to make debugging compilation issues easier.
   *
   * @param vertexCode GLES20 vertex shader program.
   * @param fragmentCode GLES20 fragment shader program.
   * @return GLES20 program id.
   */
  public static int compileProgram(String[] vertexCode, String[] fragmentCode) {
    checkGlError("Start of compileProgram");
    // prepare shaders and OpenGL program
    int vertexShader = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);
    GLES20.glShaderSource(vertexShader, TextUtils.join("\n", vertexCode));
    GLES20.glCompileShader(vertexShader);
    checkGlError("Compile vertex shader");

    int fragmentShader = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);
    GLES20.glShaderSource(fragmentShader, TextUtils.join("\n", fragmentCode));
    GLES20.glCompileShader(fragmentShader);
    checkGlError("Compile fragment shader");

    int program = GLES20.glCreateProgram();
    GLES20.glAttachShader(program, vertexShader);
    GLES20.glAttachShader(program, fragmentShader);

    // Link and check for errors.
    GLES20.glLinkProgram(program);
    int[] linkStatus = new int[1];
    GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
    if (linkStatus[0] != GLES20.GL_TRUE) {
      String errorMsg = "Unable to link shader program: \n" + GLES20.glGetProgramInfoLog(program);
      Log.e(TAG, errorMsg);
      if (HALT_ON_GL_ERROR) {
        throw new RuntimeException(errorMsg);
      }
    }
    checkGlError("End of compileProgram");

    return program;
  }

  /**
   * Computes the angle between two vectors; see
   * https://en.wikipedia.org/wiki/Vector_projection#Definitions_in_terms_of_a_and_b.
   */
  public static float angleBetweenVectors(float[] vec1, float[] vec2) {
    float cosOfAngle = dotProduct(vec1, vec2) / (vectorNorm(vec1) * vectorNorm(vec2));
    return (float) Math.acos(Math.max(-1.0f, Math.min(1.0f, cosOfAngle)));
  }

  private static float dotProduct(float[] vec1, float[] vec2) {
    return vec1[0] * vec2[0] + vec1[1] * vec2[1] + vec1[2] * vec2[2];
  }

  private static float vectorNorm(float[] vec) {
    return Matrix.length(vec[0], vec[1], vec[2]);
  }
}
