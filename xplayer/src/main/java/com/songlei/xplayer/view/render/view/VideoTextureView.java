package com.songlei.xplayer.view.render.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;

import com.songlei.xplayer.util.MeasureHelper;
import com.songlei.xplayer.view.render.IRenderView;
import com.songlei.xplayer.view.render.RenderView;
import com.songlei.xplayer.view.render.glrender.VideoGLViewBaseRender;
import com.songlei.xplayer.view.render.listener.ISurfaceListener;
import com.songlei.xplayer.view.render.listener.VideoShotListener;
import com.songlei.xplayer.view.render.listener.VideoShotSaveListener;

import java.io.File;

/**
 * 用于显示video的，做了横屏与竖屏的匹配，还有需要rotation需求的
 * Created by shuyu on 2016/11/11.
 */

public class VideoTextureView extends TextureView implements TextureView.SurfaceTextureListener, IRenderView, MeasureHelper.MeasureFormVideoParamsListener {

    private ISurfaceListener mIGSYSurfaceListener;

    private MeasureHelper.MeasureFormVideoParamsListener mVideoParamsListener;

    private MeasureHelper measureHelper;

    private Surface mSurface;

    public VideoTextureView(Context context) {
        super(context);
        init();
    }

    public VideoTextureView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        measureHelper = new MeasureHelper(this, this);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        measureHelper.prepareMeasure(widthMeasureSpec, heightMeasureSpec, (int) getRotation());
        setMeasuredDimension(measureHelper.getMeasuredWidth(), measureHelper.getMeasuredHeight());
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mSurface = new Surface(surface);
        if (mIGSYSurfaceListener != null) {
            mIGSYSurfaceListener.onSurfaceAvailable(mSurface);
        }
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        if (mIGSYSurfaceListener != null) {
            mIGSYSurfaceListener.onSurfaceSizeChanged(mSurface, width, height);
        }
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        //清空释放
        if (mIGSYSurfaceListener != null) {
            mIGSYSurfaceListener.onSurfaceDestroyed(mSurface);
        }
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        //如果播放的是暂停全屏了
        if (mIGSYSurfaceListener != null) {
            mIGSYSurfaceListener.onSurfaceUpdated(mSurface);
        }
    }

    @Override
    public ISurfaceListener getIGSYSurfaceListener() {
        return mIGSYSurfaceListener;
    }

    @Override
    public void setISurfaceListener(ISurfaceListener surfaceListener) {
        setSurfaceTextureListener(this);
        mIGSYSurfaceListener = surfaceListener;
    }

    @Override
    public int getSizeH() {
        return getHeight();
    }

    @Override
    public int getSizeW() {
        return getWidth();
    }

    /**
     * 暂停时初始化位图
     */
    @Override
    public Bitmap initCover() {
        Bitmap bitmap = Bitmap.createBitmap(
                getSizeW(), getSizeH(), Bitmap.Config.RGB_565);
        return getBitmap(bitmap);

    }

    /**
     * 暂停时初始化位图
     */
    @Override
    public Bitmap initCoverHigh() {
        Bitmap bitmap = Bitmap.createBitmap(
                getSizeW(), getSizeH(), Bitmap.Config.ARGB_8888);
        return getBitmap(bitmap);

    }


    /**
     * 获取截图
     *
     * @param shotHigh 是否需要高清的
     */
    @Override
    public void taskShotPic(VideoShotListener gsyVideoShotListener, boolean shotHigh) {
        if (shotHigh) {
            gsyVideoShotListener.getBitmap(initCoverHigh());
        } else {
            gsyVideoShotListener.getBitmap(initCover());
        }
    }

    /**
     * 保存截图
     *
     * @param high 是否需要高清的
     */
    @Override
    public void saveFrame(final File file, final boolean high, final VideoShotSaveListener gsyVideoShotSaveListener) {
        VideoShotListener gsyVideoShotListener = new VideoShotListener() {
            @Override
            public void getBitmap(Bitmap bitmap) {
                if (bitmap == null) {
                    gsyVideoShotSaveListener.result(false, file);
                } else {
//                    FileUtils.saveBitmap(bitmap, file);
                    gsyVideoShotSaveListener.result(true, file);
                }
            }
        };
        if (high) {
            gsyVideoShotListener.getBitmap(initCoverHigh());
        } else {
            gsyVideoShotListener.getBitmap(initCover());
        }

    }


    @Override
    public View getRenderView() {
        return this;
    }

    @Override
    public void onRenderResume() {
//        Debuger.printfLog(getClass().getSimpleName() + " not support onRenderResume now");
    }

    @Override
    public void onRenderPause() {
//        Debuger.printfLog(getClass().getSimpleName() + " not support onRenderPause now");
    }

    @Override
    public void releaseRenderAll() {
//        Debuger.printfLog(getClass().getSimpleName() + " not support releaseRenderAll now");
    }

    @Override
    public void setRenderMode(int mode) {
//        Debuger.printfLog(getClass().getSimpleName() + " not support setRenderMode now");
    }

    @Override
    public void setRenderTransform(Matrix transform) {
        setTransform(transform);
    }

    @Override
    public void setGLRenderer(VideoGLViewBaseRender renderer) {
//        Debuger.printfLog(getClass().getSimpleName() + " not support setGLRenderer now");
    }

    @Override
    public void setGLMVPMatrix(float[] MVPMatrix) {
//        Debuger.printfLog(getClass().getSimpleName() + " not support setGLMVPMatrix now");
    }

    /**
     * 设置滤镜效果
     */
    @Override
    public void setGLEffectFilter(VideoGLView.ShaderInterface effectFilter) {
//        Debuger.printfLog(getClass().getSimpleName() + " not support setGLEffectFilter now");
    }


    @Override
    public void setVideoParamsListener(MeasureHelper.MeasureFormVideoParamsListener listener) {
        mVideoParamsListener = listener;
        Log.e("xxx", "setVideoParamsListener mVideoParamsListener = " + mVideoParamsListener);
    }

    @Override
    public int getCurrentVideoWidth() {
        Log.e("xxx", "getCurrentVideoWidth mVideoParamsListener = " + mVideoParamsListener);
        if (mVideoParamsListener != null) {
            Log.e("xxx", "mVideoParamsListener.getCurrentVideoWidth() = " + mVideoParamsListener.getCurrentVideoWidth());
            return mVideoParamsListener.getCurrentVideoWidth();
        }
        return 0;
    }

    @Override
    public int getCurrentVideoHeight() {
        if (mVideoParamsListener != null) {
            return mVideoParamsListener.getCurrentVideoHeight();
        }
        return 0;
    }

    @Override
    public int getVideoSarNum() {
        if (mVideoParamsListener != null) {
            return mVideoParamsListener.getVideoSarNum();
        }
        return 0;
    }

    @Override
    public int getVideoSarDen() {
        if (mVideoParamsListener != null) {
            return mVideoParamsListener.getVideoSarDen();
        }
        return 0;
    }


    /**
     * 添加播放的view
     */
    public static VideoTextureView addTextureView(Context context, ViewGroup textureViewContainer, int rotate,
                                                final ISurfaceListener surfaceListener,
                                                final MeasureHelper.MeasureFormVideoParamsListener videoParamsListener) {
        if (textureViewContainer.getChildCount() > 0) {
            textureViewContainer.removeAllViews();
        }
        Log.e("xxx", "addTextureView mVideoParamsListener = " + videoParamsListener);
        VideoTextureView videoTextureView = new VideoTextureView(context);
        videoTextureView.setISurfaceListener(surfaceListener);
        videoTextureView.setVideoParamsListener(videoParamsListener);
        videoTextureView.setRotation(rotate);
        RenderView.addToParent(textureViewContainer, videoTextureView);
        return videoTextureView;
    }
}