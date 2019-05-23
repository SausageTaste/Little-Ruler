package com.sausagetaste.littleruler;

import android.content.Context;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;


public class TextInputBox extends EditText {

    public TextInputBox(Context context) {
        super(context);
        this.setWidth(2000);
        this.setMaxLines(1);
        this.setInputType(EditorInfo.TYPE_CLASS_TEXT);
    }

}
