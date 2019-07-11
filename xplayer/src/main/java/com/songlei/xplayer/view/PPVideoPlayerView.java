package com.songlei.xplayer.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.View;
import android.widget.TextView;

import com.songlei.xplayer.R;
import com.songlei.xplayer.base.Option;

/**
 * 标准播放器布局，根据该布局自定义，id不可变
 * Created by songlei on 2019/07/02.
 */
public class PPVideoPlayerView extends PPOrientationView {
    //切换缩放模式
    private TextView mMoreScale;
    //记住切换数据源类型
    private int mShowType = 0;

    public PPVideoPlayerView(Context context) {
        super(context);
    }

    public PPVideoPlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PPVideoPlayerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected void init(Context context) {
        super.init(context);
        initView();
    }

    private void initView(){
        mMoreScale = findViewById(R.id.moreScale);

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

        mFullscreenButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIfCurrentIsFullScreen) {
                    showProt();
                } else {
                    showFull();
                }
            }
        });

        mBackButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mIfCurrentIsFullScreen) {
                    showProt();
                } else {
                    //退出activity
                }
            }
        });
    }

    public void setUp(String url){
        setUrl(url);
    }

    @Override
    protected void releaseSurface(Surface surface) {

    }

    @Override
    public int getLayoutId() {
        return R.layout.layout_player_base;
    }

    /**
     * 显示比例
     * 注意，GSYVideoType.setShowType是全局静态生效，除非重启APP。
     */
    private void resolveTypeUI() {
//        if (!mHadPlay) {
//            return;
//        }
        if (mShowType == 1) {
            mMoreScale.setText("16:9");
            Option.setShowType(Option.SCREEN_TYPE_16_9);
        } else if (mShowType == 2) {
            mMoreScale.setText("4:3");
            Option.setShowType(Option.SCREEN_TYPE_4_3);
        } else if (mShowType == 3) {
            mMoreScale.setText("全屏");
            Option.setShowType(Option.SCREEN_TYPE_FULL);
        } else if (mShowType == 4) {
            mMoreScale.setText("拉伸全屏");
            Option.setShowType(Option.SCREEN_MATCH_FULL);
        } else if (mShowType == 0) {
            mMoreScale.setText("默认比例");
            Option.setShowType(Option.SCREEN_TYPE_DEFAULT);
        }
        changeViewShowType();
        if (mTextureView != null)
            mTextureView.requestLayout();
    }

    public void showFull() {
        if (mOrientationUtil.getIsLand() != 1) {
            //直接横屏
            mOrientationUtil.resolveByClick();
        }
        onEnterFullScreen(mContext, true, true);
    }

    public void showProt(){
        if (mOrientationUtil != null) {
            mOrientationUtil.backToProtVideo();
        }
        onBackFullScreen();
    }

}
