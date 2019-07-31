package com.songlei.xplayer;

import android.content.Context;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.listener.PlayerListener;
import com.songlei.xplayer.player.IPlayer;
import com.songlei.xplayer.player.ObssPlayer;
import com.songlei.xplayer.player.PlayerFactory;

/**
 * Created by songlei on 2019/07/02.
 */
public class PlayerManager {
    private IPlayer player;
    private Context context;

    public PlayerManager(Context context){
        this.context = context;
    }

    public void initPlayer(){
        Log.e("xxx", "PlayerManager initPlayer");
        player = PlayerFactory.getPlayer();
        player.initPlayer(context);
    }

    public void prepare(String url){
        Log.e("xxx", "PlayerManager prepare");
        player.prepare(url);
    }

    public void setSurface(Surface surface){
        Log.e("xxx", "PlayerManager setSurface");
        player.setSurface(surface);
    }

    public void start(){
        Log.e("xxx", "PlayerManager start");
        player.start();
    }

    public void pause(){
        Log.e("xxx", "PlayerManager pause");
        player.pause();
    }

    public void resume(){
        Log.e("xxx", "PlayerManager resume");
        player.resume();
    }

    public void stop(){
        Log.e("xxx", "PlayerManager stop");
        player.stop();
    }

    public void release(){
        Log.e("xxx", "PlayerManager release");
        player.release();
    }

    public void seekTo(long time){
        Log.e("xxx", "PlayerManager seekTo time = " + time);
        player.seekTo(time);
    }

    public void setSpeedPlaying(float speed, boolean soundTouch){
        player.setSpeedPlaying(speed, soundTouch);
    }

    public void setPlayPosition(long time){
        player.setPlayPosition((int) time);
    }

    public int getCurrentVideoWidth(){
        return player.getVideoWidth();
    }

    public int getCurrentVideoHeight() {
        return player.getVideoHeight();
    }

    public long getCurrentPosition(){
        return player.getCurrentPosition();
    }

    public long getDuration(){
        return player.getDuration();
    }

    public int getBufferedPercentage(){
        return player.getBufferedPercentage();
    }

    public void setPlayerListener(PlayerListener playerListener){
        player.setPlayerListener(playerListener);
    }

    public void surfaceChanged(Surface surface){
        if (player instanceof ObssPlayer) {
            ((ObssPlayer)player).surfaceChanged(surface);
        }
    }

}
