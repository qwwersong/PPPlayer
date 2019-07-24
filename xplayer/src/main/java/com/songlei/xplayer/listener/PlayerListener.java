package com.songlei.xplayer.listener;

/**
 * Created by songlei on 2019/07/04.
 */
public interface PlayerListener {
//
//    void onCompletion();
//
//    void onPrepared();
//
//    void onSeekComplete();
//
//    boolean onError(int what, int extra);
//
//    boolean onInfo(int what, int extra);
//
//    void onVideoSizeChanged(int width, int height);

    void onBufferingUpdate(int percent);

    void onPlayerState(int state);

    void onPlayerError(int error, int extra);
}
