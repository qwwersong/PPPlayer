package com.songlei.slplayer.view;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.Surface;

import com.songlei.slplayer.bean.PitchBean;
import com.songlei.slplayer.bean.SpeedBean;
import com.songlei.slplayer.egl.PlayerRender;
import com.songlei.slplayer.listener.OnCompleteListener;
import com.songlei.slplayer.listener.OnErrorListener;
import com.songlei.slplayer.listener.OnTimeInfoListener;
import com.songlei.slplayer.player.Player;


/**
 * Created by songlei on 2019/04/01.
 */
public class PlayerGLSurfaceView extends GLSurfaceView {
    private PlayerRender playerRender;
    private Player player;

    public PlayerGLSurfaceView(Context context) {
        this(context, null);
    }

    public PlayerGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);

        player = new Player();
        player.init();

        playerRender = new PlayerRender(context);
        setRenderer(playerRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        playerRender.setOnRenderListener(new PlayerRender.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();
            }
        });
        playerRender.setOnSurfaceCreateListener(new PlayerRender.OnSurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(Surface surface) {
                player.setNativeSurface(surface);
            }
        });
    }

    public void start(String url){
        player.startPlay();
    }

    public void pause(){
        player.pause();
    }

    public void resume(){
        player.resume();
    }
    //停止播放，释放内存
    public void stop(){
        player.stop();
    }

    public void seek(int sec){
        player.seek(sec);
    }

    public void seekVolume(int volume){
        player.seekVolume(volume);
    }

    public int getVolume(){
        return player.getVolume();
    }

    public void setChannel(int type){
        player.setChannelMute(type);
    }

    public void setPitch(PitchBean pitch){
        player.setPitch(pitch.getValue());
    }

    public void setSpeed(SpeedBean speed){
        player.setSpeed(speed.getValue());
    }

    public void setOnTimeInfoListener(OnTimeInfoListener onTimeInfoListener) {
        player.setOnTimeInfoListener(onTimeInfoListener);
    }

    public void setOnCompleteListener(OnCompleteListener onCompleteListener){
        player.setOnCompleteListener(onCompleteListener);
    }

    public void setOnErrorListener(OnErrorListener onErrorListener){
        player.setOnErrorListener(onErrorListener);
    }
}
