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

package com.google.vr.sdk.samples.video360.rendering;

import static com.google.vr.sdk.samples.video360.rendering.Utils.checkGlError;

import android.content.Context;
import android.graphics.PointF;
import android.graphics.SurfaceTexture;
import android.graphics.SurfaceTexture.OnFrameAvailableListener;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.support.annotation.AnyThread;
import android.support.annotation.BinderThread;
import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.util.Log;
import android.util.Pair;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.ViewGroup;
import com.google.vr.sdk.controller.Orientation;
import com.google.vr.sdk.samples.video360.VideoUiView;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Controls and renders the GL Scene.
 *
 * <p>This class is shared between MonoscopicView & VrVideoActivity. It renders the display mesh, UI
 * and controller reticle as required. It also has basic Controller input which allows the user to
 * interact with {@link VideoUiView} while in VR.
 */
public final class SceneRenderer {
  private static final String TAG = "SceneRenderer";

  // This is the primary interface between the Media Player and the GL Scene.
  private SurfaceTexture displayTexture;
  private final AtomicBoolean frameAvailable = new AtomicBoolean();
  // Used to notify clients that displayTexture has a new frame. This requires synchronized access.
  @Nullable
  private OnFrameAvailableListener externalFrameListener;

  // GL components for the mesh that display the media. displayMesh should only be accessed on the
  // GL Thread, but requestedDisplayMesh needs synchronization.
  @Nullable
  private Mesh displayMesh;
  @Nullable
  private Mesh requestedDisplayMesh;
  private int displayTexId;

  // These are only valid if createForVR() has been called. In the 2D Activity, these are null
  // since the UI is rendered in the standard Android layout.
  @Nullable
  private final CanvasQuad canvasQuad;
  @Nullable
  private final VideoUiView videoUiView;
  @Nullable
  private final Handler uiHandler;

  // Controller components.
  private final Reticle reticle = new Reticle();
  @Nullable
  private Orientation controllerOrientation;
  // This is accessed on the binder & GL Threads.
  private final float[] controllerOrientationMatrix = new float[16];

  /**
   * Constructs the SceneRenderer with the given values.
   */
  /* package */ SceneRenderer(
      CanvasQuad canvasQuad, VideoUiView videoUiView, Handler uiHandler,
      SurfaceTexture.OnFrameAvailableListener externalFrameListener) {
    this.canvasQuad = canvasQuad;
    this.videoUiView = videoUiView;
    this.uiHandler = uiHandler;
    this.externalFrameListener = externalFrameListener;
  }

  /**
   * Creates a SceneRenderer for 2D but does not initialize it. {@link #glInit()} is used to finish
   * initializing the object on the GL thread.
   */
  public static SceneRenderer createFor2D() {
    return new SceneRenderer(null, null, null, null);
  }

  /**
   * Creates a SceneRenderer for VR but does not initialize it. {@link #glInit()} is used to finish
   * initializing the object on the GL thread.
   *
   * <p>The also creates a {@link VideoUiView} that is bound to the VR scene. The View is backed by
   * a {@link CanvasQuad} and is meant to be rendered in a VR scene.
   *
   * @param context the {@link Context} used to initialize the {@link VideoUiView}
   * @param parent the new view is attached to the parent in order to properly handle Android
   *     events
   * @return a SceneRender configured for VR and a bound {@link VideoUiView} that can be treated
   *     similar to a View returned from findViewById.
   */
  @MainThread
  public static Pair<SceneRenderer, VideoUiView> createForVR(Context context, ViewGroup parent) {
    CanvasQuad canvasQuad = new CanvasQuad();
    VideoUiView videoUiView = VideoUiView.createForOpenGl(context, parent, canvasQuad);
    OnFrameAvailableListener externalFrameListener = videoUiView.getFrameListener();

    SceneRenderer scene = new SceneRenderer(
        canvasQuad, videoUiView, new Handler(Looper.getMainLooper()), externalFrameListener);
    return Pair.create(scene, videoUiView);
  }

  /**
   * Performs initialization on the GL thread. The scene isn't fully initialized until
   * glConfigureScene() completes successfully.
   */
  public void glInit() {
    checkGlError();
    Matrix.setIdentityM(controllerOrientationMatrix, 0);

    // Set the background frame color. This is only visible if the display mesh isn't a full sphere.
    GLES20.glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    checkGlError();

    // Create the texture used to render each frame of video.
    displayTexId = Utils.glCreateExternalTexture();
    displayTexture = new SurfaceTexture(displayTexId);
    checkGlError();

    // When the video decodes a new frame, tell the GL thread to update the image.
    displayTexture.setOnFrameAvailableListener(
        new OnFrameAvailableListener() {
          @Override
          public void onFrameAvailable(SurfaceTexture surfaceTexture) {
            frameAvailable.set(true);

            synchronized (SceneRenderer.this) {
              if (externalFrameListener != null) {
                externalFrameListener.onFrameAvailable(surfaceTexture);
              }
            }
          }
        });

    if (canvasQuad != null) {
      canvasQuad.glInit();
    }
    reticle.glInit();
  }

  /**
   * Creates the Surface & Mesh used by the MediaPlayer to render video.
   *
   * @param width passed to {@link SurfaceTexture#setDefaultBufferSize(int, int)}
   * @param height passed to {@link SurfaceTexture#setDefaultBufferSize(int, int)}
   * @param mesh {@link Mesh} used to display video
   * @return a Surface that can be passed to {@link android.media.MediaPlayer#setSurface(Surface)}
   */
  @AnyThread
  public synchronized @Nullable Surface createDisplay(int width, int height, Mesh mesh) {
    if (displayTexture == null) {
      Log.e(TAG, ".createDisplay called before GL Initialization completed.");
      return null;
    }

    requestedDisplayMesh = mesh;

    displayTexture.setDefaultBufferSize(width, height);
    return new Surface(displayTexture);
  }

  /**
   * Configures any late-initialized components.
   *
   * <p>Since the creation of the Mesh can depend on disk access, this configuration needs to run
   * during each drawFrame to determine if the Mesh is ready yet. This also supports replacing an
   * existing mesh while the app is running.
   *
   * @return true if the scene is ready to be drawn
   */
  private synchronized boolean glConfigureScene() {
    if (displayMesh == null && requestedDisplayMesh == null) {
      // The scene isn't ready and we don't have enough information to configure it.
      return false;
    }

    // The scene is ready and we don't need to change it so we can glDraw it.
    if (requestedDisplayMesh == null) {
      return true;
    }

    // Configure or reconfigure the scene.
    if (displayMesh != null) {
      // Reconfiguration.
      displayMesh.glShutdown();
    }

    displayMesh = requestedDisplayMesh;
    requestedDisplayMesh = null;
    displayMesh.glInit(displayTexId);

    return true;
  }

  /**
   * Draws the scene with a given eye pose and type.
   *
   * @param viewProjectionMatrix 16 element GL matrix.
   * @param eyeType an {@link com.google.vr.sdk.base.Eye.Type} value
   */
  public void glDrawFrame(float[] viewProjectionMatrix, int eyeType) {
    if (!glConfigureScene()) {
      // displayMesh isn't ready.
      return;
    }

    // glClear isn't strictly necessary when rendering fully spherical panoramas, but it can improve
    // performance on tiled renderers by causing the GPU to discard previous data.
    GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
    checkGlError();

    // The uiQuad uses alpha.
    GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
    GLES20.glEnable(GLES20.GL_BLEND);

    if (frameAvailable.compareAndSet(true, false)) {
      displayTexture.updateTexImage();
      checkGlError();
    }

    displayMesh.glDraw(viewProjectionMatrix, eyeType);
    if (videoUiView != null) {
      canvasQuad.glDraw(viewProjectionMatrix, videoUiView.getAlpha());
    }

    reticle.glDraw(viewProjectionMatrix, controllerOrientationMatrix);
  }

  /** Cleans up the GL resources. */
  public void glShutdown() {
    if (displayMesh != null) {
      displayMesh.glShutdown();
    }
    if (canvasQuad != null) {
      canvasQuad.glShutdown();
    }
    reticle.glShutdown();
  }

  /** Updates the Reticle's position with the latest Controller pose. */
  @BinderThread
  public synchronized void setControllerOrientation(Orientation currentOrientation) {
    this.controllerOrientation = currentOrientation;
    controllerOrientation.toRotationMatrix(controllerOrientationMatrix);
  }

  /**
   * Processes Daydream Controller clicks and dispatches the event to {@link VideoUiView} as a
   * synthetic {@link MotionEvent}.
   *
   * <p>This is a minimal input system that works because CanvasQuad is a simple rectangle with a
   * hardcoded location. If the quad had a transformation matrix, then those transformations would
   * need to be used when converting from the Controller's pose to a 2D click event.
   */
  @MainThread
  public void handleClick() {
    if (videoUiView.getAlpha() == 0) {
      // When the UI is hidden, clicking anywhere will make it visible.
      toggleUi();
      return;
    }

    if (controllerOrientation == null) {
      // Race condition between click & pose events.
      return;
    }

    final PointF clickTarget = CanvasQuad.translateClick(controllerOrientation);
    if (clickTarget == null) {
      // When the click is outside of the View, hide the UI.
      toggleUi();
      return;
    }

    // The actual processing of the synthetic event needs to happen in the UI thread.
    uiHandler.post(
        new Runnable() {
          @Override
          public void run() {
            // Generate a pair of down/up events to make the Android View processing handle the
            // click.
            long now = SystemClock.uptimeMillis();
            MotionEvent down = MotionEvent.obtain(
                now, now,  // Timestamps.
                MotionEvent.ACTION_DOWN, clickTarget.x, clickTarget.y,  // The important parts.
                1, 1, 0, 1, 1, 0, 0);  // Unused config data.
            down.setSource(InputDevice.SOURCE_GAMEPAD);
            videoUiView.dispatchTouchEvent(down);

            // Clone the down event but change action.
            MotionEvent up = MotionEvent.obtain(down);
            up.setAction(MotionEvent.ACTION_UP);
            videoUiView.dispatchTouchEvent(up);
          }
        });
  }

  /** Uses Android's animation system to fade in/out when the user wants to show/hide the UI. */
  @AnyThread
  public void toggleUi() {
    // This can be trigged via a controller action so switch to main thread to manipulate the View.
    uiHandler.post(
        new Runnable() {
          @Override
          public void run() {
            if (videoUiView.getAlpha() == 0) {
              videoUiView.animate().alpha(1).start();
            } else {
              videoUiView.animate().alpha(0).start();
            }
          }
        });
  }

  /**
   * Binds a listener used by external clients that need to know when a new video frame is ready.
   * This is used by MonoscopicView to update the video position slider each frame.
   */
  @AnyThread
  public synchronized void setVideoFrameListener(OnFrameAvailableListener videoFrameListener) {
    externalFrameListener = videoFrameListener;
  }
}
