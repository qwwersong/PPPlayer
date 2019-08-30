package com.songlei.slplayer.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.songlei.slplayer.bean.PitchBean;
import com.songlei.slplayer.bean.SpeedBean;
import com.songlei.slplayer.listener.OnCompleteListener;
import com.songlei.slplayer.listener.OnErrorListener;
import com.songlei.slplayer.listener.OnTimeInfoListener;
import com.songlei.slplayer.player.Player;


/**
 * Created by songlei on 2019/03/12.
 */
public class PlayerView extends SurfaceView implements SurfaceHolder.Callback {
    private Player player;

    public PlayerView(Context context) {
        this(context, null);
    }

    public PlayerView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public PlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        player = new Player();
        getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Surface surface = holder.getSurface();
        player.init();
////        if (surface != null) {
////            player.initPlayer("rtmp://104.224.172.235/hls/test", surface);
////        }
//
//        String storage = Environment.getExternalStorageDirectory().getAbsolutePath();
////        String path = storage + "/ypi.mp4";
////        String path = storage + "/lansongBox/dy_xialu2.mp4";
//        String path = storage + "/input.mp4";
        if (surface != null) {
//            player.initPlayer(path, surface);
            player.setNativeSurface(surface);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        player.onSurfaceChange(width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

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
