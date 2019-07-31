package com.songlei.xplayer.player;

import android.util.Log;

/**
 * Created by songlei on 2019/07/29.
 */
public class PlayerFactory {

    private static Class<? extends IPlayer> sPlayer;

    public static void setPlayer(Class<? extends IPlayer> player){
        Log.e("xxx", "设置Player sPlayer = " + player);
        sPlayer = player;
    }

    public static IPlayer getPlayer(){
        Log.e("xxx", "创建Player sPlayer = " + sPlayer);
        if (sPlayer == null) {
            sPlayer = IjkPlayer.class;
        }
        try {
            return sPlayer.newInstance();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
        return null;
    }

}
