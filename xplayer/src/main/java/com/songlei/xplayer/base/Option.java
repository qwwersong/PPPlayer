package com.songlei.xplayer.base;

import android.text.TextUtils;

import com.songlei.xplayer.bean.VideoModeBean;
import com.songlei.xplayer.view.PPVideoPlayerView;

import java.util.List;

/**
 * Created by songlei on 2019/07/02.
 */
public class Option {
    //===============================显示比例====================================
    //默认显示比例
    public final static int SCREEN_TYPE_DEFAULT = 0;

    //16:9
    public final static int SCREEN_TYPE_16_9 = 1;

    //4:3
    public final static int SCREEN_TYPE_4_3 = 2;

    //全屏裁减显示，为了显示正常 CoverImageView 建议使用FrameLayout作为父布局
    public final static int SCREEN_TYPE_FULL = 4;

    //全屏拉伸显示，使用这个属性时，surface_container建议使用FrameLayout
    public final static int SCREEN_MATCH_FULL = -4;

    //===============================渲染类型====================================
    //GLSurfaceView 主要用于OpenGL渲染的
    public final static int VIEW_GL_SURFACE = 2;

    //SurfaceView 与动画全屏的效果不是很兼容
    public final static int VIEW_SURFACE = 1;

    //TextureView 默认
    public final static int VIEW_TEXTURE = 0;
    //===============================播放内核====================================
    public static final int PLAYER_IJK = 0;             //IJKPlayer

    public static final int PLAYER_SYSTEM = 1;          //MediaPlayer

    public static final int PLAYER_OBSS = 2;            //ObssPlayer

    //===============================分辨率模式====================================
    public static final int TYPE_MODE_NORMAL = 0;         //原画模式

    public static final int TYPE_MODE_HIGH_CLEAR = 1;     //高清模式

    public static final int TYPE_MODE_SUPER_CLEAR = 2;    //超清模式


    private static int sShowType = SCREEN_TYPE_DEFAULT;
    private static int sPlayerType = PLAYER_IJK;
    private static int sRenderType = VIEW_TEXTURE;

    public static int getsShowType() {
        return sShowType;
    }

    public static void setsShowType(int type) {
        sShowType = type;
    }

    public static int getPlayerType(){
        return sPlayerType;
    }

    public static void setPlayerType(int playerType){
        sPlayerType = playerType;
    }

    public static int getRenderType() {
        return sRenderType;
    }

    public static void setRenderType(int renderType) {
        sRenderType = renderType;
    }

    public static class Builder {
        private String url;
        private List<VideoModeBean> urlList;
        private String title;
        private int renderType;
        private int playerType;

        //设置播放地址
        public Builder setUrl(String url){
            this.url = url;
            return this;
        }

        //设置多分辨率播放地址
        public Builder setUrlList(List<VideoModeBean> urlList){
            this.urlList = urlList;
            return this;
        }

        //设置标题
        public Builder setTitle(String title){
            this.title = title;
            return this;
        }

        //设置渲染类型
        public Builder setRenderType(int renderType){
            this.renderType = renderType;
            return this;
        }

        //设置播放内核
        public Builder setPlayerType(int playerType){
            this.playerType = playerType;
            return this;
        }

        public void build(PPVideoPlayerView videoPlayerView){
            if (!TextUtils.isEmpty(title)) {
                videoPlayerView.setTitle(title);
            }
            if (!TextUtils.isEmpty(url)) {
                videoPlayerView.setUp(url);
            }
            if (urlList != null && urlList.size() != 0) {
                videoPlayerView.setUp(urlList);
            }
            videoPlayerView.setRenderType(renderType);
            videoPlayerView.setPlayerType(playerType);
        }
    }

}
