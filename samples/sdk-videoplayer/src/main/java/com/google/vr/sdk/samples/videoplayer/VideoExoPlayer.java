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

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaCodec;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import com.google.android.exoplayer.DefaultLoadControl;
import com.google.android.exoplayer.ExoPlaybackException;
import com.google.android.exoplayer.ExoPlayer;
import com.google.android.exoplayer.LoadControl;
import com.google.android.exoplayer.MediaCodecAudioTrackRenderer;
import com.google.android.exoplayer.MediaCodecSelector;
import com.google.android.exoplayer.MediaCodecVideoTrackRenderer;
import com.google.android.exoplayer.TrackRenderer;
import com.google.android.exoplayer.chunk.ChunkSampleSource;
import com.google.android.exoplayer.chunk.ChunkSource;
import com.google.android.exoplayer.chunk.FormatEvaluator;
import com.google.android.exoplayer.dash.DashChunkSource;
import com.google.android.exoplayer.dash.DefaultDashTrackSelector;
import com.google.android.exoplayer.dash.mpd.AdaptationSet;
import com.google.android.exoplayer.dash.mpd.MediaPresentationDescription;
import com.google.android.exoplayer.dash.mpd.MediaPresentationDescriptionParser;
import com.google.android.exoplayer.dash.mpd.Period;
import com.google.android.exoplayer.dash.mpd.UtcTimingElement;
import com.google.android.exoplayer.dash.mpd.UtcTimingElementResolver;
import com.google.android.exoplayer.drm.ExoMediaDrm.KeyRequest;
import com.google.android.exoplayer.drm.ExoMediaDrm.ProvisionRequest;
import com.google.android.exoplayer.drm.MediaDrmCallback;
import com.google.android.exoplayer.drm.StreamingDrmSessionManager;
import com.google.android.exoplayer.drm.UnsupportedDrmException;
import com.google.android.exoplayer.upstream.Allocator;
import com.google.android.exoplayer.upstream.DataSource;
import com.google.android.exoplayer.upstream.DefaultAllocator;
import com.google.android.exoplayer.upstream.DefaultBandwidthMeter;
import com.google.android.exoplayer.upstream.DefaultUriDataSource;
import com.google.android.exoplayer.upstream.UriDataSource;
import com.google.android.exoplayer.util.ManifestFetcher;
import com.google.android.exoplayer.util.Util;
import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Simple playback controller for video streams that wraps ExoPlayer.
 */
/* package */ class VideoExoPlayer {

  private static final String TAG = "VideoExoPlayer";

  public static final int RENDERER_COUNT = 2;
  public static final int TYPE_VIDEO = 0;
  public static final int TYPE_AUDIO = 1;

  private static final int BUFFER_SEGMENT_SIZE = 64 * 1024;
  private static final int VIDEO_BUFFER_SEGMENTS = 200;
  private static final int AUDIO_BUFFER_SEGMENTS = 54;
  private static final int LIVE_EDGE_LATENCY_MS = 30000;

  private static final int SECURITY_LEVEL_UNKNOWN = -1;
  private static final int SECURITY_LEVEL_1 = 1;
  private static final int SECURITY_LEVEL_3 = 3;

  private final AudioManager audioManager;

  private final Handler mainHandler;
  private TrackRenderer audioRenderer;
  private TrackRenderer videoRenderer;
  private ExoPlayer player;
  private Surface surface;
  private AsyncRendererBuilder currentAsyncBuilder;
  private final CopyOnWriteArrayList<Listener> listeners;

  private boolean isSurfaceCreated;
  private boolean isPaused = false;

  /**
   * A listener for error reporting.
   */
  public interface Listener {
    void onError(Exception e);
  }

  /**
   * Creates a VideoExoPlayer.
   *
   * @param context The Application context.
   * @param streamUrl The URL to play.
   * @param context A function to call for DRM-related messages.
   * @param requiresSecurePlayback Whether secure playback is necessary.
   */
  public VideoExoPlayer(
      Context context,
      String streamUrl,
      MediaDrmCallback drmCallback,
      boolean requiresSecurePlayback) {
    audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

    Allocator allocator = new DefaultAllocator(BUFFER_SEGMENT_SIZE);
    String userAgent = Util.getUserAgent(context, "VRPlayMovies");

    player = ExoPlayer.Factory.newInstance(RENDERER_COUNT, 1000, 5000);
    mainHandler = new Handler();
    listeners = new CopyOnWriteArrayList<>();

    currentAsyncBuilder =
        new AsyncRendererBuilder(
            context, userAgent, streamUrl, requiresSecurePlayback, drmCallback, this);
  }

  /**
   * Adds a listener for crtical playback events.
   *
   * @param listener The Listener to add.
   */
  public void addListener(Listener listener) {
    listeners.add(listener);
  }

  /**
   * Removes a listener.
   *
   * @param listener The Listener to remove.
   */
  public void removeListener(Listener listener) {
    listeners.remove(listener);
  }

  /**
   * Initializes the player.
   */
  public void init() {
    currentAsyncBuilder.init();
  }

  /**
   * Releases the player.  Call init() to initialize it again.
   */
  public void release() {
    releasePlayer();
  }

  /**
   * Sets the Surface that will contain video frames. This also starts playback of the player.
   *
   * @param surface The Surface that should receive video.
   */
  public void setSurface(Surface surface) {
    isSurfaceCreated = true;
    this.surface = surface;
    if (videoRenderer != null) {
      mainHandler.post(
          new Runnable() {
            @Override
            public void run() {
              beginPlayback();
            }
          });
    }
  }

  /**
   * Stops the player and releases it.
   */
  public void stop() {
    // Release the player instead of stopping so that an async prepare gets stopped.
    releasePlayer();
    audioManager.abandonAudioFocus(null);
  }

  /**
   * Gets whather playback is paused.
   *
   * @return Whether playback is paused.
   */
  public boolean isPaused() {
    return isPaused;
  }

  /**
   * Pauses or restarts the player.
   */
  public void togglePause() {
    Log.d(TAG, "togglePause()");

    if (player != null) {
      if (isPaused) {
        player.setPlayWhenReady(true);
      } else {
        player.setPlayWhenReady(false);
      }
      isPaused = !isPaused;
    }
  }

  /**
   * Starts playback (unpausing).
   */
  public void play() {
    if (player != null && isPaused) {
      togglePause();
    }
  }

  /**
   * Pauses playback.
   */
  public void pause() {
    if (player != null && !isPaused) {
      togglePause();
    }
  }

  private void beginPlayback() {
    player.addListener(new VideoLooperListener());
    player.prepare(videoRenderer, audioRenderer);

    player.sendMessage(videoRenderer, MediaCodecVideoTrackRenderer.MSG_SET_SURFACE, surface);
    player.seekTo(0);
    player.setPlayWhenReady(true);
  }

  private void releasePlayer() {
    if (currentAsyncBuilder != null) {
      currentAsyncBuilder.cancel();
      currentAsyncBuilder = null;
    }
    surface = null;
    if (player != null) {
      isPaused = false;
      player.release();
      player = null;
    }
  }

  private void onRenderers(TrackRenderer videoRenderer, TrackRenderer audioRenderer) {
    this.videoRenderer = videoRenderer;
    this.audioRenderer = audioRenderer;

    if (isSurfaceCreated) {
      beginPlayback();
    }
  }

  private void onRenderersError(Exception e) {
    Log.e(TAG, "Renderer init error: ", e);
  }

  private Looper getPlaybackLooper() {
    return player.getPlaybackLooper();
  }

  private Handler getMainHandler() {
    return mainHandler;
  }

  /**
   * Class to seek to the start of a video once it ends.
   */
  private final class VideoLooperListener implements ExoPlayer.Listener {
    @Override
    public void onPlayerError(ExoPlaybackException error) {
      Log.e(TAG, "ExoPlayer error", error);
      for (Listener listener : listeners) {
        listener.onError(error);
      }
    }

    @Override
    public void onPlayerStateChanged(boolean playWhenReady, int playbackState) {
      Log.i(TAG, "ExoPlayer state changed " + playWhenReady + " : " + playbackState);
      if (playWhenReady && playbackState == ExoPlayer.STATE_ENDED) {
        player.seekTo(0); // this line causes playback to loop
      }
    }

    @Override
    public void onPlayWhenReadyCommitted() {}
  }

  private static final class AsyncRendererBuilder
      implements ManifestFetcher.ManifestCallback<MediaPresentationDescription>,
          UtcTimingElementResolver.UtcTimingCallback {

    private final Context context;
    private final String userAgent;
    private final boolean requiresSecurePlayback;
    private final MediaDrmCallback drmCallback;
    private final VideoExoPlayer player;
    private final ManifestFetcher<MediaPresentationDescription> manifestFetcher;
    private final UriDataSource manifestDataSource;

    private boolean canceled;
    private MediaPresentationDescription manifest;
    private long elapsedRealtimeOffset;

    public AsyncRendererBuilder(
        Context context,
        String userAgent,
        String url,
        boolean requiresSecurePlayback,
        MediaDrmCallback drmCallback,
        VideoExoPlayer player) {
      this.context = context;
      this.userAgent = userAgent;
      this.requiresSecurePlayback = requiresSecurePlayback;
      this.drmCallback = drmCallback;
      this.player = player;
      MediaPresentationDescriptionParser parser = new MediaPresentationDescriptionParser();
      manifestDataSource = new DefaultUriDataSource(context, userAgent);
      manifestFetcher = new ManifestFetcher<>(url, manifestDataSource, parser);
    }

    public void init() {
      manifestFetcher.singleLoad(player.getMainHandler().getLooper(), this);
    }

    public void cancel() {
      canceled = true;
    }

    @Override
    public void onSingleManifest(MediaPresentationDescription manifest) {
      if (canceled) {
        return;
      }

      this.manifest = manifest;
      if (manifest.dynamic && manifest.utcTiming != null) {
        UtcTimingElementResolver.resolveTimingElement(
            manifestDataSource,
            manifest.utcTiming,
            manifestFetcher.getManifestLoadCompleteTimestamp(),
            this);
      } else {
        buildRenderers();
      }
    }

    @Override
    public void onSingleManifestError(IOException e) {
      if (canceled) {
        return;
      }

      player.onRenderersError(e);
    }

    @Override
    public void onTimestampResolved(UtcTimingElement utcTiming, long elapsedRealtimeOffset) {
      if (canceled) {
        return;
      }

      this.elapsedRealtimeOffset = elapsedRealtimeOffset;
      buildRenderers();
    }

    @Override
    public void onTimestampError(UtcTimingElement utcTiming, IOException e) {
      if (canceled) {
        return;
      }

      Log.e(TAG, "Failed to resolve UtcTiming element [" + utcTiming + "]", e);
      // Be optimistic and continue in the hope that the device clock is correct.
      buildRenderers();
    }

    private void buildRenderers() {
      Period period = manifest.getPeriod(0);
      Handler mainHandler = player.getMainHandler();
      LoadControl loadControl = new DefaultLoadControl(new DefaultAllocator(BUFFER_SEGMENT_SIZE));
      DefaultBandwidthMeter bandwidthMeter = new DefaultBandwidthMeter(mainHandler, null);

      boolean hasContentProtection = false;
      for (int i = 0; i < period.adaptationSets.size(); i++) {
        AdaptationSet adaptationSet = period.adaptationSets.get(i);
        if (adaptationSet.type != AdaptationSet.TYPE_UNKNOWN) {
          hasContentProtection |= adaptationSet.hasContentProtection();
        }
      }

      // Check drm support if necessary.
      boolean filterHdContent = false;
      StreamingDrmSessionManager drmSessionManager = null;
      if (hasContentProtection) {
        if (Util.SDK_INT < 18) {
          player.onRenderersError(
              new UnsupportedDrmException(UnsupportedDrmException.REASON_UNSUPPORTED_SCHEME));
          return;
        }
        try {
          drmSessionManager =
              StreamingDrmSessionManager.newWidevineInstance(
                  player.getPlaybackLooper(), drmCallback, null, player.getMainHandler(), null);

          if (!requiresSecurePlayback) {
            // Force to L3 to be able to direct to the Surface.
            drmSessionManager.setPropertyString("securityLevel", "L3");
          }

          filterHdContent = getWidevineSecurityLevel(drmSessionManager) != SECURITY_LEVEL_1;
        } catch (UnsupportedDrmException e) {
          player.onRenderersError(e);
          return;
        }
      }

      // Build the video renderer.
      DataSource videoDataSource = new DefaultUriDataSource(context, bandwidthMeter, userAgent);
      ChunkSource videoChunkSource =
          new DashChunkSource(
              manifestFetcher,
              DefaultDashTrackSelector.newVideoInstance(context, true, filterHdContent),
              videoDataSource,
              new FormatEvaluator.AdaptiveEvaluator(bandwidthMeter),
              LIVE_EDGE_LATENCY_MS,
              elapsedRealtimeOffset,
              mainHandler,
              null,
              0);
      ChunkSampleSource videoSampleSource =
          new ChunkSampleSource(
              videoChunkSource,
              loadControl,
              VIDEO_BUFFER_SEGMENTS * BUFFER_SEGMENT_SIZE,
              mainHandler,
              null,
              VideoExoPlayer.TYPE_VIDEO);
      TrackRenderer videoRenderer =
          new MediaCodecVideoTrackRenderer(
              context,
              videoSampleSource,
              MediaCodecSelector.DEFAULT,
              MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT,
              5000,
              drmSessionManager,
              true,
              mainHandler,
              null,
              50);

      // Build the audio renderer.
      DataSource audioDataSource = new DefaultUriDataSource(context, bandwidthMeter, userAgent);
      ChunkSource audioChunkSource =
          new DashChunkSource(
              manifestFetcher,
              DefaultDashTrackSelector.newAudioInstance(),
              audioDataSource,
              null,
              LIVE_EDGE_LATENCY_MS,
              elapsedRealtimeOffset,
              mainHandler,
              null,
              0);
      ChunkSampleSource audioSampleSource =
          new ChunkSampleSource(
              audioChunkSource,
              loadControl,
              AUDIO_BUFFER_SEGMENTS * BUFFER_SEGMENT_SIZE,
              mainHandler,
              null,
              VideoExoPlayer.TYPE_AUDIO);
      TrackRenderer audioRenderer =
          new MediaCodecAudioTrackRenderer(
              audioSampleSource,
              MediaCodecSelector.DEFAULT,
              drmSessionManager,
              true,
              mainHandler,
              null);

      // Invoke the callback.
      player.onRenderers(videoRenderer, audioRenderer);
    }

    private static int getWidevineSecurityLevel(StreamingDrmSessionManager sessionManager) {
      String securityLevelProperty = sessionManager.getPropertyString("securityLevel");
      Log.d(TAG, "WV security: " + securityLevelProperty);
      return securityLevelProperty.equals("L1")
          ? SECURITY_LEVEL_1
          : securityLevelProperty.equals("L3") ? SECURITY_LEVEL_3 : SECURITY_LEVEL_UNKNOWN;
    }
  }

  /* package */ static final class WidevineTestMediaDrmCallback implements MediaDrmCallback {

    private static final String WIDEVINE_GTS_DEFAULT_BASE_URI =
        "http://wv-staging-proxy.appspot.com/proxy?provider=YouTube&video_id=";

    private final String defaultUri;

    public WidevineTestMediaDrmCallback(String videoId) {
      defaultUri = WIDEVINE_GTS_DEFAULT_BASE_URI + videoId;
    }

    @Override
    public byte[] executeProvisionRequest(UUID uuid, ProvisionRequest request) throws IOException {
      String url = request.getDefaultUrl() + "&signedRequest=" + new String(request.getData());
      return Util.executePost(url, null, null);
    }

    @Override
    public byte[] executeKeyRequest(UUID uuid, KeyRequest request) throws IOException {
      String url = request.getDefaultUrl();
      if (TextUtils.isEmpty(url)) {
        url = defaultUri;
      }
      return Util.executePost(url, request.getData(), null);
    }
  }
}
