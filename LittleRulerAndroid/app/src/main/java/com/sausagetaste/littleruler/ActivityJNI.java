package com.sausagetaste.littleruler;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import java.util.Objects;


public class ActivityJNI extends Activity {

    private ViewJNI mView;
    AssetManager mAssMan;

    TextInputBox m_textInputBox = null;

    @SuppressLint("ClickableViewAccessibility") @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new ViewJNI(getApplication());
        setContentView(mView);

        this.mAssMan = getResources().getAssets();

        // What is this, mr. JetBrain?? requireNonNull??
        String filePath = Objects.requireNonNull(this.getExternalFilesDir(null)).getPath();
        LibJNI.giveRequirements(this.mAssMan, filePath);

        mView.setOnTouchListener(new CallableJNI.MyListener(mView));

        this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);

        //getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        //getWindow().takeKeyEvents(true);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mView.onResume();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (hasFocus) {
            View decorView = getWindow().getDecorView();

            int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
            decorView.setSystemUiVisibility(uiOptions);
        }
    }

    private void openTextInput() {
        this.m_textInputBox = new TextInputBox(this);
        this.addContentView(this.m_textInputBox, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        this.m_textInputBox.requestFocus();
        InputMethodManager imm = (InputMethodManager) this.getSystemService(Activity.INPUT_METHOD_SERVICE);
        imm.toggleSoftInput(InputMethodManager.HIDE_IMPLICIT_ONLY, 0);
    }

    private void closeTextInput() {
        if (null == this.m_textInputBox) return;
        ((ViewGroup) this.m_textInputBox.getParent()).removeView(this.m_textInputBox);
    }

}
