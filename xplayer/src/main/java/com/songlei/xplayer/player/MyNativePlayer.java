package com.songlei.xplayer.player;

import android.view.Surface;

import com.songlei.xplayer.listener.PlayerListener;

/**
 * Created by songlei on 2019/07/02.
 */
public class MyNativePlayer implements IPlayer {
    @Override
    public void prepare(String url) {

    }

    @Override
    public void start() {

    }

    @Override
    public void stop() {

    }

    @Override
    public void pause() {

    }

    @Override
    public void release() {

    }

    @Override
    public void setPlayPosition(int playPosition) {

    }

    @Override
    public void seekTo(long time) {

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
        return 0;
    }

    @Override
    public long getDuration() {
        return 0;
    }

    @Override
    public void setSurface(Surface surface) {

    }

    @Override
    public void setPlayerListener(PlayerListener playerListener) {

    }
}
