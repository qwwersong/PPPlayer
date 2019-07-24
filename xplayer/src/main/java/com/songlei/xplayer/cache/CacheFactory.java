package com.songlei.xplayer.cache;

/**
 * 缓存到本地服务工厂
 * Created by songlei on 2019/07/24.
 */
public class CacheFactory {

    private static Class<? extends ICacheManager> sICacheManager;

    public static void setCacheManager(Class<? extends ICacheManager>  cacheManager) {
        sICacheManager = cacheManager;
    }

    public static ICacheManager getCacheManager() {
        if (sICacheManager == null) {
//            sICacheManager = ProxyCacheManager.class;
        }
        try {
            return sICacheManager.newInstance();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
        return null;
    }
}
