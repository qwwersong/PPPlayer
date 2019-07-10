package com.songlei.xplayer.player;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.listener.PlayerListener;
import com.songlei.xplayer.obssplayer.PlayerConstants;

import nativeInterface.playerView;

/**
 * Created by songlei on 2019/07/04.
 */
public class ObssPlayer implements IPlayer {
    private String url;
    private playerView obssPlayer;
    private int playPosition;

    public ObssPlayer(Context context){
        obssPlayer = new playerView(context);
    }

    @Override
    public void prepare(String url) {
        this.url = url;
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
    public int getVideoWidth() {
        return obssPlayer.getVideoWidth();
    }

    @Override
    public int getVideoHeight() {
        return obssPlayer.getVideoHeight();
    }

    @Override
    public boolean isPlaying() {
        return obssPlayer.getPlayState() == PlayerConstants.STATE_PLAYING;
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
    public void setSurface(Surface surface) {
        if (surface != null) {
            obssPlayer.setSurface(surface);
        }
    }

    @Override
    public void setPlayerListener(PlayerListener playerListener) {

    }
}
