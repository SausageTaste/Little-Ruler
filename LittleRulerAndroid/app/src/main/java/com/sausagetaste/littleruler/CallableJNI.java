package com.sausagetaste.littleruler;


import android.annotation.SuppressLint;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.view.MotionEvent;
import android.view.View;

import java.nio.ByteBuffer;


public class CallableJNI {

    static public class MyListener implements View.OnTouchListener {
        private GLSurfaceView mView;

        MyListener(GLSurfaceView view) {
            this.mView = view;
        }

        @SuppressLint("ClickableViewAccessibility")
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            if (sArraySize - sCurrentIndex < sSizeOfOneDataBlock) {
                System.out.println("OnTouch queue size exceeded 100 so new ones discarded.");
                return true;
            }

            int numPointer = event.getPointerCount();
            int pointer = (event.getAction() & MotionEvent.ACTION_POINTER_ID_MASK) >> MotionEvent.ACTION_POINTER_ID_SHIFT;

            switch (event.getAction() & MotionEvent.ACTION_MASK) {
                case MotionEvent.ACTION_DOWN:  // Type is 1
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getRawX());
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getRawY());
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, 1);
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, event.getPointerId(0));
                    break;
                case MotionEvent.ACTION_POINTER_DOWN:
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getX(pointer));
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getY(pointer));
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, 1);
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, event.getPointerId(pointer));
                    break;
                case MotionEvent.ACTION_MOVE:  // Type is 2
                    for (int i = 0; i < numPointer; i++) {
                        sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getX(i));
                        sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getY(i));
                        sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, 2);
                        sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, event.getPointerId(i));
                    }
                    break;
                case MotionEvent.ACTION_UP:  // Type is 3
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getRawX());
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getRawY());
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, 3);
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, event.getPointerId(0));
                    break;
                case MotionEvent.ACTION_POINTER_UP:  // Type is 3
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getX(pointer));
                    sCurrentIndex = insertFloatToBytearray(sTouchEvents, sCurrentIndex, event.getY(pointer));
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, 3);
                    sCurrentIndex = insertIntToBytearray(sTouchEvents, sCurrentIndex, event.getPointerId(pointer));
                    break;
            }

            return true;
        }

        static private int insertFloatToBytearray(byte[] arr, int index, float v) {
            // Returns next empty index

            byte[] byteValue = ByteBuffer.allocate(4).putFloat(v).array();
            arr[index    ] = byteValue[0];
            arr[index + 1] = byteValue[1];
            arr[index + 2] = byteValue[2];
            arr[index + 3] = byteValue[3];
            return index + 4;
        }

        static private int insertIntToBytearray(byte[] arr, int index, int v) {
            // Returns next empty index

            byte[] byteValue = ByteBuffer.allocate(4).putInt(v).array();
            arr[index    ] = byteValue[0];
            arr[index + 1] = byteValue[1];
            arr[index + 2] = byteValue[2];
            arr[index + 3] = byteValue[3];
            return index + 4;
        }

    }


    static private int sCurrentIndex = 0;
    static private int sSizeOfOneDataBlock = 4*4;
    static private int sArraySize = sSizeOfOneDataBlock * 100;
    static private byte[] sTouchEvents = new byte[sArraySize];

    static public int getSome() {
        return 44;
    }

    static public String getBuildVersion() {
        return Build.VERSION.RELEASE;
    }

    static public long getRuntimeMemorySize() {
        return Runtime.getRuntime().freeMemory();
    }

}
