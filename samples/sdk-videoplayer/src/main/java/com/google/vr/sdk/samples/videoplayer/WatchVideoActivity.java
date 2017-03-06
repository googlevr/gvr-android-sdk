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

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import com.google.android.exoplayer.drm.UnsupportedDrmException;
import com.google.android.exoplayer.util.Util;
import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;
import com.google.vr.ndk.base.GvrLayout.ExternalSurfaceListener;

/**
 * Simple activity for video playback using the Asynchronous Reprojection Video Surface API. For a
 * detailed description of the API, see the <a
 * href="https://developers.google.com/vr/android/ndk/gvr-ndk-walkthrough#using_video_viewports">ndk
 * walkthrough</a>. This sample supports DRM and unprotected video playback, configured in {@link
 * Configuration}.
 *
 * <p>The Surface for video output is acquired from the {@link ExternalSurfaceListener} and set on
 * the video player. For each frame, this sample draws a window (alpha 0) in the scene where video
 * should be visible. To trigger the GvrApi to render the video frame, the {@link
 * VideoSceneRenderer} adds a {@link BufferViewport} per eye to describe where video should be
 * drawn.
 */
public class WatchVideoActivity extends Activity implements VideoExoPlayer.Listener {
  private static final String TAG = WatchVideoActivity.class.getSimpleName();

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;
  private VideoSceneRenderer renderer;
  private VideoExoPlayer videoPlayer;
  private boolean hasFirstFrame;

  // Transform a quad that fills the clip box at Z=0 to a 16:9 screen at Z=-4. Note that the matrix
  // is column-major, so the translation is on the last line in this representation.
  private final float[] videoTransform = { 1.6f, 0.0f, 0.0f, 0.0f,
                                           0.0f, 0.9f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 1.0f, 0.0f,
                                           0.0f, 0.0f, -4.f, 1.0f };

  // Runnable to refresh the viewer profile when gvrLayout is resumed.
  // This is done on the GL thread because refreshViewerProfile isn't thread-safe.
  private final Runnable refreshViewerProfileRunnable =
      new Runnable() {
        @Override
        public void run() {
          gvrLayout.getGvrApi().refreshViewerProfile();
        }
      };

  // Runnable to play/pause the video player. Must be run on the UI thread.
  private final Runnable triggerRunnable =
      new Runnable() {
          @Override
          public void run() {
            if (videoPlayer != null) {
              videoPlayer.togglePause();
            }
          }
      };

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

    AndroidCompat.setSustainedPerformanceMode(this, true);
    AndroidCompat.setVrModeEnabled(this, true);

    gvrLayout = new GvrLayout(this);
    surfaceView = new GLSurfaceView(this);
    surfaceView.setEGLContextClientVersion(2);
    surfaceView.setEGLConfigChooser(5, 6, 5, 0, 0, 0);
    gvrLayout.setPresentationView(surfaceView);
    gvrLayout.setKeepScreenOn(true);
    renderer = new VideoSceneRenderer(this, gvrLayout.getGvrApi());

    // Initialize the ExternalSurfaceListener to receive video Surface callbacks.
    hasFirstFrame = false;
    ExternalSurfaceListener videoSurfaceListener =
        new ExternalSurfaceListener() {
          @Override
          public void onSurfaceAvailable(Surface surface) {
            // Set the surface for the video player to output video frames to. Video playback
            // is started when the Surface is set. Note that this callback is *asynchronous* with
            // respect to the Surface becoming available, in which case videoPlayer may be null due
            // to the Activity having been stopped.
            if (videoPlayer != null) {
              videoPlayer.setSurface(surface);
            }
          }

          @Override
          public void onFrameAvailable() {
            // If this is the first frame, and the Activity is still in the foreground, signal to
            // remove the loading splash screen, and draw alpha 0 in the color buffer where the
            // video will be drawn by the GvrApi.
            if (!hasFirstFrame && videoPlayer != null) {
              surfaceView.queueEvent(
                new Runnable() {
                  @Override
                  public void run() {
                    renderer.setHasVideoPlaybackStarted(true);
                  }
                });

              hasFirstFrame = true;
            }
          }
        };

    // Note that the video Surface must be enabled before enabling Async Reprojection.
    // Async Reprojection must be enabled for the app to be able to use the video Surface.
    boolean isSurfaceEnabled =
        gvrLayout.enableAsyncReprojectionVideoSurface(
            videoSurfaceListener,
            new Handler(Looper.getMainLooper()),
            Configuration.SECURE_EGL_CONTEXT /* Whether video playback needs a secure context. */);
    boolean isAsyncReprojectionEnabled = gvrLayout.setAsyncReprojectionEnabled(true);

    if (!isSurfaceEnabled || !isAsyncReprojectionEnabled) {
      // The device does not support this API, video will not play.
      Log.e(
          TAG,
          "UnsupportedException: "
              + (!isAsyncReprojectionEnabled ? "Async Reprojection not supported. " : "")
              + (!isSurfaceEnabled ? "Async Reprojection Video Surface not enabled." : ""));
    } else {
      initVideoPlayer();

      // The default value puts the viewport behind the eye, so it's invisible. Set the transform
      // now to ensure the video is visible when rendering starts.
      renderer.setVideoTransform(videoTransform);
      // The ExternalSurface buffer the GvrApi should reference when drawing the video buffer. This
      // must be called after enabling the Async Reprojection video surface.
      renderer.setVideoSurfaceId(gvrLayout.getAsyncReprojectionVideoSurfaceId());

      // Simulate cardboard trigger to play/pause video playback.
      gvrLayout.enableCardboardTriggerEmulation(triggerRunnable);
      surfaceView.setOnTouchListener(
          new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent event) {
              if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                triggerRunnable.run();
                return true;
              }
              return false;
            }
          });
    }

    // Set the renderer and start the app's GL thread.
    surfaceView.setRenderer(renderer);

    setContentView(gvrLayout);
  }

  private void initVideoPlayer() {
    // L3 content - can be directed to a Surface.
    final String streamUrlL3 =
        "http://www.youtube.com/api/manifest/dash/id/d286538032258a1c/source/youtube?"
            + "as=fmp4_audio_cenc,fmp4_sd_hd_cenc&sparams=ip,ipbits,expire,source,id,as&ip=0.0.0.0"
            + "&ipbits=0&expire=19000000000&signature=477CF7D478BE26C205045D507E9358F85F84C065."
            + "8971631EB657BC33EC2F48A2FF4211956760C3E9&key=ik0";
    final String contentIdL3 = "d286538032258a1c";

    // L1 content - requires secure path.
    final String streamUrlL1 =
        "http://www.youtube.com/api/manifest/dash/id/0894c7c8719b28a0/source/youtube?"
            + "as=fmp4_audio_cenc,fmp4_sd_hd_cenc&sparams=ip,ipbits,expire,source,id,as&ip=0.0.0.0"
            + "&ipbits=0&expire=19000000000&signature=A41D835C7387885A4A820628F57E481E00095931."
            + "9D50DBEEB5E37344647EE11BDA129A7FCDE8B7B9&key=ik0";
    final String contentIdL1 = "0894c7c8719b28a0";

    if (Configuration.REQUIRE_SECURE_PATH) {
      videoPlayer =
          new VideoExoPlayer(
              this,
              streamUrlL1,
              new VideoExoPlayer.WidevineTestMediaDrmCallback(contentIdL1),
              true);
    } else {
      videoPlayer =
          new VideoExoPlayer(
              this,
              streamUrlL3,
              new VideoExoPlayer.WidevineTestMediaDrmCallback(contentIdL3),
              false);
    }
    videoPlayer.addListener(this);
    videoPlayer.init();
  }

  @Override
  protected void onStart() {
    super.onStart();
    if (videoPlayer == null) {
      initVideoPlayer();
    }
    hasFirstFrame = false;
    surfaceView.queueEvent(
        new Runnable() {
          @Override
          public void run() {
            renderer.setHasVideoPlaybackStarted(false);
          }
        });

    // Resume the surfaceView and gvrLayout here. This will start the render thread and trigger a
    // new async reprojection video Surface to become available.
    surfaceView.onResume();
    gvrLayout.onResume();
    // Refresh the viewer profile in case the viewer params were changed.
    surfaceView.queueEvent(refreshViewerProfileRunnable);
  }

  @Override
  protected void onResume() {
    super.onResume();
    if (videoPlayer.isPaused()) {
      videoPlayer.play();
    }
  }

  @Override
  protected void onPause() {
    // Pause video playback. The video Surface may be detached before onStop() is called,
    // but will remain valid if the activity life-cycle returns to onResume(). Pause the
    // player to avoid dropping video frames.
    videoPlayer.pause();
    super.onPause();
  }

  @Override
  protected void onStop() {
    videoPlayer.stop();
    if (videoPlayer != null) {
      videoPlayer.release();
      videoPlayer = null;
    }
    // Pause the gvrLayout and surfaceView here. The video Surface is guaranteed to be detached and
    // not available after gvrLayout.onPause(). We pause from onStop() to avoid needing to wait
    // for an available video Surface following brief onPause()/onResume() events. Wait for the
    // new onSurfaceAvailable() callback with a valid Surface before resuming the video player.
    gvrLayout.onPause();
    surfaceView.onPause();
    super.onStop();
  }

  @Override
  protected void onDestroy() {
    gvrLayout.shutdown();
    if (videoPlayer != null) {
      videoPlayer.release();
      videoPlayer = null;
    }
    super.onDestroy();
  }

  @Override
  public void onError(Exception e) {
    if (e instanceof UnsupportedDrmException) {
      // Special case DRM failures.
      UnsupportedDrmException unsupportedDrmException = (UnsupportedDrmException) e;
      int stringId =
          Util.SDK_INT < 18
              ? R.string.drm_error_not_supported
              : unsupportedDrmException.reason == UnsupportedDrmException.REASON_UNSUPPORTED_SCHEME
                  ? R.string.drm_error_unsupported_scheme
                  : R.string.drm_error_unknown;
      Log.e(TAG, "UnsupportedDrmException " + getResources().getString(stringId));
    } else {
      Log.e(TAG, "UnsupportedDrmException " + getResources().getString(R.string.playback_error));
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

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      setImmersiveSticky();
    }
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
}
