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

package com.google.vr.sdk.samples.video360;

import android.Manifest.permission;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.opengl.Matrix;
import android.os.Bundle;
import android.support.annotation.MainThread;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import com.google.vr.ndk.base.DaydreamApi;
import com.google.vr.sdk.base.AndroidCompat;
import com.google.vr.sdk.base.Eye;
import com.google.vr.sdk.base.GvrActivity;
import com.google.vr.sdk.base.GvrView;
import com.google.vr.sdk.base.HeadTransform;
import com.google.vr.sdk.base.Viewport;
import com.google.vr.sdk.controller.Controller;
import com.google.vr.sdk.controller.ControllerManager;
import com.google.vr.sdk.samples.video360.rendering.SceneRenderer;
import javax.microedition.khronos.egl.EGLConfig;

/**
 * GVR Activity demonstrating a 360 video player.
 *
 * The default intent for this Activity will load a 360 placeholder panorama. For more options on
 * how to load other media using a custom Intent, see {@link MediaLoader}.
 */
public class VrVideoActivity extends GvrActivity {
  private static final String TAG = "VrVideoActivity";

  private static final int EXIT_FROM_VR_REQUEST_CODE = 42;

  private GvrView gvrView;
  private Renderer renderer;

  // Displays the controls for video playback.
  private VideoUiView uiView;

  // Given an intent with a media file and format, this will load the file and generate the mesh.
  private MediaLoader mediaLoader;

  // Interfaces with the Daydream controller.
  private ControllerManager controllerManager;
  private Controller controller;

  /**
   * Configures the VR system.
   *
   * @param savedInstanceState unused in this sample but it could be used to track video position
   */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    mediaLoader = new MediaLoader(this);

    gvrView = new GvrView(this);
    // Since the videos have fewer pixels per degree than the phones, reducing the render target
    // scaling factor reduces the work required to render the scene. This factor can be adjusted at
    // runtime depending on the resolution of the loaded video.
    // You can use Eye.getViewport() in the overridden onDrawEye() method to determine the current
    // render target size in pixels.
    gvrView.setRenderTargetScale(.5f);

    // Standard GvrView configuration
    renderer = new Renderer(gvrView);
    gvrView.setEGLConfigChooser(
        8, 8, 8, 8,  // RGBA bits.
        16,  // Depth bits.
        0);  // Stencil bits.
    gvrView.setRenderer(renderer);
    setContentView(gvrView);

    // Most Daydream phones can render a 4k video at 60fps in sustained performance mode. These
    // options can be tweaked along with the render target scale.
    if (gvrView.setAsyncReprojectionEnabled(true)) {
      AndroidCompat.setSustainedPerformanceMode(this, true);
    }

    // Handle the user clicking on the 'X' in the top left corner. Since this is done when the user
    // has taken the headset out of VR, it should launch the app's exit flow directly rather than
    // using the transition flow.
    gvrView.setOnCloseButtonListener(new Runnable() {
      @Override
      public void run() {
        launch2dActivity();
      }
    });

    // Configure Controller.
    ControllerEventListener listener = new ControllerEventListener();
    controllerManager = new ControllerManager(this, listener);
    controller = controllerManager.getController();
    controller.setEventListener(listener);
    // controller.start() is called in onResume().

    checkPermissionAndInitialize();
  }

  /**
   * Normal apps don't need this. However, since we use adb to interact with this sample, we
   * want any new adb Intents to be routed to the existing Activity rather than launching a new
   * Activity.
   */
  @Override
  protected void onNewIntent(Intent intent) {
    // Save the new Intent which may contain a new Uri. Then tear down & recreate this Activity to
    // load that Uri.
    setIntent(intent);
    recreate();
  }

  /** Launches the 2D app with the same extras and data. */
  private void launch2dActivity() {
    startActivity(new Intent(getIntent()).setClass(this, VideoActivity.class));
    // When launching the other Activity, it may be necessary to finish() this Activity in order to
    // free up the MediaPlayer resources. This sample doesn't call mediaPlayer.release() unless the
    // Activities are destroy()ed. This allows the video to be paused and resumed when another app
    // is in the foreground. However, most phones have trouble maintaining sufficient resources for
    // 2 4k videos in the same process. Large videos may fail to play in the second Activity if the
    // first Activity hasn't finish()ed.
    //
    // Alternatively, a single media player instance can be reused across multiple Activities in
    // order to conserve resources.
    finish();
  }

  /**
   * Tries to exit gracefully from VR using a VR transition dialog.
   *
   * @return whether the exit request has started or whether the request failed due to the device
   *     not being Daydream Ready
   */
  private boolean exitFromVr() {
    // This needs to use GVR's exit transition to avoid disorienting the user.
    DaydreamApi api = DaydreamApi.create(this);
    if (api != null) {
      api.exitFromVr(this, EXIT_FROM_VR_REQUEST_CODE, null);
      // Eventually, the Activity's onActivityResult will be called.
      api.close();
      return true;
    }
    return false;
  }

  /** Initializes the Activity only if the permission has been granted. */
  private void checkPermissionAndInitialize() {
    if (ContextCompat.checkSelfPermission(this, permission.READ_EXTERNAL_STORAGE)
        == PackageManager.PERMISSION_GRANTED) {
      mediaLoader.handleIntent(getIntent(), uiView);
    } else {
      exitFromVr();
      // This method will return false on Cardboard devices. This case isn't handled in this sample
      // but it should be handled for VR Activities that run on Cardboard devices.
    }
  }

  /**
   * Handles the result from {@link DaydreamApi#exitFromVr(Activity, int, Intent)}. This is called
   * via the uiView.setVrIconClickListener listener below.
   *
   * @param requestCode matches the parameter to exitFromVr()
   * @param resultCode whether the user accepted the exit request or canceled
   */
  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent unused) {
    if (requestCode == EXIT_FROM_VR_REQUEST_CODE && resultCode == RESULT_OK) {
      launch2dActivity();
    } else {
      // This should contain a VR UI to handle the user declining the exit request.
    }
  }

  @Override
  protected void onResume() {
    super.onResume();
    controllerManager.start();
    mediaLoader.resume();
  }

  @Override
  protected void onPause() {
    mediaLoader.pause();
    controllerManager.stop();
    super.onPause();
  }

  @Override
  protected void onDestroy() {
    mediaLoader.destroy();
    uiView.setMediaPlayer(null);
    super.onDestroy();
  }

  /**
   * Standard GVR renderer. Most of the real work is done by {@link SceneRenderer}.
   */
  private class Renderer implements GvrView.StereoRenderer {
    private static final float Z_NEAR = .1f;
    private static final float Z_FAR = 100;

    // Used by ControllerEventListener to manipulate the scene.
    public final SceneRenderer scene;

    private final float[] viewProjectionMatrix = new float[16];

    /**
     * Creates the Renderer and configures the VR exit button.
     *
     * @param parent Any View that is already attached to the Window. The uiView will secretly be
     *     attached to this View in order to properly handle UI events.
     */
    @MainThread
    public Renderer(ViewGroup parent) {
      Pair<SceneRenderer, VideoUiView> pair
          = SceneRenderer.createForVR(VrVideoActivity.this, parent);
      scene = pair.first;
      uiView = pair.second;
      uiView.setVrIconClickListener(
          new OnClickListener() {
            @Override
            public void onClick(View v) {
              if (!exitFromVr()) {
                // Directly exit Cardboard Activities.
                onActivityResult(EXIT_FROM_VR_REQUEST_CODE, RESULT_OK, null);
              }
            }
          });
    }

    @Override
    public void onNewFrame(HeadTransform headTransform) {}

    @Override
    public void onDrawEye(Eye eye) {
      Matrix.multiplyMM(
          viewProjectionMatrix, 0, eye.getPerspective(Z_NEAR, Z_FAR), 0, eye.getEyeView(), 0);
      scene.glDrawFrame(viewProjectionMatrix, eye.getType());
    }

    @Override
    public void onFinishFrame(Viewport viewport) {}

    @Override
    public void onSurfaceCreated(EGLConfig config) {
      scene.glInit();
      mediaLoader.onGlSceneReady(scene);
    }

    @Override
    public void onSurfaceChanged(int width, int height) { }

    @Override
    public void onRendererShutdown() {
      scene.glShutdown();
    }
  }

  /** Forwards Controller events to SceneRenderer. */
  private class ControllerEventListener extends Controller.EventListener
      implements ControllerManager.EventListener {
    private boolean touchpadDown = false;
    private boolean appButtonDown = false;

    @Override
    public void onApiStatusChanged(int status) {
      Log.i(TAG, ".onApiStatusChanged " + status);
    }

    @Override
    public void onRecentered() {}

    @Override
    public void onUpdate() {
      controller.update();

      renderer.scene.setControllerOrientation(controller.orientation);

      if (!touchpadDown && controller.clickButtonState) {
        renderer.scene.handleClick();
      }

      if (!appButtonDown && controller.appButtonState) {
        renderer.scene.toggleUi();
      }

      touchpadDown = controller.clickButtonState;
      appButtonDown = controller.appButtonState;
    }
  }
}
