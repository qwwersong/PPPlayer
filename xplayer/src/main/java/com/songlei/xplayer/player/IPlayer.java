package com.songlei.xplayer.player;

import android.content.Context;
import android.view.Surface;

import com.songlei.xplayer.listener.PlayerListener;

/**
 * Created by songlei on 2019/07/02.
 */
public interface IPlayer {

    void initPlayer(Context context);

    void prepare(String url);

    void start();

    void stop();

    void pause();

    void resume();

    void release();

    void setPlayPosition(int playPosition);

    void seekTo(long time);

    int getVideoWidth();

    int getVideoHeight();

    boolean isPlaying();

    long getCurrentPosition();

    long getDuration();

    int getBufferedPercentage();

    void setSurface(Surface surface);

    void setPlayerListener(PlayerListener playerListener);

}
