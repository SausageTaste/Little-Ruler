#include "interface4jni.h"

#include <memory>
#include <string>
#include <vector>
#include <assert.h>
#include <byteswap.h>

#include <jni.h>
#include <inttypes.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <x_mainloop.h>
#include <s_logger_god.h>
#include <s_input_queue.h>
#include <x_persist.h>

#include "ContextJNI.h"
#include "TouchInputArray.h"


using namespace std;


dal::Mainloop* gMainloop = nullptr;
dal::PersistState* gSavedState = nullptr;

AAssetManager* gAssMan = nullptr;
dal::LoggerGod& gLogger = dal::LoggerGod::getinst();

ContextJNI gCnxtJNI;
TouchInputArray gTouchInputArray;


uint32_t swap_uint32( uint32_t val ) {
	val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
	return (val << 16) | (val >> 16);
}

bool isSystemBigEndian(void) {
	union {
		uint32_t i;
		char c[4];
	} bint = {0x01020304};

	return bint.c[0] == 1;
}

void swapBit32(void* val) {
	uint32_t* temp = (uint32_t*)val;
	*temp = swap_uint32(*temp);
}


extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	gLogger.putTrace("JNI_OnLoad");
	gCnxtJNI.init(vm);

	if (gCnxtJNI.getJNIEnv() == nullptr) {
		dal::LoggerGod::getinst().putFatal("Failed JNI_OnLoad()");
		return JNI_ERR;
	}

	gTouchInputArray.init(&gCnxtJNI);

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_init(JNIEnv *env, jclass obj) {
	gLogger.putTrace("JNI::init");
	if (gMainloop != nullptr) {
		gSavedState = gMainloop->getSavedState();
		delete gMainloop;
		gMainloop = nullptr;
		gLogger.putTrace("delete gMainloop");
	}
}

JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_resize(JNIEnv *env, jclass type, jint width, jint height) {
	gLogger.putTrace("JNI::resize");
	dal::Mainloop::giveScreenResFirst((unsigned int)width, (unsigned int)height);
}

JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_step(JNIEnv *env, jclass type) {
	if (gMainloop == nullptr) {
		if (!dal::Mainloop::isScreenResGiven()) return;
		if (!dal::Mainloop::isWhatFilesystemWantsGiven()) return;

		gMainloop = new dal::Mainloop{ gSavedState };
		gSavedState = nullptr;
	}

	/* Touch event handle */ {
		auto curIndex = gTouchInputArray.getCurrentIndexAndReset();

		auto &touchQ = dal::TouchEvtQueueGod::getinst();

		jbyte* floatArr = new jbyte[curIndex];
		gTouchInputArray.copyArray(floatArr, curIndex);
		for (int i = 0; i < curIndex; i += 16) {

			auto xPos  = (jfloat*) &floatArr[i     ];
			auto yPos  = (jfloat*) &floatArr[i +  4];
			auto etype = (jint*)   &floatArr[i +  8];
			auto id    = (jint*)   &floatArr[i + 12];

			if (!isSystemBigEndian()) {
				swapBit32(xPos);
				swapBit32(yPos);
				swapBit32(etype);
				swapBit32(id);
			}

			switch (*etype) {
				case 1:  // ACTION_DOWN
					touchQ.emplaceBack(*xPos, *yPos, dal::TouchType::down, *id);
					break;
				case 2:  // ACTION_MOVE
					touchQ.emplaceBack(*xPos, *yPos, dal::TouchType::move, *id);
					break;
				case 3:  // ACTION_UP
					touchQ.emplaceBack(*xPos, *yPos, dal::TouchType::up, *id);
					break;
				default:
					break;
			}

		}
	}

	try {
		gMainloop->update();
	}
	catch (const std::exception& e) {
		gLogger.putFatal("An exception thrown: "s + e.what());
	}
	catch (const std::string& e) {
		gLogger.putFatal("A string thrown: "s + e);
	}
	catch (const int e) {
		gLogger.putFatal("An int thrown: "s + std::to_string(e));
	}
	catch (...) {
		gLogger.putFatal("Something unkown thrown");
	}
}

JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_giveRequirements(JNIEnv *env, jclass type, jobject assetManager) {
	gLogger.putTrace("JNI::giveRequirements");
	gAssMan = AAssetManager_fromJava(env, assetManager);
	dal::Mainloop::giveWhatFilesystemWants(gAssMan);
}

}