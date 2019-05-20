package com.sausagetaste.littleruler;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;

import android.os.Environment;

import java.util.Objects;


public class ActivityJNI extends Activity {

    private ViewJNI mView;
    AssetManager mAssMan;

    @SuppressLint("ClickableViewAccessibility")
    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new ViewJNI(getApplication());
        setContentView(mView);

        this.mAssMan = getResources().getAssets();

        // What is this, mr. JetBrain?? requireNonNull??
        String filePath = Objects.requireNonNull(this.getExternalFilesDir(null)).getPath();
        LibJNI.giveRequirements(this.mAssMan, filePath);

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
