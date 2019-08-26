package com.songlei.xplayer.player;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.songlei.obssplayer.ObssConstants;
import com.songlei.obssplayer.OnObssListener;
import com.songlei.obssplayer.PlayException;
import com.songlei.xplayer.PlayerConstants;
import com.songlei.xplayer.listener.PlayerListener;

import nativeInterface.playerView;


/**
 * Created by songlei on 2019/07/04.
 */
public class ObssPlayer implements IPlayer {
    private String url;
    private playerView obssPlayer;
    private int playPosition;
    private PlayerListener playerListener;

    @Override
    public void initPlayer(Context context) {
        obssPlayer = new playerView(context);
        obssPlayer.setOnObssListener(onObssListener);
    }

    private OnObssListener onObssListener = new OnObssListener() {
        @Override
        public void onError(PlayException e) {
            Log.e("xxx", "Obss onError = " + e.code);
            playerListener.onPlayerError(e.code, 0);
        }

        @Override
        public void onPlayState(int code) {
            switch (code) {
                case ObssConstants.STATE_PLAYING:
                    playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
                    break;
                case ObssConstants.STATE_FINISH:
                    playerListener.onPlayerState(PlayerConstants.STATE_COMPLETE);
                    break;
                case ObssConstants.STATE_BUFFERING:
                    playerListener.onPlayerState(PlayerConstants.STATE_BUFFERING);
                    break;
                case ObssConstants.STATE_PREPARE:
                    playerListener.onPlayerState(PlayerConstants.STATE_PREPARE);
                    break;
                case ObssConstants.STATE_PAUSE:
                    playerListener.onPlayerState(PlayerConstants.STATE_PAUSE);
                    break;
            }
        }

        @Override
        public void onBufferingUpdate(int percent) {
            playerListener.onBufferingUpdate(percent);
        }
    };

    @Override
    public void prepare(String url) {
        this.url = url;
        if (!TextUtils.isEmpty(url)) {
            obssPlayer.prepare(url, playPosition);
//        start();
        } else {
            Log.e("xxx", "url为空，准备失败");
        }
    }

    @Override
    public void start() {
        if (!TextUtils.isEmpty(url)) {
            obssPlayer.startPlay(url, playPosition);
        } else {
            Log.e("xxx", "url为空，顺序执行无法播放");
        }
    }

    @Override
    public void stop() {
        obssPlayer.stop(false);
    }

    @Override
    public void pause() {
        obssPlayer.pause();
    }

    @Override
    public void resume() {
        obssPlayer.resume();
    }

    @Override
    public void release() {
        obssPlayer.release();
    }

    @Override
    public void setPlayPosition(int playPosition) {
        this.playPosition = playPosition;
    }

    @Override
    public void seekTo(long time) {
        obssPlayer.seekTo((int) time);
    }

    @Override
    public void setSpeedPlaying(float speed, boolean soundTouch) {

    }

    @Override
    public int getVideoWidth() {
        return obssPlayer.getVideoWidth();
    }

    @Override
    public int getVideoHeight() {
        return obssPlayer.getVideoHeight();
    }

    @Override
    public boolean isPlaying() {
        return obssPlayer.getPlayState() == ObssConstants.STATE_PLAYING;
    }

    @Override
    public long getCurrentPosition() {
        return obssPlayer.getCurrentTime();
    }

    @Override
    public long getDuration() {
        return obssPlayer.getDuration();
    }

    @Override
    public int getBufferedPercentage() {
        return obssPlayer.getBufferPercentage();
    }

    @Override
    public void setSurface(Surface surface) {
        if (surface != null) {
            obssPlayer.setSurface(surface);
        }
    }

    public void surfaceChanged(Surface surface) {
        if (surface != null) {
            obssPlayer.surfaceChange(surface);
        }
    }

    @Override
    public void setPlayerListener(PlayerListener playerListener) {
        this.playerListener = playerListener;
    }
}
