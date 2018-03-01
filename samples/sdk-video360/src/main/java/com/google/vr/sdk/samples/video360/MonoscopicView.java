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

import android.content.Context;
import android.content.Intent;
import android.graphics.PointF;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.support.annotation.AnyThread;
import android.support.annotation.BinderThread;
import android.support.annotation.UiThread;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import com.google.vr.sdk.base.Eye.Type;
import com.google.vr.sdk.samples.video360.rendering.SceneRenderer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Renders a GL scene in a non-VR Activity that is affected by phone orientation and touch input.
 *
 * <p>The two input components are the TYPE_GAME_ROTATION_VECTOR Sensor and a TouchListener. The GL
 * renderer combines these two inputs to render a scene with the appropriate camera orientation.
 *
 * <p>The primary complexity in this class is related to the various rotations. It is important to
 * apply the touch and sensor rotations in the correct order or the user's touch manipulations won't
 * match what they expect.
 */
public final class MonoscopicView extends GLSurfaceView {
  // We handle all the sensor orientation detection ourselves.
  private SensorManager sensorManager;
  private Sensor orientationSensor;
  private PhoneOrientationListener phoneOrientationListener;

  private MediaLoader mediaLoader;
  private Renderer renderer;
  private TouchTracker touchTracker;
  private VideoUiView uiView;

  /** Inflates a standard GLSurfaceView. */
  public MonoscopicView(Context context, AttributeSet attributeSet) {
    super(context, attributeSet);
    setPreserveEGLContextOnPause(true);
  }

  /**
   * Finishes initialization. This should be called immediately after the View is inflated.
   *
   * @param uiView the video UI that should be bound to the underlying SceneRenderer
   */
  public void initialize(VideoUiView uiView) {
    this.uiView = uiView;
    mediaLoader = new MediaLoader(getContext());

    // Configure OpenGL.
    renderer = new Renderer(uiView, mediaLoader);
    setEGLContextClientVersion(2);
    setRenderer(renderer);
    setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

    // Configure sensors and touch.
    sensorManager = (SensorManager) getContext().getSystemService(Context.SENSOR_SERVICE);
    // TYPE_GAME_ROTATION_VECTOR is the easiest sensor since it handles all the complex math for
    // fusion. It's used instead of TYPE_ROTATION_VECTOR since the latter uses the mangetometer on
    // devices. When used indoors, the magnetometer can take some time to settle depending on the
    // device and amount of metal in the environment.
    orientationSensor = sensorManager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
    phoneOrientationListener = new PhoneOrientationListener();

    touchTracker = new TouchTracker(renderer);
    setOnTouchListener(touchTracker);
  }

  /** Starts the sensor & video only when this View is active. */
  @Override
  public void onResume() {
    super.onResume();
    // Use the fastest sensor readings.
    sensorManager.registerListener(
        phoneOrientationListener, orientationSensor, SensorManager.SENSOR_DELAY_FASTEST);
    mediaLoader.resume();
  }

  /** Stops the sensors & video when the View is inactive to avoid wasting battery. */
  @Override
  public void onPause() {
    mediaLoader.pause();
    sensorManager.unregisterListener(phoneOrientationListener);
    super.onPause();
  }

  /** Destroys the underlying resources. If this is not called, the MediaLoader may leak. */
  public void destroy() {
    uiView.setMediaPlayer(null);
    mediaLoader.destroy();
  }

  /** Parses the Intent and loads the appropriate media. */
  public void loadMedia(Intent intent) {
    mediaLoader.handleIntent(intent, uiView);
  }

  /** Detects sensor events and saves them as a matrix. */
  private class PhoneOrientationListener implements SensorEventListener {
    private final float[] phoneInWorldSpaceMatrix = new float[16];
    private final float[] remappedPhoneMatrix = new float[16];
    private final float[] angles = new float[3];

    @Override
    @BinderThread
    public void onSensorChanged(SensorEvent event) {
      SensorManager.getRotationMatrixFromVector(phoneInWorldSpaceMatrix, event.values);

      // Extract the phone's roll and pass it on to touchTracker & renderer. Remapping is required
      // since we need the calculated roll of the phone to be independent of the phone's pitch &
      // yaw. Any operation that decomposes rotation to Euler angles needs to be performed
      // carefully.
      SensorManager.remapCoordinateSystem(
          phoneInWorldSpaceMatrix,
          SensorManager.AXIS_X, SensorManager.AXIS_MINUS_Z,
          remappedPhoneMatrix);
      SensorManager.getOrientation(remappedPhoneMatrix, angles);
      float roll = angles[2];
      touchTracker.setRoll(roll);

      // Rotate from Android coordinates to OpenGL coordinates. Android's coordinate system
      // assumes Y points North and Z points to the sky. OpenGL has Y pointing up and Z pointing
      // toward the user.
      Matrix.rotateM(phoneInWorldSpaceMatrix, 0, 90, 1, 0, 0);
      renderer.setDeviceOrientation(phoneInWorldSpaceMatrix, roll);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {}
  }

  /**
   * Basic touch input system.
   *
   * <p>Mixing touch input and gyro input results in a complicated UI so this should be used
   * carefully. This touch system implements a basic (X, Y) -> (yaw, pitch) transform. This works
   * for basic UI but fails in edge cases where the user tries to drag scene up or down. There is no
   * good UX solution for this. The least bad solution is to disable pitch manipulation and only let
   * the user adjust yaw. This example tries to limit the awkwardness by restricting pitch
   * manipulation to +/- 45 degrees.
   *
   * <p>It is also important to get the order of operations correct. To match what users expect,
   * touch interaction manipulates the scene by rotating the world by the yaw offset and tilting the
   * camera by the pitch offset. If the order of operations is incorrect, the sensors & touch
   * rotations will have strange interactions. The roll of the phone is also tracked so that the
   * x & y are correctly mapped to yaw & pitch no matter how the user holds their phone.
   *
   * <p>This class doesn't handle any scrolling inertia but Android's
   * {@link com.google.vr.sdk.widgets.common.TouchTracker.FlingGestureListener} can be used with
   * this code for a nicer UI. An even more advanced UI would reproject the user's touch point into
   * 3D and drag the Mesh as the user moves their finger. However, that requires quaternion
   * interpolation and is beyond the scope of this sample.
   */
  static class TouchTracker implements OnTouchListener {
    // Arbitrary touch speed number. This should be tweaked so the scene smoothly follows the
    // finger or derived from DisplayMetrics.
    static final float PX_PER_DEGREES = 25;
    // Touch input won't change the pitch beyond +/- 45 degrees. This reduces awkward situations
    // where the touch-based pitch and gyro-based pitch interact badly near the poles.
    static final float MAX_PITCH_DEGREES = 45;
    // With every touch event, update the accumulated degrees offset by the new pixel amount.
    private final PointF previousTouchPointPx = new PointF();
    private final PointF accumulatedTouchOffsetDegrees = new PointF();
    // The conversion from touch to yaw & pitch requires compensating for device roll. This is set
    // on the sensor thread and read on the UI thread.
    private volatile float roll;

    private final Renderer renderer;

    public TouchTracker(Renderer renderer) {
      this.renderer = renderer;
    }

    /**
     * Converts ACTION_MOVE events to pitch & yaw events while compensating for device roll.
     *
     * @return true if we handled the event
     */
    @Override
    public boolean onTouch(View v, MotionEvent event) {
      switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:
          // Initialize drag gesture.
          previousTouchPointPx.set(event.getX(), event.getY());
          return true;
        case MotionEvent.ACTION_MOVE:
          // Calculate the touch delta in screen space.
          float touchX = (event.getX() - previousTouchPointPx.x) / PX_PER_DEGREES;
          float touchY = (event.getY() - previousTouchPointPx.y) / PX_PER_DEGREES;
          previousTouchPointPx.set(event.getX(), event.getY());

          float r = roll;  // Copy volatile state.
          float cr = (float) Math.cos(r);
          float sr = (float) Math.sin(r);
          // To convert from screen space to the 3D space, we need to adjust the drag vector based
          // on the roll of the phone. This is standard rotationMatrix(roll) * vector math but has
          // an inverted y-axis due to the screen-space coordinates vs GL coordinates.
          // Handle yaw.
          accumulatedTouchOffsetDegrees.x -= cr * touchX - sr * touchY;
          // Handle pitch and limit it to 45 degrees.
          accumulatedTouchOffsetDegrees.y += sr * touchX + cr * touchY;
          accumulatedTouchOffsetDegrees.y =
              Math.max(-MAX_PITCH_DEGREES,
                  Math.min(MAX_PITCH_DEGREES, accumulatedTouchOffsetDegrees.y));

          renderer.setPitchOffset(accumulatedTouchOffsetDegrees.y);
          renderer.setYawOffset(accumulatedTouchOffsetDegrees.x);
          return true;
        default:
          return false;
      }
    }

    @BinderThread
    public void setRoll(float roll) {
      // We compensate for roll by rotating in the opposite direction.
      this.roll = -roll;
    }
  }

  /**
   * Standard GL Renderer implementation. The notable code is the matrix multiplication in
   * onDrawFrame and updatePitchMatrix.
   */
  static class Renderer implements GLSurfaceView.Renderer {
    private final SceneRenderer scene = SceneRenderer.createFor2D();

    // Arbitrary vertical field of view. Adjust as desired.
    private static final int FIELD_OF_VIEW_DEGREES = 90;
    private static final float Z_NEAR = .1f;
    private static final float Z_FAR = 100;
    private final float[] projectionMatrix = new float[16];

    // There is no model matrix for this scene so viewProjectionMatrix is used for the mvpMatrix.
    private final float[] viewProjectionMatrix = new float[16];

    // Device orientation is derived from sensor data. This is accessed in the sensor's thread and
    // the GL thread.
    private final float[] deviceOrientationMatrix = new float[16];

    // Optional pitch and yaw rotations are applied to the sensor orientation. These are accessed on
    // the UI, sensor and GL Threads.
    private final float[] touchPitchMatrix = new float[16];
    private final float[] touchYawMatrix = new float[16];
    private float touchPitch;
    private float deviceRoll;

    // viewMatrix = touchPitch * deviceOrientation * touchYaw.
    private final float[] viewMatrix = new float[16];
    private final float[] tempMatrix = new float[16];

    private final VideoUiView uiView;
    private final MediaLoader mediaLoader;

    public Renderer(VideoUiView uiView, MediaLoader mediaLoader) {
      Matrix.setIdentityM(deviceOrientationMatrix, 0);
      Matrix.setIdentityM(touchPitchMatrix, 0);
      Matrix.setIdentityM(touchYawMatrix, 0);
      this.uiView = uiView;
      this.mediaLoader = mediaLoader;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
      scene.glInit();
      if (uiView != null) {
        scene.setVideoFrameListener(uiView.getFrameListener());
      }
      mediaLoader.onGlSceneReady(scene);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
      GLES20.glViewport(0, 0, width, height);
      Matrix.perspectiveM(
          projectionMatrix, 0, FIELD_OF_VIEW_DEGREES, (float) width / height, Z_NEAR, Z_FAR);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
      // Combine touch & sensor data.
      // Orientation = pitch * sensor * yaw since that is closest to what most users expect the
      // behavior to be.
      synchronized (this) {
        Matrix.multiplyMM(tempMatrix, 0, deviceOrientationMatrix, 0, touchYawMatrix, 0);
        Matrix.multiplyMM(viewMatrix, 0, touchPitchMatrix, 0, tempMatrix, 0);
      }

      Matrix.multiplyMM(viewProjectionMatrix, 0, projectionMatrix, 0, viewMatrix, 0);
      scene.glDrawFrame(viewProjectionMatrix, Type.MONOCULAR);
    }

    /** Adjusts the GL camera's rotation based on device rotation. Runs on the sensor thread. */
    @BinderThread
    public synchronized void setDeviceOrientation(float[] matrix, float deviceRoll) {
      System.arraycopy(matrix, 0, deviceOrientationMatrix, 0, deviceOrientationMatrix.length);
      this.deviceRoll = -deviceRoll;
      updatePitchMatrix();
    }

    /**
     * Updates the pitch matrix after a physical rotation or touch input. The pitch matrix rotation
     * is applied on an axis that is dependent on device rotation so this must be called after
     * either touch or sensor update.
     */
    @AnyThread
    private void updatePitchMatrix() {
      // The camera's pitch needs to be rotated along an axis that is parallel to the real world's
      // horizon. This is the <1, 0, 0> axis after compensating for the device's roll.
      Matrix.setRotateM(touchPitchMatrix, 0,
          -touchPitch, (float) Math.cos(deviceRoll), (float) Math.sin(deviceRoll), 0);
    }

    /** Set the pitch offset matrix. */
    @UiThread
    public synchronized void setPitchOffset(float pitchDegrees) {
      touchPitch = pitchDegrees;
      updatePitchMatrix();
    }

    /** Set the yaw offset matrix. */
    @UiThread
    public synchronized void setYawOffset(float yawDegrees) {
      Matrix.setRotateM(touchYawMatrix, 0, -yawDegrees, 0, 1, 0);
    }
  }
}
