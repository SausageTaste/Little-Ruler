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
#include <d_logger.h>
#include <s_input_queue.h>

#include "javautil.h"


using namespace std::string_literals;


namespace {

    dal::Mainloop* gMainloop = nullptr;

    AAssetManager* gAssMan = nullptr;
    std::string g_storagePath;
    unsigned int g_width = 0, g_height = 0;

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

    void swapBit32(void* val) {
        uint32_t* temp = (uint32_t*)val;
        *temp = swap_uint32(*temp);
    }

}


extern "C" {

    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
        dalVerbose("JNI_OnLoad");
        dal::initJavautil(vm);

        if (nullptr == dal::getJNIEnv()) {
            dalFatal("Failed JNI_OnLoad()");
            return JNI_ERR;
        }

        return JNI_VERSION_1_6;
    }

    JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_init(JNIEnv* env, jclass obj) {
        dalVerbose("JNI::init");
        if (gMainloop != nullptr) {
            delete gMainloop;
            gMainloop = nullptr;
            dalVerbose("delete gMainloop");
        }
    }

    JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_resize(JNIEnv* env, jclass type, jint width, jint height) {
        dalVerbose("JNI::resize");
        g_width = static_cast<unsigned int>(width);
        g_height = static_cast<unsigned int>(height);

        if (nullptr != gMainloop){
            gMainloop->onResize(g_width, g_height);
        }
    }

    JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_step(JNIEnv* env, jclass type) {
        if (gMainloop == nullptr) {
            if (!dal::Mainloop::isWhatFilesystemWantsGiven()) return;
            if (0 == g_width * g_height) return;

            gMainloop = new dal::Mainloop{g_width, g_height};
        }

        // Touch event handle
        {
			auto &touchQ = dal::TouchEvtQueueGod::getinst();
            const auto curIndex = dal::touchinput::getCurrentIndexAndReset();
            std::unique_ptr<jbyte[]> floatArr{ new jbyte[curIndex] };
            dal::touchinput::copyArray(floatArr.get(), curIndex);

            for (int i = 0; i < curIndex; i += 16) {
                auto xPos = reinterpret_cast<jfloat *>(&floatArr[i]);
                auto yPos = reinterpret_cast<jfloat *>(&floatArr[i + 4]);
                auto etype = reinterpret_cast<jint *>  (&floatArr[i + 8]);
                auto id = reinterpret_cast<jint *>  (&floatArr[i + 12]);

                if (!isSystemBigEndian()) {
                    swapBit32(xPos);
                    swapBit32(yPos);
                    swapBit32(etype);
                    swapBit32(id);
                }

                switch (*etype) {
                    case 1:  // ACTION_DOWN
                        touchQ.emplaceBack(*xPos, *yPos, dal::TouchActionType::down, *id);
                        break;
                    case 2:  // ACTION_MOVE
                        touchQ.emplaceBack(*xPos, *yPos, dal::TouchActionType::move, *id);
                        break;
                    case 3:  // ACTION_UP
                        touchQ.emplaceBack(*xPos, *yPos, dal::TouchActionType::up, *id);
                        break;
                    default:
                        dalWarn("Unknwon touch event type from Java.");
                        break;
                }
            }
        }

        gMainloop->update();
    }

    JNIEXPORT void JNICALL Java_com_sausagetaste_littleruler_LibJNI_giveRequirements(JNIEnv* env, jclass type, jobject assetManager, jstring sdcardPath) {
        dalVerbose("JNI::giveRequirements");

        // Asset manager
        gAssMan = AAssetManager_fromJava(env, assetManager);

        // Storage path
        g_storagePath = env->GetStringUTFChars(sdcardPath, nullptr);
        g_storagePath += '/';

        dal::Mainloop::giveWhatFilesystemWants(gAssMan, g_storagePath.c_str());
    }

}