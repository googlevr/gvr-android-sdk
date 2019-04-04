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
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import java.util.Locale;

/**
 * Stores settings which are settable through intent extras and persisted in a
 * {@link SharedPreferences} file.
 */
public class Settings {
  private static final String TAG = "Settings";

  private static final String LOCAL_PREFERENCE_FILE = "videoplayerPrefs";
  public static final String USE_DRM_VIDEO_SAMPLE = "use_drm_video_sample";
  public static final String SHOW_FRAME_RATE_BAR = "show_frame_rate_bar";
  public static final String VIDEO_LENGTH_SECONDS = "video_length_seconds";

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
        .apply();
  }

  public void dump() {
    String settings =
        String.format(
            Locale.US,
            "Use DRM video [%b], Show framerate bar [%b], Playback duration (seconds) [%d]",
            useDrmVideoSample,
            showFrameRateBar,
            videoLengthSeconds);
    Log.d(TAG, "Video settings: " + settings);
  }
}
