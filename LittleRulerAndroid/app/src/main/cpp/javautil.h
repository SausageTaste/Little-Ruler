#pragma once

#include <jni.h>


namespace dal {

    void initJavautil(JavaVM* javaVM);
    JNIEnv* getJNIEnv(void);

    namespace touchinput {
        int getArraySize(void);
        int getCurrentIndexAndReset(void);
        jbyte* startAccess(void);
        void finishAccess(jbyte* array);
        void copyArray(jbyte* buf, int bufSize);
    }

}