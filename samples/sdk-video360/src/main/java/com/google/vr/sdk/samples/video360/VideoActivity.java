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

import android.Manifest;
import android.Manifest.permission;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import com.google.vr.ndk.base.DaydreamApi;
import com.google.vr.sdk.samples.video360.rendering.Mesh;

/**
 * Basic Activity to hold {@link MonoscopicView} and render a 360 video in 2D.
 *
 * Most of this Activity's code is related to Android & VR permission handling. The real work is in
 * MonoscopicView.
 *
 * The default intent for this Activity will load a 360 placeholder panorama. For more options on
 * how to load other media using a custom Intent, see {@link MediaLoader}.
 */
public class VideoActivity extends Activity {
  private static final String TAG = "VideoActivity";
  private static final int READ_EXTERNAL_STORAGE_PERMISSION_ID = 1;
  private MonoscopicView videoView;

  /**
   * Checks that the appropriate permissions have been granted. Otherwise, the sample will wait
   * for the user to grant the permission.
   *
   * @param savedInstanceState unused in this sample but it could be used to track video position
   */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.video_activity);

    // Configure the MonoscopicView which will render the video and UI.
    videoView = (MonoscopicView) findViewById(R.id.video_view);
    VideoUiView videoUi = (VideoUiView) findViewById(R.id.video_ui_view);
    videoUi.setVrIconClickListener(
        new OnClickListener() {
          @Override
          public void onClick(View v) {
            // Convert the Intent used to launch the 2D Activity into one that can launch the VR
            // Activity. This flow preserves the extras and data in the Intent.
            DaydreamApi api =  DaydreamApi.create(VideoActivity.this);
            if (api != null){
              // Launch the VR Activity with the proper intent.
              Intent intent = DaydreamApi.createVrIntent(
                  new ComponentName(VideoActivity.this, VrVideoActivity.class));
              intent.setData(getIntent().getData());
              intent.putExtra(
                  MediaLoader.MEDIA_FORMAT_KEY,
                  getIntent().getIntExtra(MediaLoader.MEDIA_FORMAT_KEY, Mesh.MEDIA_MONOSCOPIC));
              api.launchInVr(intent);
              api.close();
            } else {
              // Fall back for devices that don't have Google VR Services. This flow should only
              // be used for older Cardboard devices.
              Intent intent =
                  new Intent(getIntent()).setClass(VideoActivity.this, VrVideoActivity.class);
              intent.removeCategory(Intent.CATEGORY_LAUNCHER);
              intent.setFlags(0);  // Clear any flags from the previous intent.
              startActivity(intent);
            }

            // See VrVideoActivity's launch2dActivity() for more info about why this finish() call
            // may be required.
            finish();
          }
        });
    videoView.initialize(videoUi);

    // Boilerplate for checking runtime permissions in Android.
    if (ContextCompat.checkSelfPermission(this, permission.READ_EXTERNAL_STORAGE)
        != PackageManager.PERMISSION_GRANTED) {
      View button = findViewById(R.id.permission_button);
      button.setOnClickListener(
          new OnClickListener() {
            @Override
            public void onClick(View v) {
              ActivityCompat.requestPermissions(
                  VideoActivity.this,
                  new String[] {Manifest.permission.READ_EXTERNAL_STORAGE},
                  READ_EXTERNAL_STORAGE_PERMISSION_ID);
            }
          });
      // The user can click the button to request permission but we will also click on their behalf
      // when the Activity is created.
      button.callOnClick();
    } else {
      // Permission has already been granted.
      initializeActivity();
    }
  }

  /** Handles the user accepting the permission. */
  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
    if (requestCode == READ_EXTERNAL_STORAGE_PERMISSION_ID) {
      if (results.length > 0 && results[0] == PackageManager.PERMISSION_GRANTED) {
        initializeActivity();
      }
    }
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

  /** Initializes the Activity only if the permission has been granted. */
  private void initializeActivity() {
    ViewGroup root = (ViewGroup) findViewById(R.id.activity_root);
    for (int i = 0; i < root.getChildCount(); ++i) {
      root.getChildAt(i).setVisibility(View.VISIBLE);
    }
    findViewById(R.id.permission_button).setVisibility(View.GONE);
    videoView.loadMedia(getIntent());
  }

  @Override
  protected void onResume() {
    super.onResume();
    videoView.onResume();
  }

  @Override
  protected void onPause() {
    // MonoscopicView is a GLSurfaceView so it needs to pause & resume rendering. It's also
    // important to pause MonoscopicView's sensors & the video player.
    videoView.onPause();
    super.onPause();
  }

  @Override
  protected void onDestroy() {
    videoView.destroy();
    super.onDestroy();
  }
}
