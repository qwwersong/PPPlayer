package com.songlei.xplayer;

import android.content.Context;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.listener.PlayerListener;
import com.songlei.xplayer.player.IPlayer;
import com.songlei.xplayer.player.ObssPlayer;
import com.songlei.xplayer.player.PlayerFactory;

import java.lang.ref.WeakReference;

/**
 * Created by songlei on 2019/07/02.
 */
public class PlayerManager {
    private IPlayer player;
    private static PlayerManager playerManager;
    protected int playerPosition = -22;
    protected WeakReference<PlayerListener> mPlayerListener;

    public static synchronized PlayerManager getInstance() {
        if (playerManager == null) {
            playerManager = new PlayerManager();
        }
        return playerManager;
    }

    public void initPlayer(Context context){
        Log.e("xxx", "PlayerManager initPlayer");
        player = PlayerFactory.getPlayer();
        if (player != null) {
            player.initPlayer(context);
        }
    }

    public void prepare(String url){
        Log.e("xxx", "PlayerManager prepare");
        if (player != null) {
            player.prepare(url);
        }
    }

    public void setSurface(Surface surface){
        Log.e("xxx", "PlayerManager setSurface");
        if (player != null) {
            player.setSurface(surface);
        }
    }

    public void start(){
        Log.e("xxx", "PlayerManager start");
        if (player != null) {
            player.start();
        }
    }

    public void pause(){
        Log.e("xxx", "PlayerManager pause");
        if (player != null) {
            player.pause();
        }
    }

    public void resume(){
        Log.e("xxx", "PlayerManager resume");
        if (player != null) {
            player.resume();
        }
    }

    public void stop(){
        Log.e("xxx", "PlayerManager stop");
        if (player != null) {
            playerPosition = -22;
            player.stop();
        }
    }

    public void release(){
        Log.e("xxx", "PlayerManager release");
        if (player != null) {
            mPlayerListener.get().onPlayerState(PlayerConstants.STATE_COMPLETE);
            player.release();
        }
    }

    public void seekTo(long time){
        Log.e("xxx", "PlayerManager seekTo time = " + time);
        if (player != null) {
            player.seekTo(time);
        }
    }

    public void setSpeedPlaying(float speed, boolean soundTouch){
        if (player != null) {
            player.setSpeedPlaying(speed, soundTouch);
        }
    }

    public void setPlayPosition(long time){
        if (player != null) {
            player.setPlayPosition((int) time);
        }
    }

    public int getCurrentVideoWidth(){
        if (player != null) {
            return player.getVideoWidth();
        }
        return 0;
    }

    public int getCurrentVideoHeight() {
        if (player != null) {
            return player.getVideoHeight();
        }
        return 0;
    }

    public long getCurrentPosition(){
        if (player != null) {
            return player.getCurrentPosition();
        }
        return 0;
    }

    public long getDuration(){
        if (player != null) {
            return player.getDuration();
        }
        return 0;
    }

    public int getBufferedPercentage(){
        if (player != null) {
            return player.getBufferedPercentage();
        }
        return 0;
    }

    public void setPlayerListener(PlayerListener playerListener){
        if (playerListener != null) {
            this.mPlayerListener = new WeakReference<>(playerListener);
            if (player != null) {
                player.setPlayerListener(playerListener);
            }
        }
    }

    public void surfaceChanged(Surface surface){
        if (player != null && player instanceof ObssPlayer) {
            ((ObssPlayer)player).surfaceChanged(surface);
        }
    }

    public void setPlayerPosition(int playerPosition){
        this.playerPosition = playerPosition;
    }

    public int getPlayerPosition(){
        return playerPosition;
    }

}
