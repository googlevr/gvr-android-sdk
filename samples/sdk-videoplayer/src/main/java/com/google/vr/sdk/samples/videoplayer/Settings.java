// Copyright 2016 Google Inc. All Rights Reserved.
package com.google.vr.sdk.samples.videoplayer;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

/**
 * Stores settings which are settable through intent extras and persisted in a
 * {@link SharedPreferences} file.
 */
public class Settings {
  private static final String TAG = "Settings";

  private static final String LOCAL_PREFERENCE_FILE = "videoplayerPrefs";
  private static final String USE_DRM_VIDEO_SAMPLE = "use_drm_video_sample";
  private static final String SHOW_FRAME_RATE_BAR = "show_frame_rate_bar";
  private static final String VIDEO_LENGTH_SECONDS = "video_length_seconds";

  private final Activity activity;
  // When true, a DRM-protected sample is played back in a protected compositor GL context. When
  // false, a cleartext sample is played in a normal context.
  public boolean useDrmVideoSample = true;
  // Controls whether a colored bar showing the average video frame rate over the last few seconds
  // is shown under the video.
  public boolean showFrameRateBar = false;
  // When greater than zero, indicates how long the video should run before stopping. This is mainly
  // useful to facilitate faster tests.
  public int videoLengthSeconds = -1;

  public Settings(Activity activity, Bundle intentParams) {
    this.activity = activity;
    // Use extra intent arguments to dynamically disable/enable DRM, the frame rate bar, etc.
    // The intents are "sticky", i.e., they affect all future launches of the app until a different
    // intent is passed or the package data is cleared.
    readPreferences();
    if (intentParams != null) {
      if (intentParams.containsKey(USE_DRM_VIDEO_SAMPLE)) {
        useDrmVideoSample = intentParams.getBoolean(USE_DRM_VIDEO_SAMPLE, true);
      }
      if (intentParams.containsKey(SHOW_FRAME_RATE_BAR)) {
        showFrameRateBar = intentParams.getBoolean(SHOW_FRAME_RATE_BAR, false);
      }
      if (intentParams.containsKey(VIDEO_LENGTH_SECONDS)) {
        videoLengthSeconds = intentParams.getInt(VIDEO_LENGTH_SECONDS, -1);
      }
    }
    storePreferences();
    dump();
  }

  private void readPreferences() {
    SharedPreferences pref =
        activity.getSharedPreferences(LOCAL_PREFERENCE_FILE, Context.MODE_PRIVATE);
    useDrmVideoSample = pref.getBoolean(USE_DRM_VIDEO_SAMPLE, true);
    showFrameRateBar = pref.getBoolean(SHOW_FRAME_RATE_BAR, false);
    videoLengthSeconds = pref.getInt(VIDEO_LENGTH_SECONDS, -1);
  }

  private void storePreferences() {
    SharedPreferences.Editor pref =
        activity.getSharedPreferences(LOCAL_PREFERENCE_FILE, Context.MODE_PRIVATE).edit();
    pref.putBoolean(USE_DRM_VIDEO_SAMPLE, useDrmVideoSample)
        .putBoolean(SHOW_FRAME_RATE_BAR, showFrameRateBar)
        .putInt(VIDEO_LENGTH_SECONDS, videoLengthSeconds)
        .commit();
  }

  public void dump() {
    String settings = String.format(
        "Use DRM video [%b], Show framerate bar [%b], Playback duration (seconds) [%d]",
        useDrmVideoSample, showFrameRateBar, videoLengthSeconds);
    Log.d(TAG, "Video settings: " + settings);
  }
}
