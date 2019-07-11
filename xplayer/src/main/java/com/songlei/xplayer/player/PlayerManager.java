package com.songlei.xplayer.player;

import android.content.Context;
import android.util.Log;
import android.view.Surface;

import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.listener.PlayerListener;

/**
 * Created by songlei on 2019/07/02.
 */
public class PlayerManager {
    private IPlayer player;
    private Context context;

    public PlayerManager(Context context){
        this.context = context;
    }

    public void initPlayer(Option option){
        switch (Option.getPlayerType()) {
            case Option.PLAYER_IJK:
                Log.e("xxx", "创建播放器 Ijk");
                player = new IjkPlayer(context);
                break;
            case Option.PLAYER_OBSS:
                Log.e("xxx", "创建播放器 Obss");
                player = new ObssPlayer(context);
                break;

        }
    }

    public void prepare(String url){
        player.prepare(url);
    }

    public void setSurface(Surface surface){
        player.setSurface(surface);
    }

    public void start(){
        player.start();
    }

    public void pause(){
        player.pause();
    }

    public void resume(){
        player.resume();
    }

    public void stop(){
        player.stop();
    }

    public void release(){
        player.release();
    }

    public void seekTo(long time){
        player.seekTo(time);
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

    public void setPlayerListener(PlayerListener playerListener){
        player.setPlayerListener(playerListener);
    }

}
