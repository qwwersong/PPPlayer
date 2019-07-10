package com.songlei.xplayer.view.render.listener;

import android.view.Surface;

/**
 * Surface 状态变化回调
 * Created by songlei on 2019/7/5.
 */

public interface ISurfaceListener {
    void onSurfaceAvailable(Surface surface);

    void onSurfaceSizeChanged(Surface surface, int width, int height);

    boolean onSurfaceDestroyed(Surface surface);

    void onSurfaceUpdated(Surface surface);
}
