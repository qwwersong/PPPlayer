package com.songlei.xplayer.player;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.PlayerConstants;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.listener.PlayerListener;

import java.io.IOException;
import java.util.HashMap;

import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkLibLoader;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

/**
 * Created by songlei on 2019/07/02.
 */
public class IjkPlayer implements IPlayer, IMediaPlayer.OnCompletionListener,
        IMediaPlayer.OnPreparedListener, IMediaPlayer.OnSeekCompleteListener,
        IMediaPlayer.OnErrorListener, IMediaPlayer.OnInfoListener,
        IMediaPlayer.OnVideoSizeChangedListener, IMediaPlayer.OnBufferingUpdateListener {
    private static int logLevel = IjkMediaPlayer.IJK_LOG_SILENT;
    private static IjkLibLoader ijkLibLoader;
    private IjkMediaPlayer mediaPlayer;
    private Surface surface;

    private PlayerListener playerListener;

    private boolean isModifyTone = false;
    private boolean isBufferCache = false;

    @Override
    public void initPlayer(Context context) {
        mediaPlayer = (ijkLibLoader == null) ? new IjkMediaPlayer() : new IjkMediaPlayer(ijkLibLoader);
        mediaPlayer.setLogEnabled(false);
        mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mediaPlayer.setOnNativeInvokeListener(new IjkMediaPlayer.OnNativeInvokeListener() {
            @Override
            public boolean onNativeInvoke(int i, Bundle bundle) {
                return true;
            }
        });
        mediaPlayer.native_setLogLevel(logLevel);

        //开启硬解码
        if (Option.isMediaCodec()) {
            Log.e("xxx", "硬解码");
            mediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 1);
            mediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 1);
            mediaPlayer.setOption(IjkMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 1);
        }
        Log.e("xxx", "软解码");

        mediaPlayer.setScreenOnWhilePlaying(true);

        mediaPlayer.setOnCompletionListener(this);
        mediaPlayer.setOnBufferingUpdateListener(this);
        mediaPlayer.setOnPreparedListener(this);
        mediaPlayer.setOnSeekCompleteListener(this);
        mediaPlayer.setOnErrorListener(this);
        mediaPlayer.setOnInfoListener(this);
        mediaPlayer.setOnVideoSizeChangedListener(this);
    }

    @Override
    public void prepare(String url) {
        try {
            mediaPlayer.setDataSource(url, new HashMap<String, String>());
            mediaPlayer.prepareAsync();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void start() {
        if (mediaPlayer != null) {
            mediaPlayer.start();
            playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
        }
    }

    @Override
    public void stop() {
        if (mediaPlayer != null) {
            mediaPlayer.stop();
        }
    }

    @Override
    public void pause() {
        if (mediaPlayer != null) {
            mediaPlayer.pause();
            playerListener.onPlayerState(PlayerConstants.STATE_PAUSE);
        }
    }

    @Override
    public void resume() {
        if (mediaPlayer != null) {
            mediaPlayer.start();
            playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
        }
    }

    @Override
    public void release() {
        if (mediaPlayer != null) {
            mediaPlayer.release();
        }
        if (surface != null) {
            surface.release();
            surface = null;
        }
    }

    @Override
    public void setPlayPosition(int playPosition) {

    }

    @Override
    public void seekTo(long time) {
        if (mediaPlayer != null) {
            mediaPlayer.seekTo(time);
        }
    }

    @Override
    public int getVideoWidth() {
        if (mediaPlayer != null) {
            return mediaPlayer.getVideoWidth();
        }
        return 0;
    }

    @Override
    public int getVideoHeight() {
        if (mediaPlayer != null) {
            return mediaPlayer.getVideoHeight();
        }
        return 0;
    }

    @Override
    public boolean isPlaying() {
        if (mediaPlayer != null) {
            return mediaPlayer.isPlaying();
        }
        return false;
    }

    @Override
    public long getCurrentPosition() {
        if (mediaPlayer != null) {
            return mediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    @Override
    public long getDuration() {
        if (mediaPlayer != null) {
            return mediaPlayer.getDuration();
        }
        return 0;
    }

    @Override
    public int getBufferedPercentage() {
        return -1;
    }

    @Override
    public void setSurface(Surface surface) {
        this.surface = surface;
        if (mediaPlayer != null) {
            mediaPlayer.setSurface(surface);
        }
    }

    @Override
    public void setPlayerListener(PlayerListener playerListener) {
        this.playerListener = playerListener;
    }

    @Override
    public void onCompletion(IMediaPlayer iMediaPlayer) {
        playerListener.onPlayerState(PlayerConstants.STATE_COMPLETE);
    }

    @Override
    public void onPrepared(IMediaPlayer iMediaPlayer) {
        playerListener.onPlayerState(PlayerConstants.STATE_PREPARE);
    }

    @Override
    public void onSeekComplete(IMediaPlayer iMediaPlayer) {
    }

    @Override
    public boolean onError(IMediaPlayer iMediaPlayer, int what, int extra) {
        playerListener.onPlayerError(what, extra);
        return true;
    }

    @Override
    public boolean onInfo(IMediaPlayer iMediaPlayer, int what, int extra) {
        Log.e("xxx", "Ijk onInfo what = " + what);
        switch (what) {
            case IjkMediaPlayer.MEDIA_INFO_BUFFERING_END:
                playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
                break;
            case IjkMediaPlayer.MEDIA_INFO_BUFFERING_START:
                playerListener.onPlayerState(PlayerConstants.STATE_BUFFERING);
                break;
            case IjkMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START:
                playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
                break;
        }
        return false;
    }

    @Override
    public void onVideoSizeChanged(IMediaPlayer iMediaPlayer, int width, int height, int sar_num, int sar_den) {

    }

    @Override
    public void onBufferingUpdate(IMediaPlayer iMediaPlayer, int percent) {
        playerListener.onBufferingUpdate(percent);
    }
}
