package com.songlei.xplayer.listener;

/**
 * Created by songlei on 2019/07/04.
 */
public interface PlayerListener {

    void onBufferingUpdate(int percent);

    void onPlayerState(int state);

    void onPlayerError(int error, int extra);
}
