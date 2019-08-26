package com.songlei.basemodule;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import java.lang.ref.WeakReference;

public class CommonHandler extends Handler {
    private WeakReference<HandlerCallBack> tWeakReference;

    public CommonHandler(HandlerCallBack handlerCallBack) {
        this.tWeakReference = new WeakReference<HandlerCallBack>(handlerCallBack);
    }

    public CommonHandler(HandlerCallBack handlerCallBack, Looper looper) {
        super(looper);
        this.tWeakReference = new WeakReference<HandlerCallBack>(handlerCallBack);
    }

    @Override
    public void handleMessage(Message msg) {
        super.handleMessage(msg);
        HandlerCallBack handlerCallBack = tWeakReference.get();
        if (handlerCallBack != null) {
            handlerCallBack.handleMessage(msg);
        }
    }

    public interface HandlerCallBack {
        void handleMessage(Message msg);
    }
}