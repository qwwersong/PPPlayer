package com.songlei.xplayer.base;

/**
 * Created by songlei on 2019/07/02.
 */
public class Option {
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

    //显示比例类型
    private static int SHOW_TYPE = SCREEN_TYPE_DEFAULT;

    public static int getShowType() {
        return SHOW_TYPE;
    }

    /**
     * 设置显示比例,注意，这是全局生效的
     */
    public static void setShowType(int type) {
        SHOW_TYPE = type;
    }

    /**
     * GLSurfaceView 主要用于OpenGL渲染的
     */
    public final static int VIEW_GL_SURFACE = 2;

    /**
     * SurfaceView，与动画全屏的效果不是很兼容
     */
    public final static int VIEW_SURFACE = 1;

    /**
     * TextureView,默认
     */
    public final static int VIEW_TEXTURE = 0;

    //渲染类型
    private static int sRenderType = VIEW_TEXTURE;

    public static int getRenderType() {
        return sRenderType;
    }

    /**
     * 渲染控件
     */
    public static void setRenderType(int renderType) {
        sRenderType = renderType;
    }

    public static final int PLAYER_IJK = 0;

    public static final int PLAYER_SYSTEM = 1;

    public static final int PLAYER_OBSS = 2;

    private static int sPlayerType = PLAYER_IJK;

    public static int getPlayerType(){
        return sPlayerType;
    }

    public static void setPlayerType(int playerType){
        sPlayerType = playerType;
    }

    public static final int TYPE_MODE_NORMAL = 0;         //原画模式

    public static final int TYPE_MODE_HIGH_CLEAR = 1;     //高清模式

    public static final int TYPE_MODE_SUPER_CLEAR = 2;    //超清模式
}
