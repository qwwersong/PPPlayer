package com.songlei.xplayer.cache;

import android.content.Context;

import com.songlei.xplayer.player.IPlayer;

import java.io.File;
import java.util.Map;

/**
 * 缓存管理接口
 * Created by songlei on 2019/07/24.
 */
public interface ICacheManager {

    /**
     * 开始缓存逻辑
     *
     * @param mediaPlayer 播放内核
     * @param url         播放url
     * @param header      头部信息
     * @param cachePath   缓存路径，可以为空
     */
    void doCacheLogic(Context context, IPlayer mediaPlayer, String url, Map<String, String> header, File cachePath);

    /**
     * 清除缓存
     *
     * @param cachePath 可以为空，空时用默认
     * @param url       可以为空，空时清除所有
     */
    void clearCache(Context context, File cachePath, String url);

    /**
     * 是否缓存管理
     */
    void release();

    /**
     * 播放中判断是否缓存，会频繁调用
     */
    boolean hadCached();

    /**
     * 播放前判断是否缓存
     */
    boolean cachePreview(Context context, File cacheDir, String url);

    void setCacheAvailableListener(ICacheAvailableListener cacheAvailableListener);

    /**
     * 缓存进度接口
     */
    interface ICacheAvailableListener {
        void onCacheAvailable(File cacheFile, String url, int percentsAvailable);
    }

}
