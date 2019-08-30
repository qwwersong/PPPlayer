package com.songlei.slplayer.bean;

/**
 * Created by songlei on 2019/03/15.
 */
public enum SpeedBean {
    SPEED_1(1.0f),
    SPEED_1_2(1.2f),
    SPEED_1_5(1.5f),
    SPEED_2(2.0f);

    private float value;

    SpeedBean(float value) {
        this.value = value;
    }

    public float getValue() {
        return value;
    }
}
