/*
 * Copyright 2017 Google Inc. All Rights Reserved.
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
package com.google.vr.sdk.samples.videoplayer;

import android.content.Context;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.opengl.Matrix;
import com.google.android.exoplayer2.decoder.DecoderCounters;
import com.google.vr.ndk.base.BufferViewport;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.TimeUnit;

/**
 * Handles positioning the video in the correct place in the scene and rendering a transparent hole
 * into the color buffer in the same place. All methods in this class should be called on the
 * application's GL thread, unless otherwise noted.
 */
public class VideoScene {
  private static final String TAG = "VideoScreen";
  private static final RectF videoUv = new RectF(0.f, 1.f, 1.f, 0.f);

  private final Settings settings;
  // Helper object for GL resources used by the scene.
  private final Resources resources = new Resources();
  // Scratch array for the transform from SPRITE_VERTICES_DATA space to eye space.
  private final float[] eyeFromQuad = new float[16];
  // Scratch array for the transform from SPRITE_VERTICES_DATA space to perspective eye space.
  private final float[] perspectiveFromQuad = new float[16];
  // Transform from SPRITE_VERTICES_DATA space to world space. Set by setVideoTransform().
  private final float[] worldFromQuad = new float[16];
  // Transform from double-sided unit box with the video to the position of the frame rate bar.
  // Note that this is effectively just the offset and scale of the bar with respect to the video,
  // not its world position.
  private final float[] frameRateBarFromQuad = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 0.1f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, -1.2f, 0.f, 1.f
  };
  // Transform from SPRITE_VERTICES_DATA space to world space for the framerate bar under the video.
  private final float[] worldFromFrameRateBar = new float[16];

  private final TreeMap<Long, Integer> frameCounts = new TreeMap<Long, Integer>();
  private volatile int videoSurfaceID = BufferViewport.EXTERNAL_SURFACE_ID_NONE;
  private volatile boolean isVideoPlaying = false;
  private float currentFpsFraction = 0.f;

  public VideoScene(Settings settings) {
    this.settings = settings;
  }

  /**
   * Sets whether video playback has started. If video playback has not started, the loading splash
   * screen is drawn.
   *
   * @param hasPlaybackStarted True if video is playing.
   */
  public void setHasVideoPlaybackStarted(boolean hasPlaybackStarted) {
    isVideoPlaying = hasPlaybackStarted;
  }

  /**
   * Set the ID of the external surface used to display the video.
   * Can be called from any thread. The ID will be updated on the next frame.
   */
  public void setVideoSurfaceId(int id) {
    videoSurfaceID = id;
  }

  /**
   * Specify where in the world space the video should appear.
   *
   * @param newWorldFromQuad Matrix in OpenGL format containing a transformation that positions
   *     a quad with vertices (1, 1, 0), (1, -1, 0), (-1, 1, 0), (-1, -1, 0) in the desired place
   *     in world space. The video will be shown at this quad's position.
   */
  public void setVideoTransform(float[] newWorldFromQuad) {
    System.arraycopy(newWorldFromQuad, 0, this.worldFromQuad, 0, 16);
  }

  /**
   * Update a viewport so that it positions the video in the correct place in the scene seen by the
   * user and references the correct external surface. Can be safely called from a different thread
   * than the setter functions.
   *
   * @param viewport Viewport to update.
   * @param eyeFromWorld Matrix in OpenGL format containing the eye-from-world transformation,
   *     i.e., without the projective component.
   */
  public void updateViewport(BufferViewport viewport, float[] eyeFromWorld) {
    Matrix.multiplyMM(eyeFromQuad, 0, eyeFromWorld, 0, worldFromQuad, 0);
    viewport.setSourceUv(videoUv);
    viewport.setSourceBufferIndex(BufferViewport.BUFFER_INDEX_EXTERNAL_SURFACE);
    viewport.setExternalSurfaceId(videoSurfaceID);
    viewport.setTransform(eyeFromQuad);
  }

  /**
   * Draws the hole punch or a sprite that is in the same position as the video
   *
   * @param perspectiveFromWorld Transformation from world space to clip space.
   */
  public void draw(float[] perspectiveFromWorld) {
    Matrix.multiplyMM(perspectiveFromQuad, 0, perspectiveFromWorld, 0, worldFromQuad, 0);
    int program;
    if (isVideoPlaying) {
      program = resources.solidColorProgram;
    } else {
      program = resources.spriteProgram;
    }

    GLES20.glUseProgram(program);
    GLUtil.checkGlError(TAG, "glUseProgram");

    if (program == resources.spriteProgram) {
      GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
      GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, resources.loadingTextureId);
      GLUtil.checkGlError(TAG, "glBindTexture");
      final int uImageTexture = GLES20.glGetUniformLocation(program, "uImageTexture");
      GLUtil.checkGlError(TAG, "glGetUniformLocation uImageTexture");
      if (uImageTexture == -1) {
        throw new RuntimeException("Could not get uniform location for uImageTexture");
      }
      GLES20.glUniform1i(uImageTexture, 0);
    } else {
      final int uColor = GLES20.glGetUniformLocation(program, "uColor");
      GLES20.glUniform4f(uColor, 0.f, 0.f, 0.f, 0.f);
    }

    final int positionAttribute = GLES20.glGetAttribLocation(program, "aPosition");
    GLUtil.checkGlError(TAG, "glGetAttribLocation aPosition");

    GLES20.glVertexAttribPointer(
        positionAttribute, 3, GLES20.GL_FLOAT, false, Resources.VERTEX_DATA_STRIDE_BYTES,
        resources.vertexPositions);
    GLUtil.checkGlError(TAG, "glVertexAttribPointer position");
    GLES20.glEnableVertexAttribArray(positionAttribute);
    GLUtil.checkGlError(TAG, "glEnableVertexAttribArray position handle");

    final int uvAttribute = GLES20.glGetAttribLocation(program, "aTextureCoord");
    GLUtil.checkGlError(TAG, "glGetAttribLocation aTextureCoord");
    if (uvAttribute >= 0) {
      GLES20.glVertexAttribPointer(
          uvAttribute, 2, GLES20.GL_FLOAT, false, Resources.VERTEX_DATA_STRIDE_BYTES,
          resources.vertexUVs);
      GLUtil.checkGlError(TAG, "glVertexAttribPointer uv handle");
      GLES20.glEnableVertexAttribArray(uvAttribute);
      GLUtil.checkGlError(TAG, "glEnableVertexAttribArray uv handle");
    }

    final int uMVPMatrix = GLES20.glGetUniformLocation(program, "uMVPMatrix");
    GLUtil.checkGlError(TAG, "glGetUniformLocation uMVPMatrix");
    if (uMVPMatrix == -1) {
      throw new RuntimeException("Could not get uniform location for uMVPMatrix");
    }
    GLES20.glUniformMatrix4fv(uMVPMatrix, 1, false, perspectiveFromQuad, 0);

    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, Resources.NUM_VERTICES);

    GLES20.glDisableVertexAttribArray(positionAttribute);
    if (uvAttribute >= 0) {
      GLES20.glDisableVertexAttribArray(uvAttribute);
    }
    GLUtil.checkGlError(TAG, "glDrawArrays");

    if (settings.showFrameRateBar) {
      drawVideoFrameRateBar(perspectiveFromWorld);
    }
  }

  private void drawVideoFrameRateBar(float[] perspectiveFromWorld) {
    // When the frame rate is 90% or less of native, we interpret this as a "bad" state.
    float colorFpsFraction = Math.max(0.f, (currentFpsFraction - 0.9f) / 0.1f);

    // Adjust the size of the bar and offset it to align its left end with the left edge of the
    // video quad.
    frameRateBarFromQuad[0] = currentFpsFraction;
    frameRateBarFromQuad[12] = -1.f + currentFpsFraction;
    Matrix.multiplyMM(worldFromFrameRateBar, 0, worldFromQuad, 0, frameRateBarFromQuad, 0);
    Matrix.multiplyMM(perspectiveFromQuad, 0, perspectiveFromWorld, 0, worldFromFrameRateBar, 0);

    GLES20.glUseProgram(resources.solidColorProgram);
    final int uColor = GLES20.glGetUniformLocation(resources.solidColorProgram, "uColor");
    // Fade between red and 80% gray when the video is DRM-protected. Fade between red and yellow
    // when the video is not protected.
    if (settings.useDrmVideoSample) {
      GLES20.glUniform4f(uColor, 1.f - 0.2f * colorFpsFraction, 0.8f * colorFpsFraction,
          0.8f * colorFpsFraction, 1.f);
    } else {
      GLES20.glUniform4f(uColor, 0.5f + 0.5f * colorFpsFraction, colorFpsFraction, 0.f, 1.f);
    }
    final int positionAttribute =
        GLES20.glGetAttribLocation(resources.solidColorProgram, "aPosition");
    GLES20.glVertexAttribPointer(
        positionAttribute, 3, GLES20.GL_FLOAT, false, Resources.VERTEX_DATA_STRIDE_BYTES,
        resources.vertexPositions);
    GLES20.glEnableVertexAttribArray(positionAttribute);
    final int uMVPMatrix = GLES20.glGetUniformLocation(resources.solidColorProgram, "uMVPMatrix");
    GLES20.glUniformMatrix4fv(uMVPMatrix, 1, false, perspectiveFromQuad, 0);
    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, Resources.NUM_VERTICES);
    GLES20.glDisableVertexAttribArray(positionAttribute);
    GLUtil.checkGlError(TAG, "frame rate bar");
  }

  /**
   * Updates the average fraction of the native frame rate of the video achieved over the last N
   * seconds, based on the passed DecoderCounters object.
   *
   * @param averagingPeriodInSeconds Compute the average over this many seconds in the past.
   * @param nativeFrameRate Native frame rate of the video.
   * @param counters DecoderCounters object retrieved from the video decoder.
   */
  public void updateVideoFpsFraction(
      long averagingPeriodInSeconds, float nativeFrameRate, DecoderCounters counters) {
    if (!settings.showFrameRateBar || counters == null) {
      currentFpsFraction = 0.f;
      return;
    }
    // Compute the frame rate over the last N seconds.
    final long nowTime = System.nanoTime();
    final long cutoffTime = nowTime - TimeUnit.SECONDS.toNanos(averagingPeriodInSeconds);
    long pastTime = 0L;
    counters.ensureUpdated();
    final int currentBufferCount = counters.renderedOutputBufferCount;
    int pastBufferCount = 0;

    // Insert the current buffer count into the map for future computations.
    frameCounts.put(nowTime, currentBufferCount);

    // Loop over the map, pruning outdated entries and stopping at the first one that is within the
    // cutoff time.
    for (Iterator<Map.Entry<Long, Integer>> iterator = frameCounts.entrySet().iterator();
        iterator.hasNext(); ) {
      Map.Entry<Long, Integer> count = iterator.next();
      if (count.getKey() < cutoffTime) {
        iterator.remove();
      } else {
        pastTime = count.getKey();
        pastBufferCount = count.getValue();
        break;
      }
    }

    // Compute the average fraction of the frame rate and clamp it to [0, 1].
    float elapsedSeconds = ((float) (nowTime - pastTime)) / 1e9f;
    float rawFraction =
        ((float) (currentBufferCount - pastBufferCount)) / (elapsedSeconds * nativeFrameRate);
    currentFpsFraction = Math.min(1.0f, Math.max(0.f, rawFraction));
  }

  /**
   * Create and load OpenGL resources.
   *
   * This needs to be called every time the GL context is re-created. There is no release
   * counterpart for now, since GL resources are automatically cleaned up when the GL context
   * is destroyed.
   *
   * @param context Android activity context used to load the resources.
   */
  public void prepareGLResources(Context context) {
    resources.prepare(context);
  }

  /**
   * Manages all GL resources used by video scenes. Only one copy of these resources is needed
   * for all VideoScene instances.
   */
  private static final class Resources { 
    static final String VERTEX_SHADER =
        "uniform mat4 uMVPMatrix;\n"
            + "attribute vec4 aPosition;\n"
            + "attribute vec4 aTextureCoord;\n"
            + "varying vec2 vTextureCoord;\n"
            + "void main() {\n"
            + "  gl_Position = uMVPMatrix * aPosition;\n"
            + "  vTextureCoord = aTextureCoord.st;\n"
            + "}\n";

    static final String SPRITE_FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "varying vec2 vTextureCoord;\n"
            + "uniform sampler2D uImageTexture;\n"
            + "void main() {\n"
            + "  gl_FragColor = texture2D(uImageTexture, vTextureCoord);\n"
            + "}\n";

    static final String SOLID_COLOR_FRAGMENT_SHADER =
        "precision mediump float;\n"
            + "uniform vec4 uColor;\n"
            + "varying vec2 vTextureCoord;\n"
            + "void main() {\n"
            + "  gl_FragColor = uColor;\n"
            + "}\n";

    static final float[] VERTEX_DATA = {
      // X,   Y,    Z,    U, V
      -1.0f,  1.0f, 0.0f, 1, 1,
       1.0f,  1.0f, 0.0f, 0, 1,
      -1.0f, -1.0f, 0.0f, 1, 0,
       1.0f, -1.0f, 0.0f, 0, 0,
    };

    static final int NUM_VERTICES = 4;
    static final int FLOAT_SIZE_BYTES = 4;
    static final int VERTEX_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
    static final int VERTEX_DATA_POS_OFFSET = 0;
    static final int VERTEX_DATA_UV_OFFSET = 3;

    int solidColorProgram = 0;
    int spriteProgram = 0;
    int loadingTextureId = 0;
    FloatBuffer vertexPositions;
    FloatBuffer vertexUVs;

    /* package */ void prepare(Context context) {
      // Prepare shader programs.
      solidColorProgram = GLUtil.createProgram(VERTEX_SHADER, SOLID_COLOR_FRAGMENT_SHADER);
      if (solidColorProgram == 0) {
        throw new RuntimeException("Could not create video program");
      }
      spriteProgram = GLUtil.createProgram(VERTEX_SHADER, SPRITE_FRAGMENT_SHADER);
      if (spriteProgram == 0) {
        throw new RuntimeException("Could not create sprite program");
      }

      // Prepare vertex data.
      ByteBuffer vertices = ByteBuffer.allocateDirect(VERTEX_DATA.length * FLOAT_SIZE_BYTES)
                                .order(ByteOrder.nativeOrder());
      vertexPositions = vertices.asFloatBuffer();
      vertexPositions.put(VERTEX_DATA);
      vertexPositions.position(VERTEX_DATA_POS_OFFSET);
      vertexUVs = vertices.asFloatBuffer();
      vertexUVs.position(VERTEX_DATA_UV_OFFSET);

      // Load the texture to be shown instead of the video while the latter is initializing.
      int[] textureIds = new int[1];
      GLES20.glGenTextures(1, textureIds, 0);
      loadingTextureId = textureIds[0];
      GLUtil.createResourceTexture(context, loadingTextureId, R.raw.loading_bg);
    }
  }
}
