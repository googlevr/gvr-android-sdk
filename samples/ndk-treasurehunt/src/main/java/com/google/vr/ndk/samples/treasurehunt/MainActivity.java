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

package com.google.vr.ndk.samples.treasurehunt;

import android.app.Activity;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Vibrator;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * A Google VR NDK sample application.
 *
 * <p>This app presents a scene consisting of a planar ground grid and a floating "treasure" cube.
 * When the user finds the "treasure", they can invoke the trigger action, and the cube will be
 * randomly repositioned. When in Cardboard mode, the user must gaze at the cube and use the
 * Cardboard trigger button. When in Daydream mode, the user can use the controller to position the
 * cursor, and use the controller buttons to invoke the trigger action.
 *
 * <p>This is the main Activity for the sample application. It initializes a GLSurfaceView to allow
 * rendering, a GvrLayout for GVR API access, and forwards relevant events to the native renderer
 * where rendering and interaction are handled.
 */
public class MainActivity extends Activity {
  static {
    System.loadLibrary("gvr");
    System.loadLibrary("gvr_audio");
    System.loadLibrary("treasurehunt_jni");
  }

  // Opaque native pointer to the native TreasureHuntRenderer instance.
  private long nativeTreasureHuntRenderer;

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;

  // Note that pause and resume signals to the native renderer are performed on the GL thread,
  // ensuring thread-safety.
  private final Runnable pauseNativeRunnable =
      new Runnable() {
        @Override
        public void run() {
          nativeOnPause(nativeTreasureHuntRenderer);
        }
      };

  private final Runnable resumeNativeRunnable =
      new Runnable() {
        @Override
        public void run() {
          nativeOnResume(nativeTreasureHuntRenderer);
        }
      };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Ensure fullscreen immersion.
    setImmersiveSticky();
    getWindow()
        .getDecorView()
        .setOnSystemUiVisibilityChangeListener(
            new View.OnSystemUiVisibilityChangeListener() {
              @Override
              public void onSystemUiVisibilityChange(int visibility) {
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                  setImmersiveSticky();
                }
              }
            });

    // Initialize GvrLayout and the native renderer.
    gvrLayout = new GvrLayout(this);
    nativeTreasureHuntRenderer =
        nativeCreateRenderer(
            getClass().getClassLoader(),
            this.getApplicationContext(),
            gvrLayout.getGvrApi().getNativeGvrContext());

    // Add the GLSurfaceView to the GvrLayout.
    surfaceView = new GLSurfaceView(this);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setRenderer(
        new GLSurfaceView.Renderer() {
          @Override
          public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            nativeInitializeGl(nativeTreasureHuntRenderer);
          }

          @Override
          public void onSurfaceChanged(GL10 gl, int width, int height) {}

          @Override
          public void onDrawFrame(GL10 gl) {
            nativeDrawFrame(nativeTreasureHuntRenderer);
          }
        });
    surfaceView.setOnTouchListener(
        new View.OnTouchListener() {
          @Override
          public boolean onTouch(View v, MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
              // Give user feedback and signal a trigger event.
              ((Vibrator) getSystemService(Context.VIBRATOR_SERVICE)).vibrate(50);
              surfaceView.queueEvent(
                  new Runnable() {
                    @Override
                    public void run() {
                      nativeOnTriggerEvent(nativeTreasureHuntRenderer);
                    }
                  });
              return true;
            }
            return false;
          }
        });
    gvrLayout.setPresentationView(surfaceView);

    // Add the GvrLayout to the View hierarchy.
    setContentView(gvrLayout);

    // Enable scan line racing.
    if (gvrLayout.setAsyncReprojectionEnabled(true)) {
      // Scanline racing decouples the app framerate from the display framerate,
      // allowing immersive interaction even at the throttled clockrates set by
      // sustained performance mode.
      AndroidCompat.setSustainedPerformanceMode(this, true);
    }

    // Enable VR Mode.
    AndroidCompat.setVrModeEnabled(this, true);
  }

  @Override
  protected void onPause() {
    surfaceView.queueEvent(pauseNativeRunnable);
    surfaceView.onPause();
    gvrLayout.onPause();
    super.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();
    gvrLayout.onResume();
    surfaceView.onResume();
    surfaceView.queueEvent(resumeNativeRunnable);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    // Destruction order is important; shutting down the GvrLayout will detach
    // the GLSurfaceView and stop the GL thread, allowing safe shutdown of
    // native resources from the UI thread.
    gvrLayout.shutdown();
    nativeDestroyRenderer(nativeTreasureHuntRenderer);
    nativeTreasureHuntRenderer = 0;
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {
    // Avoid accidental volume key presses while the phone is in the VR headset.
    if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP
        || event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) {
      return true;
    }
    return super.dispatchKeyEvent(event);
  }

  private void setImmersiveSticky() {
    getWindow()
        .getDecorView()
        .setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
  }

  private native long nativeCreateRenderer(
      ClassLoader appClassLoader, Context context, long nativeGvrContext);
  private native void nativeDestroyRenderer(long nativeTreasureHuntRenderer);
  private native void nativeInitializeGl(long nativeTreasureHuntRenderer);
  private native long nativeDrawFrame(long nativeTreasureHuntRenderer);
  private native void nativeOnTriggerEvent(long nativeTreasureHuntRenderer);
  private native void nativeOnPause(long nativeTreasureHuntRenderer);
  private native void nativeOnResume(long nativeTreasureHuntRenderer);
}
