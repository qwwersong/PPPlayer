package com.songlei.xplayer.player;

import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.Surface;

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
    private static int logLevel = IjkMediaPlayer.IJK_LOG_DEFAULT;
    private static IjkLibLoader ijkLibLoader;
    private IjkMediaPlayer mediaPlayer;
    private Surface surface;

    private PlayerListener playerListener;

    private boolean isModifyTone = false;
    private boolean isBufferCache = false;

    public IjkPlayer(Context context){
        mediaPlayer = (ijkLibLoader == null) ? new IjkMediaPlayer() : new IjkMediaPlayer(ijkLibLoader);
        mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mediaPlayer.setOnNativeInvokeListener(new IjkMediaPlayer.OnNativeInvokeListener() {
            @Override
            public boolean onNativeInvoke(int i, Bundle bundle) {
                return true;
            }
        });
        mediaPlayer.native_setLogLevel(logLevel);

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
    public void setSurface(Surface surface) {
        this.surface = surface;
        if (mediaPlayer != null && surface.isValid()) {
            mediaPlayer.setSurface(surface);
        }
    }

    @Override
    public void setPlayerListener(PlayerListener playerListener) {
        this.playerListener = playerListener;
    }

    @Override
    public void onCompletion(IMediaPlayer iMediaPlayer) {
        playerListener.onCompletion();
    }

    @Override
    public void onPrepared(IMediaPlayer iMediaPlayer) {
        playerListener.onPrepared();
    }

    @Override
    public void onSeekComplete(IMediaPlayer iMediaPlayer) {
        playerListener.onSeekComplete();
    }

    @Override
    public boolean onError(IMediaPlayer iMediaPlayer, int what, int extra) {
        playerListener.onError(what, extra);
        return true;
    }

    @Override
    public boolean onInfo(IMediaPlayer iMediaPlayer, int what, int extra) {
        playerListener.onInfo(what, extra);
        return false;
    }

    @Override
    public void onVideoSizeChanged(IMediaPlayer iMediaPlayer, int width, int height, int sar_num, int sar_den) {
        playerListener.onVideoSizeChanged(width, height);
    }

    @Override
    public void onBufferingUpdate(IMediaPlayer iMediaPlayer, int i) {

    }
}
