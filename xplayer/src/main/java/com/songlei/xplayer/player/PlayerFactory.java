package com.songlei.xplayer.player;

/**
 * Created by songlei on 2019/07/29.
 */
public class PlayerFactory {

    private static Class<? extends IPlayer> sPlayer;

    public static void setPlayer(Class<? extends IPlayer> player){
        sPlayer = player;
    }

    public static IPlayer getPlayer(){
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
