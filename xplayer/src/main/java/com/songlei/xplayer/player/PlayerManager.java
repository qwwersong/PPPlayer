package com.songlei.xplayer.player;

import android.content.Context;
import android.view.Surface;

import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.listener.PlayerListener;

/**
 * Created by songlei on 2019/07/02.
 */
public class PlayerManager {
    private IPlayer player;
    private Context context;

//    private PlayerListener playerListener;

    public PlayerManager(Context context){
        this.context = context;
    }

    public void initPlayer(Option option){
        switch (Option.getPlayerType()) {
            case Option.PLAYER_IJK:
                player = new IjkPlayer(context);
                break;
            case Option.PLAYER_OBSS:
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

    public int getCurrentVideoWidth(){
        return player.getVideoWidth();
    }

    public int getCurrentVideoHeight() {
        return player.getVideoHeight();
    }

    public void setPlayerListener(PlayerListener playerListener){
//        this.playerListener = playerListener;
        player.setPlayerListener(playerListener);
    }

}
