#pragma once

#include <jni.h>

#include <s_logger_god.h>

#include "ContextJNI.h"


class TouchInputArray {

private:
	ContextJNI* mCnxtJNI;

public:
	TouchInputArray(void) {

	}

	void init(ContextJNI* cnxtJNI) {
		this->mCnxtJNI = cnxtJNI;
	}

	int getArraySize(void) {
		auto env = mCnxtJNI->getJNIEnv();
		return (int)env->GetStaticIntField(findClass_CallableJNI(), getFieldID_sArraySize());
	}

	int getCurrentIndexAndReset(void) {
		auto env = mCnxtJNI->getJNIEnv();
		auto currentIndex = int( env->GetStaticIntField(findClass_CallableJNI(), getFieldID_sCurrentIndex()) );

		env->SetStaticIntField(findClass_CallableJNI(), getFieldID_sCurrentIndex(), 0);
		return currentIndex;
	}

	jbyte* startAccess(void) {
		auto env = mCnxtJNI->getJNIEnv();
		return env->GetByteArrayElements(getArrayObject_sTouchEvents(), nullptr);
	}

	void finishAccess(jbyte* array) {
		auto env = mCnxtJNI->getJNIEnv();
		env->ReleaseByteArrayElements(getArrayObject_sTouchEvents(), array, 0);
	}

	void copyArray(jbyte* buf, int bufSize) {
		auto env = mCnxtJNI->getJNIEnv();
		env->GetByteArrayRegion(getArrayObject_sTouchEvents(), 0, (jsize)bufSize, buf);
	}

private:
	jbyteArray getArrayObject_sTouchEvents(void) {
		auto env = mCnxtJNI->getJNIEnv();
		auto logger = dal::LoggerGod::getinst();

		auto fieldID = env->GetStaticFieldID(findClass_CallableJNI(), "sTouchEvents", "[B");
		if (fieldID == nullptr) {
			logger.putFatal("Failed to find static field CallableJNI::sTouchEvents", __LINE__, __func__, __FILE__);
			abort();
		}

		return static_cast<jbyteArray>(env->GetStaticObjectField(findClass_CallableJNI(), fieldID));
	}

	jfieldID getFieldID_sCurrentIndex(void) {
		auto env = mCnxtJNI->getJNIEnv();
		auto logger = dal::LoggerGod::getinst();

		auto fieldID = env->GetStaticFieldID(findClass_CallableJNI(), "sCurrentIndex", "I");
		if (fieldID == nullptr) {
			logger.putFatal("Failed to find static field CallableJNI::sCurrentIndex", __LINE__, __func__, __FILE__);
			abort();
		}
		else {
			return fieldID;
		}
	}

	jfieldID getFieldID_sArraySize(void) {
		auto env = mCnxtJNI->getJNIEnv();
		auto logger = dal::LoggerGod::getinst();

		auto fieldID = env->GetStaticFieldID(findClass_CallableJNI(), "sArraySize", "I");
		if (fieldID == nullptr) {
			logger.putFatal("Failed to find static field CallableJNI::sArraySize", __LINE__, __func__, __FILE__);
			abort();
		}
		else {
			return fieldID;
		}
	}

	jclass findClass_CallableJNI(void) {
		auto env = mCnxtJNI->getJNIEnv();
		auto logger = dal::LoggerGod::getinst();

		auto clazzCallableJNI = env->FindClass("com/sausagetaste/littleruler/CallableJNI");
		if (clazzCallableJNI == nullptr) {
			logger.putError("Failed to find class: CallableJNI", __LINE__, __func__, __FILE__);
			abort();
		}
		else {
			return clazzCallableJNI;
		}
	}

};