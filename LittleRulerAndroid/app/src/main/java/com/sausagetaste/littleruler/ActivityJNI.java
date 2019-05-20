package com.sausagetaste.littleruler;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;

import android.os.Environment;


public class ActivityJNI extends Activity {

    private ViewJNI mView;
    AssetManager mAssMan;

    @SuppressLint("ClickableViewAccessibility")
    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new ViewJNI(getApplication());
        setContentView(mView);

        this.mAssMan = getResources().getAssets();
        //mView.giveRequirements(mgr, CallableJNI.sTouchEvents);
        LibJNI.giveRequirements(this.mAssMan, this.getExternalFilesDir(null).getPath());

        mView.setOnTouchListener(new CallableJNI.MyListener(mView));
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }

}
