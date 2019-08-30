package com.songlei.slplayer.player;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import com.songlei.slplayer.listener.OnCompleteListener;
import com.songlei.slplayer.listener.OnErrorListener;
import com.songlei.slplayer.listener.OnPrepareListener;
import com.songlei.slplayer.listener.OnTimeInfoListener;
import com.songlei.slplayer.util.MediaDecodeUtil;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by songlei on 2019/02/25.
 */
public class Player {
    private OnTimeInfoListener onTimeInfoListener;
    private OnCompleteListener onCompleteListener;
    private OnErrorListener onErrorListener;
    private OnPrepareListener onPrepareListener;

    private int volume = 100;
    private int channelType = 2;

    private int currentTime;
    private int duration;

    //硬解码MediaCodec
    private MediaCodec mediaCodec;
    private MediaCodec.BufferInfo bufferInfo;
    private Surface surface;

    static {
        System.loadLibrary("Player");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avdevice-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
        System.loadLibrary("postproc-54");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
    }

    public void setOnPrepareListener(OnPrepareListener onPrepareListener){
        this.onPrepareListener = onPrepareListener;
    }

    public void setOnTimeInfoListener(OnTimeInfoListener onTimeInfoListener) {
        this.onTimeInfoListener = onTimeInfoListener;
    }

    public void setOnCompleteListener(OnCompleteListener onCompleteListener) {
        this.onCompleteListener = onCompleteListener;
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setNativeSurface(Surface surface){
        Log.e("xx", "setNativeSurface surface = " + surface);
        this.surface = surface;
        setSurface(surface);
    }

//    public void initPlayer(String url, Surface surface){
//        if (!TextUtils.isEmpty(url) && surface != null) {
//            init(url, surface);
//        }
//    }

    public void startPlay(){
        seekVolume(volume);
        setChannelMute(channelType);
        start();
    }

    public int getVolume() {
        return volume;
    }

    public int getCurrentTime(){
        return currentTime;
    }

    public int getDuration(){
        return duration;
    }

    //native
    public native void onSurfaceChange(int width, int height);

    private native void setSurface(Surface surface);

    public native void init();

    public native void prepare(String url);

    private native void start();

    public native void pause();

    public native void resume();

    public native void stop();//停止播放，释放资源

    public native void seek(int sec);

    public native void seekVolume(int volume);

    public native void setChannelMute(int type);

    public native void setPitch(float pitch);

    public native void setSpeed(float speed);

    //C++回调java方法
    public void onCallTimeInfo(int currentTime, int totalTime){
        this.currentTime = currentTime;
        this.duration = totalTime;
        if (onTimeInfoListener != null) {
            onTimeInfoListener.onTimeInfo(currentTime, totalTime);
        }
    }

    public void onCallError(int code, String msg){
        if (onErrorListener != null) {
            onErrorListener.onError(code, msg);
        }
    }

    public void onCallComplete(){
        if (onCompleteListener != null) {
            onCompleteListener.onComplete();
        }
    }

    public void onCallPrepare(){
        if (onPrepareListener != null) {
            onPrepareListener.onPrepare();
        }
    }

    public boolean onCallSupportMediaDecode(String codecName){
        return MediaDecodeUtil.isSupportDecoder(codecName);
    }

    public void onCallInitMediaCodec(String codecName, int width, int height, byte[] csd_0, byte[] csd_1){
        if (surface != null) {
            try {
                String mime = MediaDecodeUtil.getMediaDecodeName(codecName);
                MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, width, height);
                mediaFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);//TODO::为什么是width * height
                mediaFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd_0));
                mediaFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd_1));
                Log.e("xx", "mediaFormat = " + mediaFormat.toString());

                mediaCodec = MediaCodec.createDecoderByType(mime);
                bufferInfo = new MediaCodec.BufferInfo();
                //surface在这里关联EGL环境，不能在底层关联，要不然会报错
                mediaCodec.configure(mediaFormat, surface, null, 0);
                mediaCodec.start();
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            if (onErrorListener != null) {
                onErrorListener.onError(2001, "init mediaCodec surface is null");
            }
        }
    }

    public void onDecodecAVPacket(int data_size, byte[] data){
        if (surface != null && data_size > 0 && data != null && mediaCodec != null) {
            //TODO::这个inputBufferIndex是什么
            //将data数据放到MediaCodec入队队列，进行硬解码
            int inputBufferIndex = mediaCodec.dequeueInputBuffer(10);//超时时间
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = mediaCodec.getInputBuffers()[inputBufferIndex];
                byteBuffer.clear();
                byteBuffer.put(data);
                mediaCodec.queueInputBuffer(inputBufferIndex, 0, data_size, 0, 0);
            }

            //将硬解码后的数据直接绘制到surface上
            int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10);
            while (outputBufferIndex >= 0) {
                //render设置为true，表示解码后的数据直接绘制到surface上
                mediaCodec.releaseOutputBuffer(outputBufferIndex, true);
                outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10);
            }
        }
    }

    private void releaseMediaCodec(){
        if (mediaCodec != null) {
            mediaCodec.flush();
            mediaCodec.stop();
            mediaCodec.release();
        }
        mediaCodec = null;
        bufferInfo = null;
    }

}
