package com.songlei.xplayer.view.render.listener;


import java.io.File;

/**
 * 截屏保存结果
 * Created by guoshuyu on 2017/9/21.
 */

public interface VideoShotSaveListener {
    void result(boolean success, File file);
}
