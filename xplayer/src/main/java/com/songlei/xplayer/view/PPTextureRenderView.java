package com.songlei.xplayer.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.util.MeasureHelper;
import com.songlei.xplayer.view.render.RenderView;
import com.songlei.xplayer.view.render.effect.NoEffect;
import com.songlei.xplayer.view.render.glrender.VideoGLViewBaseRender;
import com.songlei.xplayer.view.render.listener.ISurfaceListener;
import com.songlei.xplayer.view.render.view.VideoGLView;

/**
 * Created by songlei on 2019/07/02.
 */
public abstract class PPTextureRenderView extends FrameLayout implements ISurfaceListener, MeasureHelper.MeasureFormVideoParamsListener {
    //native绘制
    protected Surface mSurface;

    //渲染控件
    protected RenderView mTextureView;

    //渲染控件父类
    protected ViewGroup mTextureViewContainer;

    //满屏填充暂停为徒
    protected Bitmap mFullPauseBitmap;

    //GL的滤镜
    protected VideoGLView.ShaderInterface mEffectFilter = new NoEffect();

    //GL的自定义渲染
    protected VideoGLViewBaseRender mRenderer;

    //GL的角度
    protected float[] mMatrixGL = null;

    //画面选择角度
    protected int mRotate;

    //GL的布局模式
    protected int mMode = VideoGLView.MODE_LAYOUT_SIZE;

    public PPTextureRenderView(Context context) {
        super(context);
    }

    public PPTextureRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PPTextureRenderView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    /**
     * 添加播放的view
     * 继承后重载addTextureView，继承RenderView后实现自己的IRenderView类，既可以使用自己自定义的显示层
     */
    protected void addTextureView() {
        Log.e("xxx", "addTextureView");
        mTextureView = new RenderView();
        mTextureView.addView(getContext(), mTextureViewContainer, mRotate, this,
                this, mEffectFilter, mMatrixGL, mRenderer, mMode);
    }

    @Override
    public void onSurfaceAvailable(Surface surface) {
        Log.e("xxx", "onSurfaceAvailable");
        setDisplay(surface);
    }

    @Override
    public void onSurfaceSizeChanged(Surface surface, int width, int height) {
        Log.e("xxx", "onSurfaceSizeChanged");
        surfaceChanged(surface);
    }

    @Override
    public boolean onSurfaceDestroyed(Surface surface) {
        Log.e("xxx", "onSurfaceDestroyed");
        //清空释放
        setDisplay(null);
        //同一消息队列中去release
        releaseSurface(surface);
        return true;
    }

    @Override
    public void onSurfaceUpdated(Surface surface) {

    }

    public void setEffectFilter(VideoGLView.ShaderInterface effectFilter){
        Log.e("xxx", "setEffectFilter");
        this.mEffectFilter = effectFilter;
        if (mTextureView != null) {
            mTextureView.setEffectFilter(effectFilter);
        }
    }

    /**
     * GL模式下的画面matrix效果
     * @param matrixGL 16位长度
     */
    public void setMatrixGL(float[] matrixGL) {
        this.mMatrixGL = matrixGL;
        if (mTextureView != null) {
            mTextureView.setMatrixGL(mMatrixGL);
        }
    }

    /**
     * 自定义GL的渲染render
     */
    public void setCustomGLRenderer(VideoGLViewBaseRender renderer) {
        this.mRenderer = renderer;
        if (mTextureView != null) {
            mTextureView.setGLRenderer(renderer);
        }
    }

    /**
     * GL布局的绘制模式，利用布局计算大小还是使用render计算大小
     *
     * @param mode MODE_LAYOUT_SIZE = 0,  MODE_RENDER_SIZE = 1
     */
    public void setGLRenderMode(int mode) {
        mMode = mode;
        if (mTextureView != null) {
            mTextureView.setGLRenderMode(mode);
        }
    }

    protected void changeViewShowType(){
        if (mTextureView != null) {
            boolean typeChanged = (Option.getsShowType() != Option.SCREEN_TYPE_DEFAULT);
            int params =  (typeChanged) ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.MATCH_PARENT;
            ViewGroup.LayoutParams layoutParams = mTextureView.getLayoutParams();
            layoutParams.width = params;
            layoutParams.height = params;
            mTextureView.setLayoutParams(layoutParams);
        }
    }

    //设置播放
    protected abstract void setDisplay(Surface surface);

    //释放
    protected abstract void releaseSurface(Surface surface);

    //绘制大小改变
    protected abstract void surfaceChanged(Surface surface);

}
