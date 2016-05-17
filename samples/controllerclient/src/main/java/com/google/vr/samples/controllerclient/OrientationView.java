package com.google.vr.sdk.samples.controllerclient;

import com.google.vr.sdk.controller.Controller;
import com.google.vr.sdk.controller.Orientation;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.opengl.GLES10;
import android.opengl.GLSurfaceView;
import android.opengl.GLU;
import android.util.AttributeSet;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * View that graphically demonstrates the orientation of the Daydream Controller. It renders an RGB
 * box with the same orientation as the physical controller. Rotating the phone will rotate the GL
 * camera.
 */
public class OrientationView extends GLSurfaceView {

  // Tracks the orientation of the phone. This would be a head pose if this was a VR application.
  // See the {@link Sensor.TYPE_ROTATION_VECTOR} section of {@link SensorEvent.values} for details
  // about the specific sensor that is used. It's important to note that this sensor defines the
  // Z-axis as point up and the Y-axis as pointing toward what the phone believes to be magnetic
  // north. Google VR's coordinate system defines the Y-axis as pointing up and the Z-axis as
  // pointing toward the user. This requires a 90-degree rotation on the X-axis to convert between
  // the two coordinate systems.
  private final SensorManager sensorManager;
  private final Sensor orientationSensor;
  private final PhoneOrientationListener phoneOrientationListener;
  private float[] phoneInWorldSpaceMatrix = new float[16];

  // Tracks the orientation of the physical controller in its properly centered Start Space.
  private Controller controller;
  /** See {@link #resetYaw} */
  private float[] startFromSensorTransformation;
  private float[] controllerInStartSpaceMatrix = new float[16];

  public OrientationView(Context context, AttributeSet attributeSet) {
    super(context, attributeSet);
    setEGLConfigChooser(8, 8, 8, 8, 16, 0);
    setRenderer(new Renderer());

    sensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
    orientationSensor = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
    phoneOrientationListener = new PhoneOrientationListener();
  }

  /**
   * Bind the controller used for rendering.
   */
  public void setController(Controller controller) {
    this.controller = controller;
  }

  /**
   * Start the orientation sensor only when the app is visible.
   */
  public void startTrackingOrientation() {
    sensorManager.registerListener(
        phoneOrientationListener, orientationSensor, SensorManager.SENSOR_DELAY_GAME);
  }

  /**
   * This is similar to {@link com.google.vr.sdk.base.GvrView#resetHeadTracker}.
   */
  public void resetYaw() {
    startFromSensorTransformation = null;
  }

  /**
   * Stop the orientation sensor when the app is dismissed.
   */
  public void stopTrackingOrientation() {
    sensorManager.unregisterListener(phoneOrientationListener);
  }

  private class PhoneOrientationListener implements SensorEventListener {

    @Override
    public void onSensorChanged(SensorEvent event) {
      SensorManager.getRotationMatrixFromVector(phoneInWorldSpaceMatrix, event.values);
      if (startFromSensorTransformation == null) {
        // Android's hardware uses radians, but OpenGL uses degrees. Android uses
        // [yaw, pitch, roll] for the order of elements in the orientation array.
        float[] orientationRadians =
            SensorManager.getOrientation(phoneInWorldSpaceMatrix, new float[3]);
        startFromSensorTransformation = new float[3];
        for (int i = 0; i < 3; ++i) {
          startFromSensorTransformation[i] = (float) Math.toDegrees(orientationRadians[i]);
        }
      }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }
  }

  private class Renderer implements GLSurfaceView.Renderer {

    // Size of the 3D space where the box is rendered.
    private static final float VIEW_SIZE = 6;

    // The box is a cube with XYZ lines colored RGB, respectively. Lines get brighter in the
    // positive direction. It has a green arrow to represent the trackpad at the front and top of
    // the controller
    private FloatBuffer boxVertices;
    private FloatBuffer boxColors;

    // Initialize geometry.
    @Override
    public final void onSurfaceCreated(GL10 gl, EGLConfig config) {
      // 16 lines * 2 points * XYZ
      float[] boxVertexData = {
          // X-aligned lines of length 4
          -2, -1, -3,  2, -1, -3,
          -2, -1,  3,  2, -1,  3,
          -2,  1, -3,  2,  1, -3,
          -2,  1,  3,  2,  1,  3,
          // Y-aligned lines of length 2
          -2, -1, -3, -2,  1, -3,
          -2, -1,  3, -2,  1,  3,
           2, -1, -3,  2,  1, -3,
           2, -1,  3,  2,  1,  3,
          // Z-aligned lines of length 6
          -2, -1, -3, -2, -1,  3,
          -2,  1, -3, -2,  1,  3,
           2, -1, -3,  2, -1,  3,
           2,  1, -3,  2,  1,  3,
          // Trackpad diamond
          -1,  1, -1,  0,  1,  0,
           0,  1,  0,  1,  1, -1,
           1,  1, -1,  0,  1, -3,
           0,  1, -3, -1,  1, -1,
      };

      ByteBuffer buffer = ByteBuffer.allocateDirect(boxVertexData.length * 4);
      buffer.order(ByteOrder.nativeOrder());
      boxVertices = buffer.asFloatBuffer();
      boxVertices.put(boxVertexData);
      boxVertices.position(0);

      // The XYZ lines are RGB in the positive direction and black in the negative direction.
      // 16 lines * 2 points * RGBA
      float[] boxColorData = {
          // X-aligned lines
          0, 0, 0, 1, 1, 0, 0, 1,
          0, 0, 0, 1, 1, 0, 0, 1,
          0, 0, 0, 1, 1, 0, 0, 1,
          0, 0, 0, 1, 1, 0, 0, 1,
          // Y-aligned lines
          0, 0, 0, 1, 0, 1, 0, 1,
          0, 0, 0, 1, 0, 1, 0, 1,
          0, 0, 0, 1, 0, 1, 0, 1,
          0, 0, 0, 1, 0, 1, 0, 1,
          // Z-aligned lines
          0, 0, 0, 1, 0, 0, 1, 1,
          0, 0, 0, 1, 0, 0, 1, 1,
          0, 0, 0, 1, 0, 0, 1, 1,
          0, 0, 0, 1, 0, 0, 1, 1,
          // Trackpad
          0, 1, 0, 1, 0, 1, 0, 1,
          0, 1, 0, 1, 0, 1, 0, 1,
          0, 1, 0, 1, 0, 1, 0, 1,
          0, 1, 0, 1, 0, 1, 0, 1,
      };
      buffer = ByteBuffer.allocateDirect(boxColorData.length * 4);
      buffer.order(ByteOrder.nativeOrder());
      boxColors = buffer.asFloatBuffer();
      boxColors.put(boxColorData);
      boxColors.position(0);
    }

    // Set up GL environment.
    @Override
    public final void onSurfaceChanged(GL10 gl, int width, int height) {
      // Camera.
      gl.glViewport(0, 0, width, height);
      gl.glMatrixMode(GL10.GL_PROJECTION);
      gl.glLoadIdentity();
      GLU.gluPerspective(gl, 90, (float) (width) / height, 1, 2 * VIEW_SIZE);
      gl.glMatrixMode(GL10.GL_MODELVIEW);
      gl.glLoadIdentity();

      // GL configuration.
      gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
      gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
      gl.glEnable(GL10.GL_DEPTH_TEST);

      // Styles.
      gl.glClearColor(1, 1, 1, 1);
      gl.glLineWidth(10);
    }

    /**
     * Converts an {@link com.google.vr.sdk.controller.Orientation} to a column-major rotation
     * matrix.
     */
    private void orientationToRotationMatrix(float[] matrix, Orientation o) {
      // Based on https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion.
      matrix[0]  = 1 - 2 * o.y * o.y - 2 * o.z * o.z;
      matrix[1]  =     2 * o.x * o.y + 2 * o.z * o.w;
      matrix[2]  =     2 * o.x * o.z - 2 * o.y * o.w;
      matrix[3]  = 0;

      matrix[4]  =     2 * o.x * o.y - 2 * o.z * o.w;
      matrix[5]  = 1 - 2 * o.x * o.x - 2 * o.z * o.z;
      matrix[6]  =     2 * o.y * o.z + 2 * o.x * o.w;
      matrix[7]  = 0;

      matrix[8]  =     2 * o.x * o.z + 2 * o.y * o.w;
      matrix[9]  =     2 * o.y * o.z - 2 * o.x * o.w;
      matrix[10] = 1 - 2 * o.x * o.x - 2 * o.y * o.y;
      matrix[11] = 0;

      matrix[12] = 0;
      matrix[13] = 0;
      matrix[14] = 0;
      matrix[15] = 1;
    }

    @Override
    public final void onDrawFrame(GL10 gl) {
      gl.glClear(GLES10.GL_COLOR_BUFFER_BIT | GLES10.GL_DEPTH_BUFFER_BIT);
      gl.glPushMatrix();

      // Convert world space to head space
      gl.glTranslatef(0, 0, -VIEW_SIZE);
      gl.glMultMatrixf(phoneInWorldSpaceMatrix, 0);

      // Phone's Z faces up. We need it to face toward the user.
      gl.glRotatef(90, 1, 0, 0);

      if (startFromSensorTransformation != null) {
        // Compensate for the yaw by rotating in the other direction.
        gl.glRotatef(-startFromSensorTransformation[0], 0, 1, 0);
      }  // Else we're in a transient state between a resetYaw call and an onSensorChanged call.

      // Convert object space to world space
      if (controller != null) {
        controller.update();
        orientationToRotationMatrix(controllerInStartSpaceMatrix, controller.orientation);
      }
      gl.glMultMatrixf(controllerInStartSpaceMatrix, 0);

      // Render.
      gl.glVertexPointer(3, GL10.GL_FLOAT, 0, boxVertices);
      gl.glColorPointer(4, GL10.GL_FLOAT, 0, boxColors);
      gl.glDrawArrays(GL10.GL_LINES, 0, 32); // 16 lines * 2 points

      gl.glPopMatrix();
    }
  }
}
