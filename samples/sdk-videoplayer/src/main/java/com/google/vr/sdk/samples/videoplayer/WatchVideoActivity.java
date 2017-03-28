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
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import com.google.android.exoplayer2.drm.UnsupportedDrmException;
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
public class WatchVideoActivity extends Activity {
  private static final String TAG = WatchVideoActivity.class.getSimpleName();

  private GvrLayout gvrLayout;
  private GLSurfaceView surfaceView;
  private VideoSceneRenderer renderer;
  private VideoExoPlayer2 videoPlayer;
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
            /* Whether video playback should use a protected reprojection pipeline. */
            Configuration.USE_PROTECTED_PIPELINE);
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
    videoPlayer = new VideoExoPlayer2(getApplication());
    Uri streamUri;
    String drmVideoId = null;

    if (Configuration.USE_DRM_VIDEO_SAMPLE) {
      // Protected video, requires a secure path for playback.
      streamUri = Uri.parse("https://storage.googleapis.com/wvmedia/cenc/h264/tears/tears.mpd");
      drmVideoId = "0894c7c8719b28a0";
    } else {
      // Unprotected video, does not require a secure path for playback.
      streamUri = Uri.parse("https://storage.googleapis.com/wvmedia/clear/h264/tears/tears.mpd");
    }

    try {
      videoPlayer.initPlayer(streamUri, drmVideoId);
    } catch (UnsupportedDrmException e) {
      Log.e(TAG, "Error initializing video player", e);
    }
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
    if (videoPlayer != null) {
      videoPlayer.releasePlayer();
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
    super.onDestroy();
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

  protected VideoExoPlayer2 getVideoExoPlayer() {
    return videoPlayer;
  }

  /** @return {@code true} if the first video frame has played **/
  protected boolean hasFirstFrame() {
    return hasFirstFrame;
  }
}
