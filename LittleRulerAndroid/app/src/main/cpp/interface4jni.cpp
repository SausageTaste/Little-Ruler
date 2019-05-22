#include "interface4jni.h"

#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <byteswap.h>
#include <cstdio>
#include <sys/stat.h>  // mkdir

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


using namespace std::string_literals;


namespace {

	dal::Mainloop *gMainloop = nullptr;
	dal::PersistState *gSavedState = nullptr;

	AAssetManager *gAssMan = nullptr;
	std::string g_storagePath;
	dal::LoggerGod &gLogger = dal::LoggerGod::getinst();

	ContextJNI gCnxtJNI;
	TouchInputArray gTouchInputArray;

}


namespace {

	uint32_t swap_uint32(uint32_t val) {
		val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
		return (val << 16) | (val >> 16);
	}

	bool isSystemBigEndian(void) {
		union {
			uint32_t i;
			char c[4];
		} bint = { 0x01020304 };

		return bint.c[0] == 1;
	}

	void swapBit32(void *val) {
		uint32_t *temp = (uint32_t *) val;
		*temp = swap_uint32( *temp );
	}

}


extern "C" {


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) try {
	dalVerbose("JNI_OnLoad");
	gCnxtJNI.init(vm);

	if (gCnxtJNI.getJNIEnv() == nullptr) {
		dalFatal("Failed JNI_OnLoad()");
		return JNI_ERR;
	}

	gTouchInputArray.init(&gCnxtJNI);

	return JNI_VERSION_1_6;
}
catch (const std::exception& e) {
	dalFatal("An exception thrown: "s + e.what());
	throw;
}
catch (const std::string& e) {
	dalFatal("A string thrown: "s + e);
	throw;
}
catch (const char* const e) {
	dalFatal("A char* thrown: "s + e); throw;
}
catch (const int e) {
	dalFatal("An int thrown: "s + std::to_string(e));
	throw;
}
catch (...) {
	dalFatal("Something unkown thrown");
	throw;
}


JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_init(JNIEnv *env, jclass obj) try {
	dalVerbose("JNI::init");
	if (gMainloop != nullptr) {
		gSavedState = gMainloop->getSavedState();
		delete gMainloop;
		gMainloop = nullptr;
		dalVerbose("delete gMainloop");
	}
}
catch (const std::exception& e) {
	dalFatal("An exception thrown: "s + e.what()); throw;
}
catch (const std::string& e) {
	dalFatal("A string thrown: "s + e); throw;
}
catch (const char* const e) {
	dalFatal("A char* thrown: "s + e); throw;
}
catch (const int e) {
	dalFatal("An int thrown: "s + std::to_string(e)); throw;
}
catch (...) {
	dalFatal("Something unkown thrown"); throw;
}


JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_resize(JNIEnv *env, jclass type, jint width, jint height) try {
	dalVerbose("JNI::resize");
	dal::Mainloop::giveScreenResFirst((unsigned int)width, (unsigned int)height);
}
catch (const std::exception& e) {
	dalFatal("An exception thrown: "s + e.what()); throw;
}
catch (const std::string& e) {
	dalFatal("A string thrown: "s + e); throw;
}
catch (const char* const e) {
	dalFatal("A char* thrown: "s + e); throw;
}
catch (const int e) {
	dalFatal("An int thrown: "s + std::to_string(e)); throw;
}
catch (...) {
	dalFatal("Something unkown thrown"); throw;
}


JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_step(JNIEnv *env, jclass type) try {
	if (gMainloop == nullptr) {
		if (!dal::Mainloop::isScreenResGiven()) return;
		if (!dal::Mainloop::isWhatFilesystemWantsGiven()) return;

		gMainloop = new dal::Mainloop{ gSavedState };
		gSavedState = nullptr;
	}

	// Touch event handle
	{
		const auto curIndex = gTouchInputArray.getCurrentIndexAndReset();

		auto& touchQ = dal::TouchEvtQueueGod::getinst();

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

	gMainloop->update();
}
catch (const std::exception& e) {
	dalFatal("An exception thrown: "s + e.what()); throw;
}
catch (const std::string& e) {
	dalFatal("A string thrown: "s + e); throw;
}
catch (const char* const e) {
	dalFatal("A char* thrown: "s + e); throw;
}
catch (const int e) {
	dalFatal("An int thrown: "s + std::to_string(e)); throw;
}
catch (...) {
	dalFatal("Something unkown thrown"); throw;
}


JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_giveRequirements(JNIEnv *env, jclass type, jobject assetManager, jstring sdcardPath) try {
	dalVerbose("JNI::giveRequirements");

	// Asset manager
	gAssMan = AAssetManager_fromJava(env, assetManager);

	// Storage path
	g_storagePath = env->GetStringUTFChars(sdcardPath, NULL);
	g_storagePath += '/';

	dal::Mainloop::giveWhatFilesystemWants(gAssMan, g_storagePath.c_str());
}
catch (const std::exception& e) {
	dalFatal("An exception thrown: "s + e.what()); throw;
}
catch (const std::string& e) {
	dalFatal("A string thrown: "s + e); throw;
}
catch (const char* const e) {
	dalFatal("A char* thrown: "s + e); throw;
}
catch (const int e) {
	dalFatal("An int thrown: "s + std::to_string(e)); throw;
}
catch (...) {
	dalFatal("Something unkown thrown"); throw;
}


}