package com.songlei.slplayer.bean;

/**
 * Created by songlei on 2019/03/15.
 */
public enum PitchBean {
    PITCH_1(1.0f),
    PITCH_1_2(1.2f),
    PITCH_1_5(1.5f),
    PITCH_2(2.0f);

    private float value;

    PitchBean(float value) {
        this.value = value;
    }

    public float getValue() {
        return value;
    }
}
