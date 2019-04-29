package com.sausagetaste.littleruler;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;


public class ViewJNI extends GLSurfaceView {

    private Renderer mRenderer;

    public ViewJNI(Context context) {
        super(context);

        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(3);
        this.setPreserveEGLContextOnPause(true);

        this.mRenderer = new Renderer();
        setRenderer(mRenderer);
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            LibJNI.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            LibJNI.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            LibJNI.init();
        }
    }

}
