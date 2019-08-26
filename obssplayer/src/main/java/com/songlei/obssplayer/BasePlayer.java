package com.songlei.obssplayer;

import android.view.Surface;

/**
 * Created by songlei on 2018/11/08.
 */
public abstract class BasePlayer {

    public BasePlayer(){

    }

    public abstract void startPlay(String uri, int startTime);

    public abstract void stop(boolean isCallBack);

    public abstract void pause();

    public abstract void resume();

    public abstract void release();

    public abstract void seekTo(int ms);//Video

    //状态设置
    public abstract void setScaleType(int scaleType);

    public abstract int getCurrentTime();//Video

    public abstract int getDuration();//Video

    public abstract int getBufferTime();

    public abstract void setOnObssListener(OnObssListener onObssListener);

    //0：不静音 1：静音
    public abstract void setMute(int mute);

    public abstract int getVideoWidth();

    public abstract int getVideoHeight();

    public abstract void surfaceChange(Surface surface);

    public abstract void setSurface(Surface surface);

    public abstract int getPlayState();
}
