/*
 * Copyright 2016 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.vr.ndk.samples.controllerpaint;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Main Activity.
 *
 * <p>This is the main Activity for this demo app. It consists of a GLSurfaceView that is
 * responsible for doing the rendering. We forward all of the interesting events to native code.
 */
public class MainActivity extends Activity {
  private static final String TAG = "MainActivity";
  // Opaque native pointer to the DemoApp C++ object.
  // This object is owned by the MainActivity instance and passed to the native methods.
  private long nativeControllerPaint;

  static {
    // Load our JNI code.
    System.loadLibrary("app_jni");
  }

  private native long nativeOnCreate(AssetManager assetManager, long gvrContextPtr);

  private native void nativeOnResume(long controllerPaintJptr);

  private native void nativeOnPause(long controllerPaintJptr);

  private native void nativeOnSurfaceCreated(long controllerPaintJptr);

  private native void nativeOnSurfaceChanged(int width, int height, long controllerPaintJptr);

  private native void nativeOnDrawFrame(long controllerPaintJptr);

  private native void nativeOnDestroy(long controllerPaintJptr);

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;
  private AssetManager assetManager;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

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

    // Enable VR mode, if the device supports it.
    AndroidCompat.setVrModeEnabled(this, true);

    // Get the GvrLayout.
    gvrLayout = new GvrLayout(this);

    // Enable scan line racing, if possible.
    if (gvrLayout.setAsyncReprojectionEnabled(true)) {
      Log.d(TAG, "Successfully enabled scanline racing.");
      // Scanline racing decouples the app framerate from the display framerate,
      // allowing immersive interaction even at the throttled clockrates set by
      // sustained performance mode.
      AndroidCompat.setSustainedPerformanceMode(this, true);
    } else {
      Log.w(TAG, "Failed to enable scanline racing.");
    }

    // Configure the GLSurfaceView.
    surfaceView = new GLSurfaceView(this);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setRenderer(renderer);

    // Set the GLSurfaceView as the GvrLayout's presentation view.
    gvrLayout.setPresentationView(surfaceView);

    // Add the GvrLayout to the View hierarchy.
    setContentView(gvrLayout);

    assetManager = getResources().getAssets();

    nativeControllerPaint =
        nativeOnCreate(assetManager, gvrLayout.getGvrApi().getNativeGvrContext());

    // Prevent screen from dimming/locking.
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    // Destruction order is important; shutting down the GvrLayout will detach
    // the GLSurfaceView and stop the GL thread, allowing safe shutdown of
    // native resources from the UI thread.
    gvrLayout.shutdown();
    nativeOnDestroy(nativeControllerPaint);
    nativeControllerPaint = 0;
  }

  @Override
  protected void onPause() {
    surfaceView.onPause();
    gvrLayout.onPause();
    nativeOnPause(nativeControllerPaint);
    super.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();
    gvrLayout.onResume();
    surfaceView.onResume();
    nativeOnResume(nativeControllerPaint);
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

  private final GLSurfaceView.Renderer renderer =
      new GLSurfaceView.Renderer() {
        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
          nativeOnSurfaceCreated(nativeControllerPaint);
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
          nativeOnSurfaceChanged(width, height, nativeControllerPaint);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
          nativeOnDrawFrame(nativeControllerPaint);
        }
      };
}
