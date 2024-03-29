package com.songlei.xplayer.view.render;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;

import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.util.MeasureHelper;
import com.songlei.xplayer.view.render.glrender.VideoGLViewBaseRender;
import com.songlei.xplayer.view.render.listener.ISurfaceListener;
import com.songlei.xplayer.view.render.listener.VideoShotListener;
import com.songlei.xplayer.view.render.listener.VideoShotSaveListener;
import com.songlei.xplayer.view.render.view.VideoGLView;
import com.songlei.xplayer.view.render.view.VideoSurfaceView;
import com.songlei.xplayer.view.render.view.VideoTextureView;

import java.io.File;

/**
 * render绘制中间控件
 * Created by songlei on 2019/7/5.
 */

public class RenderView {

    protected IRenderView mShowView;

    public void requestLayout() {
        if (mShowView != null) {
            mShowView.getRenderView().requestLayout();
        }
    }

    public float getRotation() {
        return mShowView.getRenderView().getRotation();
    }

    public void setRotation(float rotation) {
        if (mShowView != null)
            mShowView.getRenderView().setRotation(rotation);
    }

    public void invalidate() {
        if (mShowView != null)
            mShowView.getRenderView().invalidate();
    }

    public int getWidth() {
        return (mShowView != null) ? mShowView.getRenderView().getWidth() : 0;
    }

    public int getHeight() {
        return (mShowView != null) ? mShowView.getRenderView().getHeight() : 0;
    }

    public View getShowView() {
        if (mShowView != null)
            return mShowView.getRenderView();
        return null;
    }

    public ViewGroup.LayoutParams getLayoutParams() {
        return mShowView.getRenderView().getLayoutParams();
    }

    public void setLayoutParams(ViewGroup.LayoutParams layoutParams) {
        if (mShowView != null)
            mShowView.getRenderView().setLayoutParams(layoutParams);
    }

    /**
     * 添加播放的view
     */
    public void addView(final Context context, final ViewGroup textureViewContainer, final int rotate,
                        final ISurfaceListener surfaceListener,
                        final MeasureHelper.MeasureFormVideoParamsListener videoParamsListener,
                        final VideoGLView.ShaderInterface effect, final float[] transform,
                        final VideoGLViewBaseRender customRender, int mode) {
        Log.e("xxx", "RenderView addView renderType = " + Option.getRenderType());
        if (Option.getRenderType() == Option.VIEW_SURFACE) {
            mShowView = VideoSurfaceView.addSurfaceView(context, textureViewContainer, rotate, surfaceListener, videoParamsListener);
        } else if (Option.getRenderType() == Option.VIEW_GL_SURFACE) {
            mShowView = VideoGLView.addGLView(context, textureViewContainer, rotate, surfaceListener, videoParamsListener, effect, transform, customRender, mode);
        } else {
            mShowView = VideoTextureView.addTextureView(context, textureViewContainer, rotate, surfaceListener, videoParamsListener);
        }
    }

    /*************************RenderView function end *************************/

    /*************************ShowView function start *************************/

    /**
     * 主要针对TextureView，设置旋转
     */
    public void setTransform(Matrix transform) {
        if (mShowView != null)
            mShowView.setRenderTransform(transform);
    }

    /**
     * 暂停时初始化位图
     */
    public Bitmap initCover() {
        if (mShowView != null)
            return mShowView.initCover();
        return null;
    }

    /**
     * 暂停时初始化位图
     */
    public Bitmap initCoverHigh() {
        if (mShowView != null)
            return mShowView.initCoverHigh();
        return null;
    }

    /**
     * 获取截图
     */
    public void taskShotPic(VideoShotListener videoShotListener) {
        this.taskShotPic(videoShotListener, false);
    }


    /**
     * 获取截图
     *
     * @param shotHigh 是否需要高清的
     */
    public void taskShotPic(VideoShotListener videoShotListener, boolean shotHigh) {
        if (mShowView != null)
            mShowView.taskShotPic(videoShotListener, shotHigh);
    }

    /**
     * 保存截图
     */
    public void saveFrame(final File file, VideoShotSaveListener videoShotSaveListener) {
        saveFrame(file, false, videoShotSaveListener);
    }

    /**
     * 保存截图
     *
     * @param high 是否需要高清的
     */
    public void saveFrame(final File file, final boolean high, final VideoShotSaveListener videoShotSaveListener) {
        if (mShowView != null)
            mShowView.saveFrame(file, high, videoShotSaveListener);
    }

    /**
     * 主要针对GL
     */
    public void onResume() {
        if (mShowView != null)
            mShowView.onRenderResume();
    }

    /**
     * 主要针对GL
     */
    public void onPause() {
        if (mShowView != null)
            mShowView.onRenderPause();
    }

    /**
     * 主要针对GL
     */
    public void releaseAll() {
        if (mShowView != null)
            mShowView.releaseRenderAll();
    }

    /**
     * 主要针对GL
     */
    public void setGLRenderMode(int mode) {
        if (mShowView != null)
            mShowView.setRenderMode(mode);
    }

    /**
     * 自定义GL的渲染render
     */
    public void setGLRenderer(VideoGLViewBaseRender renderer) {
        if (mShowView != null)
            mShowView.setGLRenderer(renderer);
    }

    /**
     * GL模式下的画面matrix效果
     *
     * @param matrixGL 16位长度
     */
    public void setMatrixGL(float[] matrixGL) {
        if (mShowView != null)
            mShowView.setGLMVPMatrix(matrixGL);
    }

    /**
     * 设置滤镜效果
     */
    public void setEffectFilter(VideoGLView.ShaderInterface effectFilter) {
        if (mShowView != null)
            mShowView.setGLEffectFilter(effectFilter);
    }


    /*************************ShowView function end *************************/


    /*************************common function *************************/

    public static void addToParent(ViewGroup textureViewContainer, View render) {
        int params = getTextureParams();
        if (textureViewContainer instanceof RelativeLayout) {
            RelativeLayout.LayoutParams layoutParams = new RelativeLayout.LayoutParams(params, params);
            layoutParams.addRule(RelativeLayout.CENTER_IN_PARENT);
            textureViewContainer.addView(render, layoutParams);
        } else if (textureViewContainer instanceof FrameLayout) {
            FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(params, params);
            layoutParams.gravity = Gravity.CENTER;
            textureViewContainer.addView(render, layoutParams);
        }
    }

    /**
     * 获取布局参数
     *
     * @return
     */
    public static int getTextureParams() {
        boolean typeChanged = (Option.getsShowType() != Option.SCREEN_TYPE_DEFAULT);
        return (typeChanged) ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.MATCH_PARENT;
    }

}
