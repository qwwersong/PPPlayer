package com.songlei.xplayer.view;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;
import com.songlei.xplayer.util.CommonUtil;
import com.songlei.xplayer.view.render.effect.AutoFixEffect;
import com.songlei.xplayer.view.render.effect.BarrelBlurEffect;
import com.songlei.xplayer.view.render.effect.BitmapIconEffect;
import com.songlei.xplayer.view.render.effect.BlackAndWhiteEffect;
import com.songlei.xplayer.view.render.effect.BrightnessEffect;
import com.songlei.xplayer.view.render.effect.ContrastEffect;
import com.songlei.xplayer.view.render.effect.CrossProcessEffect;
import com.songlei.xplayer.view.render.effect.DocumentaryEffect;
import com.songlei.xplayer.view.render.effect.DuotoneEffect;
import com.songlei.xplayer.view.render.effect.FillLightEffect;
import com.songlei.xplayer.view.render.effect.GammaEffect;
import com.songlei.xplayer.view.render.effect.GaussianBlurEffect;
import com.songlei.xplayer.view.render.effect.GrainEffect;
import com.songlei.xplayer.view.render.effect.HueEffect;
import com.songlei.xplayer.view.render.effect.InvertColorsEffect;
import com.songlei.xplayer.view.render.effect.LamoishEffect;
import com.songlei.xplayer.view.render.effect.NoEffect;
import com.songlei.xplayer.view.render.effect.OverlayEffect;
import com.songlei.xplayer.view.render.effect.PixelationEffect;
import com.songlei.xplayer.view.render.effect.PosterizeEffect;
import com.songlei.xplayer.view.render.effect.SampleBlurEffect;
import com.songlei.xplayer.view.render.effect.SaturationEffect;
import com.songlei.xplayer.view.render.effect.SepiaEffect;
import com.songlei.xplayer.view.render.effect.SharpnessEffect;
import com.songlei.xplayer.view.render.effect.TemperatureEffect;
import com.songlei.xplayer.view.render.effect.TintEffect;
import com.songlei.xplayer.view.render.effect.VideoGLViewCustomRender;
import com.songlei.xplayer.view.render.effect.VignetteEffect;
import com.songlei.xplayer.view.render.view.VideoGLView;

/**
 * Created by songlei on 2019/07/30.
 */
public class PPRenderView extends PPVideoPlayerView {
    //切换滤镜
    private TextView mSwitchEffect;
    //切换缩放模式
    private TextView mMoreScale;

    //切换渲染类型
    private int mShowType = 0;
    //滤镜类型
    private int mEffectType = 0;

    public PPRenderView(Context context) {
        super(context);
    }

    public PPRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PPRenderView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public int getLayoutId() {
        return R.layout.layout_player_render;
    }

    @Override
    protected void initView() {
        super.initView();
        mMoreScale = findViewById(R.id.moreScale);
        mSwitchEffect = findViewById(R.id.moreEffect);

        //水印图效果
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), R.mipmap.ic_launcher);
        VideoGLViewCustomRender mVideoGLViewCustomRender = new VideoGLViewCustomRender();
        BitmapIconEffect mCustomBitmapIconEffect = new BitmapIconEffect(bitmap, CommonUtil.dip2px(mContext,50), CommonUtil.dip2px(mContext,50), 0.6f);
        mVideoGLViewCustomRender.setBitmapEffect(mCustomBitmapIconEffect);
        setCustomGLRenderer(mVideoGLViewCustomRender);
        setGLRenderMode(VideoGLView.MODE_RENDER_SIZE);
    }

    @Override
    protected void initListener() {
        super.initListener();

        mMoreScale.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mShowType == 0) {
                    mShowType = 1;
                } else if (mShowType == 1) {
                    mShowType = 2;
                } else if (mShowType == 2) {
                    mShowType = 3;
                } else if (mShowType == 3) {
                    mShowType = 4;
                } else if (mShowType == 4) {
                    mShowType = 0;
                }
                resolveTypeUI();
            }
        });

        mSwitchEffect.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                switchEffect();
            }
        });
    }

    /**
     * 显示比例
     * 注意，VideoType.setShowType是全局静态生效，除非重启APP。
     */
    private void resolveTypeUI() {
        if (mShowType == 1) {
            mMoreScale.setText("16:9");
            Option.setsShowType(Option.SCREEN_TYPE_16_9);
        } else if (mShowType == 2) {
            mMoreScale.setText("4:3");
            Option.setsShowType(Option.SCREEN_TYPE_4_3);
        } else if (mShowType == 3) {
            mMoreScale.setText("全屏");
            Option.setsShowType(Option.SCREEN_TYPE_FULL);
        } else if (mShowType == 4) {
            mMoreScale.setText("拉伸全屏");
            Option.setsShowType(Option.SCREEN_MATCH_FULL);
        } else if (mShowType == 0) {
            mMoreScale.setText("默认比例");
            Option.setsShowType(Option.SCREEN_TYPE_DEFAULT);
        }
        changeViewShowType();
        if (mTextureView != null)
            mTextureView.requestLayout();
    }

    /**
     * 切换滤镜
     */
    private void switchEffect() {
        Log.e("xxx", "switchEffect mEffectType = " + mEffectType);
        VideoGLView.ShaderInterface effect = new NoEffect();
        float deep = 0.8f;
        switch (mEffectType) {
            case 0:
                effect = new AutoFixEffect(deep);
                break;
            case 1:
                effect = new PixelationEffect();
                break;
            case 2:
                effect = new BlackAndWhiteEffect();
                break;
            case 3:
                effect = new ContrastEffect(deep);
                break;
            case 4:
                effect = new CrossProcessEffect();
                break;
            case 5:
                effect = new DocumentaryEffect();
                break;
            case 6:
                effect = new DuotoneEffect(Color.BLUE, Color.YELLOW);
                break;
            case 7:
                effect = new FillLightEffect(deep);
                break;
            case 8:
                effect = new GammaEffect(deep);
                break;
            case 9:
                effect = new GrainEffect(deep);
                break;
            case 10:
                effect = new GrainEffect(deep);
                break;
            case 11:
                effect = new HueEffect(deep);
                break;
            case 12:
                effect = new InvertColorsEffect();
                break;
            case 13:
                effect = new LamoishEffect();
                break;
            case 14:
                effect = new PosterizeEffect();
                break;
            case 15:
                effect = new BarrelBlurEffect();
                break;
            case 16:
                effect = new SaturationEffect(deep);
                break;
            case 17:
                effect = new SepiaEffect();
                break;
            case 18:
                effect = new SharpnessEffect(deep);
                break;
            case 19:
                effect = new TemperatureEffect(deep);
                break;
            case 20:
                effect = new TintEffect(Color.GREEN);
                break;
            case 21:
                effect = new VignetteEffect(deep);
                break;
            case 22:
                effect = new NoEffect();
                break;
            case 23:
                effect = new OverlayEffect();
                break;
            case 24:
                effect = new SampleBlurEffect(4.0f);
                break;
            case 25:
                effect = new GaussianBlurEffect(6.0f, GaussianBlurEffect.TYPEXY);
                break;
            case 26:
                effect = new BrightnessEffect(deep);
                break;
        }
        setEffectFilter(effect);
        mEffectType++;
        if (mEffectType > 25) {
            mEffectType = 0;
        }
    }
}
