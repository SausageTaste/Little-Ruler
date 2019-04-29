#pragma once

#include <jni.h>


class ContextJNI {

private:
	JavaVM* mJavaVM;

public:
	ContextJNI(void)
	:	mJavaVM(nullptr)
	{

	}

	void init(JavaVM* vm) {
		this->mJavaVM = vm;
	}

	JNIEnv* getJNIEnv(void) {
		JNIEnv* env;
		if (mJavaVM->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
			return nullptr;
		}
		else {
			return env;
		}
	}

};