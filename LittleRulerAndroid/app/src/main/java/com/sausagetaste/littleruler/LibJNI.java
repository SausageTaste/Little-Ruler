package com.sausagetaste.littleruler;

import android.content.res.AssetManager;


public class LibJNI {

    static {
        System.loadLibrary("interface4jni");
    }

    public static native void init();
    public static native void resize(int width, int height);
    public static native void step();
    public static native void giveRequirements(AssetManager mgr, String sdcardPath);

}
