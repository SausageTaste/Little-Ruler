#include "javautil.h"

#include "s_logger_god.h"


// Global var
namespace {

	JavaVM* g_javaVM = nullptr;

}


namespace {

	jclass findClass_CallableJNI(JNIEnv* const env) {
		auto clazzCallableJNI = env->FindClass( "com/sausagetaste/littleruler/CallableJNI" );
		if (clazzCallableJNI == nullptr) {
			dalAbort( "Failed to find class: CallableJNI" );
		} else {
			return clazzCallableJNI;
		}
	}

	jclass findClass_ActivityJNI(JNIEnv* const env) {
		auto clazzActivityJNI = env->FindClass("com/sausagetaste/littleruler/ActivityJNI");
		if (nullptr == clazzActivityJNI) {
			dalAbort("Failed to find java class: ActivityJNI");
		}
		else {
			return clazzActivityJNI;
		}
	}


	jbyteArray getArrayObject_sTouchEvents(JNIEnv* const env) {
		auto &logger = dal::LoggerGod::getinst();

		auto fieldID = env->GetStaticFieldID( findClass_CallableJNI(env), "sTouchEvents", "[B" );
		if (fieldID == nullptr) {
			dalAbort( "Failed to find static field CallableJNI::sTouchEvents" );
		}

		return static_cast<jbyteArray>(env->GetStaticObjectField( findClass_CallableJNI(env), fieldID ));
	}

	jfieldID getFieldID_sCurrentIndex(JNIEnv* const env) {
		auto &logger = dal::LoggerGod::getinst();

		auto fieldID = env->GetStaticFieldID( findClass_CallableJNI(env), "sCurrentIndex", "I" );
		if (fieldID == nullptr) {
			dalAbort( "Failed to find static field CallableJNI::sCurrentIndex" );
		} else {
			return fieldID;
		}
	}

	jfieldID getFieldID_sArraySize(JNIEnv* const env) {
		auto &logger = dal::LoggerGod::getinst();

		auto fieldID = env->GetStaticFieldID( findClass_CallableJNI(env), "sArraySize", "I" );
		if (fieldID == nullptr) {
			dalAbort( "Failed to find static field CallableJNI::sArraySize" );
		} else {
			return fieldID;
		}
	}

}


// JNIEnv
namespace dal {

	void initJavautil(JavaVM* javaVM) {
		g_javaVM = javaVM;
	}

	JNIEnv* getJNIEnv(void) {
		if (nullptr == g_javaVM) return nullptr;

		JNIEnv* env;
		if (g_javaVM->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
			return nullptr;
		}
		else {
			return env;
		}
	}

	JNIEnv* assertJNIEnv(void) {
		auto env = getJNIEnv();
		if (nullptr == env) {
			dalAbort("JNI not initiated.");
		}
		else {
			return env;
		}
	}

}


// TouchInputArray
namespace dal {
	namespace touchinput {

		int getArraySize(void) {
			auto env = assertJNIEnv();
			return (int) env->GetStaticIntField( findClass_CallableJNI(env), getFieldID_sArraySize(env));
		}

		int getCurrentIndexAndReset(void) {
			auto env = assertJNIEnv();
			auto currentIndex = static_cast<int>(env->GetStaticIntField( findClass_CallableJNI(env), getFieldID_sCurrentIndex(env)));

			env->SetStaticIntField( findClass_CallableJNI(env), getFieldID_sCurrentIndex(env), 0 );
			return currentIndex;
		}

		jbyte *startAccess(void) {
			auto env = assertJNIEnv();
			return env->GetByteArrayElements( getArrayObject_sTouchEvents(env), nullptr );
		}

		void finishAccess(jbyte *array) {
			auto env = assertJNIEnv();
			env->ReleaseByteArrayElements( getArrayObject_sTouchEvents(env), array, 0 );
		}

		void copyArray(jbyte *buf, int bufSize) {
			auto env = assertJNIEnv();
			env->GetByteArrayRegion( getArrayObject_sTouchEvents(env), 0, (jsize) bufSize, buf );
		}

	}
}