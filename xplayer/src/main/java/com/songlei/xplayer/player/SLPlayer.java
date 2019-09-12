package com.songlei.xplayer.player;

import android.content.Context;
import android.text.TextUtils;
import android.view.Surface;

import com.songlei.slplayer.listener.OnCompleteListener;
import com.songlei.slplayer.listener.OnErrorListener;
import com.songlei.slplayer.listener.OnPrepareListener;
import com.songlei.slplayer.player.Player;
import com.songlei.xplayer.PlayerConstants;
import com.songlei.xplayer.listener.PlayerListener;

/**
 * Created by songlei on 2019/08/30.
 */
public class SLPlayer implements IPlayer, OnCompleteListener, OnErrorListener, OnPrepareListener {
    private Player player;
    private PlayerListener playerListener;

    @Override
    public void initPlayer(Context context) {
        player = new Player();
        player.init();
        player.setOnCompleteListener(this);
        player.setOnErrorListener(this);
        player.setOnPrepareListener(this);
    }

    @Override
    public void prepare(String url) {
        if (!TextUtils.isEmpty(url)) {
            player.prepare(url);
        }
    }

    @Override
    public void start() {
        player.startPlay();
        playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
    }

    @Override
    public void stop() {
        player.stop();
    }

    @Override
    public void pause() {
        player.pause();
        playerListener.onPlayerState(PlayerConstants.STATE_PAUSE);
    }

    @Override
    public void resume() {
        player.resume();
        playerListener.onPlayerState(PlayerConstants.STATE_PLAYING);
    }

    @Override
    public void release() {

    }

    @Override
    public void setPlayPosition(int playPosition) {

    }

    @Override
    public void seekTo(long time) {
        player.seek((int) time);
    }

    @Override
    public void setSpeedPlaying(float speed, boolean soundTouch) {

    }

    @Override
    public int getVideoWidth() {
        return 0;
    }

    @Override
    public int getVideoHeight() {
        return 0;
    }

    @Override
    public boolean isPlaying() {
        return false;
    }

    @Override
    public long getCurrentPosition() {
        if (player != null) {
            return player.getCurrentTime();
        }
        return 0;
    }

    @Override
    public long getDuration() {
        if (player != null) {
            return player.getDuration();
        }
        return 0;
    }

    @Override
    public int getBufferedPercentage() {
        return 0;
    }

    @Override
    public void setSurface(Surface surface) {
        if (surface != null) {
            player.setNativeSurface(surface);
        }
    }

    public void surfaceChanged(Surface surface) {
        if (surface != null) {
//            player.onSurfaceChange();
        }
    }

    @Override
    public void setPlayerListener(PlayerListener playerListener) {
        this.playerListener = playerListener;
    }

    @Override
    public void onComplete() {
        if (playerListener != null) {
            playerListener.onPlayerState(PlayerConstants.STATE_COMPLETE);
        }

    }

    @Override
    public void onError(int code, String msg) {
        if (playerListener != null) {
            playerListener.onPlayerError(code, code);
        }
    }

    @Override
    public void onPrepare() {
        if (playerListener != null) {
            playerListener.onPlayerState(PlayerConstants.STATE_PREPARE);
        }
    }
}
