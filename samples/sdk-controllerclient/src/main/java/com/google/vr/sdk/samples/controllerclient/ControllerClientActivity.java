/*
 * Copyright 2017 Google Inc. All Rights Reserved.

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

package com.google.vr.sdk.samples.controllerclient;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.widget.TextView;
import com.google.vr.sdk.base.AndroidCompat;
import com.google.vr.sdk.controller.Controller;
import com.google.vr.sdk.controller.Controller.ConnectionStates;
import com.google.vr.sdk.controller.ControllerManager;
import com.google.vr.sdk.controller.ControllerManager.ApiStatus;

/**
 * Minimal example demonstrating how to receive and process Daydream controller input. It connects
 * to a Daydream Controller and displays a simple graphical and textual representation of the
 * controller's sensors. This example only works with Android N and Daydream-ready phones.
 */
public class ControllerClientActivity extends Activity {

  private static final String TAG = ControllerClientActivity.class.getSimpleName();

  // These two objects are the primary APIs for interacting with the Daydream controller.
  private ControllerManager controllerManager;
  private Controller controller;

  // These TextViews display controller events.
  private TextView apiStatusView;
  private TextView controllerStateView;
  private TextView controllerOrientationText;
  private TextView controllerTouchpadView;
  private TextView controllerButtonView;
  private TextView controllerBatteryView;

  // This is a 3D representation of the controller's pose. See its comments for more information.
  private OrientationView controllerOrientationView;

  // The various events we need to handle happen on arbitrary threads. They need to be reposted to
  // the UI thread in order to manipulate the TextViews. This is only required if your app needs to
  // perform actions on the UI thread in response to controller events.
  private Handler uiHandler = new Handler();

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // General view initialization.
    setContentView(R.layout.main_layout);
    apiStatusView = (TextView) findViewById(R.id.api_status_view);
    controllerStateView = (TextView) findViewById(R.id.controller_state_view);
    controllerTouchpadView = (TextView) findViewById(R.id.controller_touchpad_view);
    controllerButtonView = (TextView) findViewById(R.id.controller_button_view);
    controllerOrientationText = (TextView) findViewById(R.id.controller_orientation_text);
    controllerTouchpadView = (TextView) findViewById(R.id.controller_touchpad_view);
    controllerButtonView = (TextView) findViewById(R.id.controller_button_view);
    controllerBatteryView = (TextView) findViewById(R.id.controller_battery_view);

    // Start the ControllerManager and acquire a Controller object which represents a single
    // physical controller. Bind our listener to the ControllerManager and Controller.
    EventListener listener = new EventListener();
    controllerManager = new ControllerManager(this, listener);
    apiStatusView.setText("Binding to VR Service");
    controller = controllerManager.getController();
    controller.setEventListener(listener);

    // Bind the OrientationView to our acquired controller.
    controllerOrientationView = (OrientationView) findViewById(R.id.controller_orientation_view);
    controllerOrientationView.setController(controller);

    // This configuration won't be required for normal GVR apps. However, since this sample doesn't
    // use GvrView, it needs pretend to be a VR app in order to receive controller events. The
    // Activity.setVrModeEnabled is only enabled on in N, so this is an GVR-internal utility method
    // to configure the app via reflection.
    //
    // If this sample is compiled with the N SDK, Activity.setVrModeEnabled can be called directly.
    AndroidCompat.setVrModeEnabled(this, true);
  }

  @Override
  protected void onStart() {
    super.onStart();
    controllerManager.start();
    controllerOrientationView.startTrackingOrientation();
  }

  @Override
  protected void onStop() {
    controllerManager.stop();
    controllerOrientationView.stopTrackingOrientation();
    super.onStop();
  }

  // We receive all events from the Controller through this listener. In this example, our
  // listener handles both ControllerManager.EventListener and Controller.EventListener events.
  // This class is also a Runnable since the events will be reposted to the UI thread.
  private class EventListener extends Controller.EventListener
      implements ControllerManager.EventListener, Runnable {

    // The status of the overall controller API. This is primarily used for error handling since
    // it rarely changes.
    private String apiStatus;

    // The state of a specific Controller connection.
    private int controllerState = ConnectionStates.DISCONNECTED;

    @Override
    public void onApiStatusChanged(int state) {
      apiStatus = ApiStatus.toString(state);
      uiHandler.post(this);
    }

    @Override
    public void onConnectionStateChanged(int state) {
      controllerState = state;
      uiHandler.post(this);
    }

    @Override
    public void onRecentered() {
      // In a real GVR application, this would have implicitly called recenterHeadTracker().
      // Most apps don't care about this, but apps that want to implement custom behavior when a
      // recentering occurs should use this callback.
      controllerOrientationView.resetYaw();
    }

    @Override
    public void onUpdate() {
      uiHandler.post(this);
    }

    // Update the various TextViews in the UI thread.
    @Override
    public void run() {
      apiStatusView.setText(apiStatus);
      controllerStateView.setText(ConnectionStates.toString(controllerState));
      controller.update();

      float[] angles = new float[3];
      controller.orientation.toYawPitchRollDegrees(angles);
      controllerOrientationText.setText(String.format(
          "%s\n%s\n[%4.0f\u00b0 y %4.0f\u00b0 p %4.0f\u00b0 r]",
          controller.orientation,
          controller.orientation.toAxisAngleString(),
          angles[0], angles[1], angles[2]));

      if (controller.isTouching) {
        controllerTouchpadView.setText(
            String.format("[%4.2f, %4.2f]", controller.touch.x, controller.touch.y));
      } else {
        controllerTouchpadView.setText("[ NO TOUCH ]");
      }

      controllerButtonView.setText(String.format("[%s][%s][%s][%s][%s]",
          controller.appButtonState ? "A" : " ",
          controller.homeButtonState ? "H" : " ",
          controller.clickButtonState ? "T" : " ",
          controller.volumeUpButtonState ? "+" : " ",
          controller.volumeDownButtonState ? "-" : " "));

      controllerBatteryView.setText(String.format("[level: %s][charging: %s]",
          Controller.BatteryLevels.toString(controller.batteryLevelBucket),
          controller.isCharging));
    }
  }
}
