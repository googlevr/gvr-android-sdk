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

import com.google.vr.ndk.base.GvrLayout;
import com.google.vr.ndk.base.GvrUiLayout;

import android.app.Activity;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Main Activity.
 *
 * This is the main Activity for this demo app. It consists of a GLSurfaceView that is
 * responsible for doing the rendering. We forward all of the interesting events to
 * native code.
 */
public class MainActivity extends Activity {
  private static final String TAG = "MainActivity";

  private static final String VR_MODE_PACKAGE = "com.google.vr.vrcore";
  private static final String VR_MODE_CLASS =
      "com.google.vr.vrcore.common.VrCoreListenerService";

  static {
    // Load our JNI code.
    System.loadLibrary("app_jni");
  }

  private native void nativeOnCreate(AssetManager assetManager, long gvrContextPtr);
  private native void nativeOnResume();
  private native void nativeOnPause();
  private native void nativeOnSurfaceCreated();
  private native void nativeOnSurfaceChanged(int width, int height);
  private native void nativeOnDrawFrame();
  private native void nativeOnDestroy();

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;
  private TextView debugView;
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
    com.google.vr.ndk.base.AndroidCompat.setVrModeEnabled(this, true);

    // Get the GvrLayout.
    gvrLayout = new GvrLayout(this);

    // Configure the GLSurfaceView.
    surfaceView = new GLSurfaceView(this);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(8, 8, 8, 8, 16, 8);
    surfaceView.setPreserveEGLContextOnPause(true);
    surfaceView.setRenderer(renderer);

    // Set the GLSurfaceView as the GvrLayout's presentation view.
    gvrLayout.setPresentationView(surfaceView);
    setContentView(gvrLayout);

    // Add UI layer.
    gvrLayout.addView(new GvrUiLayout(this));

    assetManager = getResources().getAssets();

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
    nativeOnDestroy();
  }

  @Override
  protected void onPause() {
    surfaceView.onPause();
    gvrLayout.onPause();
    nativeOnPause();
    super.onPause();
  }

  @Override
  protected void onResume() {
    super.onResume();
    gvrLayout.onResume();
    surfaceView.onResume();
    nativeOnResume();
  }

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
  }

  private void setImmersiveSticky() {
    getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
        | View.SYSTEM_UI_FLAG_FULLSCREEN
        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
  }

  private final GLSurfaceView.Renderer renderer = new GLSurfaceView.Renderer() {
    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
      nativeOnSurfaceCreated();
    }
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
      nativeOnSurfaceChanged(width, height);
    }
    @Override
    public void onDrawFrame(GL10 gl) {
      nativeOnDrawFrame();
    }
  };
}

