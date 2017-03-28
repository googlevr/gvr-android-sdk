// Copyright 2016 Google Inc. All Rights Reserved.
package com.google.vr.sdk.samples.videoplayer;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.view.Surface;
import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.DefaultLoadControl;
import com.google.android.exoplayer2.ExoPlayerFactory;
import com.google.android.exoplayer2.SimpleExoPlayer;
import com.google.android.exoplayer2.drm.DefaultDrmSessionManager;
import com.google.android.exoplayer2.drm.DrmSessionManager;
import com.google.android.exoplayer2.drm.FrameworkMediaCrypto;
import com.google.android.exoplayer2.drm.FrameworkMediaDrm;
import com.google.android.exoplayer2.drm.HttpMediaDrmCallback;
import com.google.android.exoplayer2.drm.UnsupportedDrmException;
import com.google.android.exoplayer2.extractor.DefaultExtractorsFactory;
import com.google.android.exoplayer2.source.ExtractorMediaSource;
import com.google.android.exoplayer2.source.MediaSource;
import com.google.android.exoplayer2.source.dash.DashMediaSource;
import com.google.android.exoplayer2.source.dash.DefaultDashChunkSource;
import com.google.android.exoplayer2.source.hls.HlsMediaSource;
import com.google.android.exoplayer2.source.smoothstreaming.DefaultSsChunkSource;
import com.google.android.exoplayer2.source.smoothstreaming.SsMediaSource;
import com.google.android.exoplayer2.trackselection.AdaptiveVideoTrackSelection;
import com.google.android.exoplayer2.trackselection.DefaultTrackSelector;
import com.google.android.exoplayer2.trackselection.TrackSelection;
import com.google.android.exoplayer2.upstream.DataSource;
import com.google.android.exoplayer2.upstream.DefaultBandwidthMeter;
import com.google.android.exoplayer2.upstream.DefaultDataSourceFactory;
import com.google.android.exoplayer2.upstream.DefaultHttpDataSourceFactory;
import com.google.android.exoplayer2.upstream.HttpDataSource;
import com.google.android.exoplayer2.util.Util;
import java.util.Map;
import java.util.UUID;

/**
 * Wrapper for ExoPlayer2 functionality. Handles all the necessary setup for video playback and
 * provides access to simple video playback controls. Plays unprotected videos from a specified Uri,
 * and plays DRM video under the Widevine test license.
 */
public class VideoExoPlayer2 {

  private static final String TAG = VideoExoPlayer2.class.getSimpleName();
  private static final DefaultBandwidthMeter BANDWIDTH_METER = new DefaultBandwidthMeter();

  private final Context context;
  private final String userAgent;
  private final DataSource.Factory mediaDataSourceFactory;
  private SimpleExoPlayer player;

  public VideoExoPlayer2(Context context) {
    this.context = context;
    userAgent = Util.getUserAgent(context, "VideoExoPlayer");
    mediaDataSourceFactory = buildDataSourceFactory(true);
  }

  public void play() {
    if (player != null) {
      player.setPlayWhenReady(true);
    }
  }

  public void pause() {
    if (player != null) {
      player.setPlayWhenReady(false);
    }
  }

  public void togglePause() {
    if (isPaused()) {
      play();
    } else {
      pause();
    }
  }

  public boolean isPaused() {
    return !player.getPlayWhenReady();
  }

  public SimpleExoPlayer getPlayer() {
    return player;
  }
  
  /**
   * Sets the Surface for the video player to decode frames into. When the Surface is set, the video
   * player will begin to autoplay.
   *
   * @param surface The Surface to consume video frames.
   */
  public void setSurface(Surface surface) {
    if (player != null) {
      player.setVideoSurface(surface);
    } else {
      Log.e(TAG, "Error: video player has not been initialized. Cannot set Surface.");
    }
  }

  /**
   * Inialializes the video player. The player can playback a video at the specified Uri. DRM videos
   * hosted by the Widevine test license can be played
   *
   * <p>Note: this should be called in the host activity's onStart() method.
   *
   * @param uri The uri of the source video to play.
   * @param optionalDrmVideoId The ID of the video under the Widevine test license.
   */
  public void initPlayer(Uri uri, String optionalDrmVideoId) throws UnsupportedDrmException {
    DrmSessionManager<FrameworkMediaCrypto> drmSessionManager = null;
    if (optionalDrmVideoId != null) {
      try {
        drmSessionManager =
            buildDrmSessionManager(
                C.WIDEVINE_UUID,
                getWidevineTestLicenseUrl(optionalDrmVideoId),
                null /* keyRequestProperties */);
      } catch (UnsupportedDrmException e) {
        String errorString =
            e.reason == UnsupportedDrmException.REASON_UNSUPPORTED_SCHEME
                ? "UnsupportedDrmException"
                : "UnknownDrmException";
        Log.e(TAG, "Error: " + errorString, e);
        throw e;
      }
    }

    TrackSelection.Factory videoTrackSelectionFactory =
        new AdaptiveVideoTrackSelection.Factory(BANDWIDTH_METER);
    DefaultTrackSelector trackSelector = new DefaultTrackSelector(videoTrackSelectionFactory);
    player =
        ExoPlayerFactory.newSimpleInstance(
            context,
            trackSelector,
            new DefaultLoadControl(),
            drmSessionManager,
            SimpleExoPlayer.EXTENSION_RENDERER_MODE_OFF);

    // Auto play the video.
    player.setPlayWhenReady(true);

    MediaSource mediaSource = buildMediaSource(uri);
    // Prepare the player with the source.
    player.prepare(mediaSource);
  }

  /**
   * Releases the video player. Once the player is released, the client must call initPlayer()
   * before the video player can be used again.
   *
   * <p>Note: this should be called in the host activity's onStop().
   */
  public void releasePlayer() {
    if (player != null) {
      player.release();
      player = null;
    }
  }

  private static String getWidevineTestLicenseUrl(String id) {
    return "https://proxy.uat.widevine.com/proxy?video_id=" + id + "&provider=widevine_test";
  }

  /**
   * Returns a new DataSource factory.
   *
   * @param useBandwidthMeter Whether to set {@link #BANDWIDTH_METER} as a listener to the new
   *     DataSource factory.
   * @return A new DataSource factory.
   */
  private DataSource.Factory buildDataSourceFactory(boolean useBandwidthMeter) {
    return buildDataSourceFactory(useBandwidthMeter ? BANDWIDTH_METER : null);
  }

  /**
   * Returns a new HttpDataSource factory.
   *
   * @param useBandwidthMeter Whether to set {@link #BANDWIDTH_METER} as a listener to the new
   *     DataSource factory.
   * @return A new HttpDataSource factory.
   */
  private HttpDataSource.Factory buildHttpDataSourceFactory(boolean useBandwidthMeter) {
    return buildHttpDataSourceFactory(useBandwidthMeter ? BANDWIDTH_METER : null);
  }

  private DrmSessionManager<FrameworkMediaCrypto> buildDrmSessionManager(
      UUID uuid, String licenseUrl, Map<String, String> keyRequestProperties)
      throws UnsupportedDrmException {
    if (Util.SDK_INT < 18) {
      return null;
    }
    HttpMediaDrmCallback drmCallback =
        new HttpMediaDrmCallback(
            licenseUrl, buildHttpDataSourceFactory(false), keyRequestProperties);
    return new DefaultDrmSessionManager<>(
        uuid, FrameworkMediaDrm.newInstance(uuid), drmCallback, null, null, null);
  }

  private MediaSource buildMediaSource(Uri uri) {
    int type = Util.inferContentType(uri.getLastPathSegment());
    switch (type) {
      case C.TYPE_SS:
        return new SsMediaSource(
            uri,
            buildDataSourceFactory(false),
            new DefaultSsChunkSource.Factory(mediaDataSourceFactory),
            null,
            null);
      case C.TYPE_DASH:
        return new DashMediaSource(
            uri,
            buildDataSourceFactory(false),
            new DefaultDashChunkSource.Factory(mediaDataSourceFactory),
            null,
            null);
      case C.TYPE_HLS:
        return new HlsMediaSource(uri, mediaDataSourceFactory, null, null);
      case C.TYPE_OTHER:
        return new ExtractorMediaSource(
            uri, mediaDataSourceFactory, new DefaultExtractorsFactory(), null, null);
      default:
        {
          throw new IllegalStateException("Unsupported type: " + type);
        }
    }
  }

  private DataSource.Factory buildDataSourceFactory(DefaultBandwidthMeter bandwidthMeter) {
    return new DefaultDataSourceFactory(
        context, bandwidthMeter, buildHttpDataSourceFactory(bandwidthMeter));
  }

  public HttpDataSource.Factory buildHttpDataSourceFactory(DefaultBandwidthMeter bandwidthMeter) {
    return new DefaultHttpDataSourceFactory(userAgent, bandwidthMeter);
  }
}
