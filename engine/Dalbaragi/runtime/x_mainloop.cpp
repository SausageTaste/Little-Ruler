#include "x_mainloop.h"

#include <time.h>
#include <unordered_set>

#include <fmt/format.h>

#include <d_logger.h>
#include <d_filesystem.h>
#include <u_math.h>

#include "s_configs.h"
#include "o_widget_textbox.h"
#include "g_charastate.h"
#include "p_model.h"
#include "o_widgetcache.h"
#include "u_luascript.h"


using namespace fmt::literals;


// Utils
namespace {

    struct Datetime {
        int m_year, m_month, m_day, m_hour, m_min, m_sec;
    };

    Datetime getCurrentDatetime(void) {
        Datetime info;

        const auto theTime = time(nullptr);
        struct std::tm timeInfo;

#if defined(_WIN32)
        const auto err = localtime_s(&timeInfo, &theTime);
#elif defined(__ANDROID__)
        const auto err = localtime_r(&theTime, &timeInfo);
#endif
        // TODO : Error handling.

        info.m_day = timeInfo.tm_mday;
        info.m_month = timeInfo.tm_mon + 1;  // Month is 0 – 11, add 1 to get a jan-dec 1-12 concept
        info.m_year = timeInfo.tm_year + 1900;  // Year is # years since 1900
        info.m_hour = timeInfo.tm_hour;
        info.m_min = timeInfo.tm_min;
        info.m_sec = timeInfo.tm_sec;

        return info;
    }

}


// FileLoggingChannel
namespace {

    class FileLoggingChannel : public dal::ILoggingChannel {

    public:
        FileLoggingChannel(void) {
            dal::LoggerGod::getinst().addChannel(this);
        }

        ~FileLoggingChannel(void) {
            dal::LoggerGod::getinst().deleteChannel(this);
        }

        virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {

        }

        virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {

        }

        virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {

        }

        virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
            this->saveToFile("warning", str, line, func, file);
        }

        virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
            this->saveToFile("error", str, line, func, file);
        }

        virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
            this->saveToFile("fatal", str, line, func, file);
        }

    private:
        static void saveToFile(const char* const logLevel, const char* const str, const int line, const char* const func, const char* const file) {
            const auto autoDeleted = dal::LoggerGod::getinst().disable();

            const auto dt = getCurrentDatetime();
            const auto fileID = fmt::format("log::log_{}-{:0>2}-{:0>2}_{:0>2}-{:0>2}.txt", dt.m_year, dt.m_month, dt.m_day, dt.m_hour, dt.m_min);
            const auto fileContents = makeFileContents(dt, logLevel, str, line, func, file);

            dal::assertLogFolder();
            auto logFile = dal::fileopen(fileID.c_str(), dal::FileMode2::append);
            if ( nullptr == logFile ) {
                fmt::print("Failed to create log file: {}\n", fileID);
                return;
            }

            const auto res = logFile->write(fileContents.c_str());
            if ( !res ) {
                fmt::print("Failed to write to log file: {}\n", fileID);
                return;
            }
        }

        static std::string makeFileContents(const Datetime& dt, const char* const logLevel, const char* const str, const int line, const char* const func, const char* const file) {
            static const char* const formatStr =
                "Dalbaragi Log\n"
                "{}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}\n"
                "\n"
                "File : {}\n"
                "Line : {}\n"
                "Function : {}\n"
                "Log level : {}\n"
                "\n"
                "========\n"
                "{}\n"
                "========\n"
                "\n"
                "##############\n"
                "\n"
                ;

            return fmt::format(formatStr,
                dt.m_year, dt.m_month, dt.m_day, dt.m_hour, dt.m_min, dt.m_sec, file, line, func, logLevel, str
            );
        }

    } g_fileLogger;

}


// Test codes
namespace {

    void test(const float deltaTime) {
        auto file = dal::fileopen(u8"한글::텍스트.txt", dal::FileMode2::read);
        if ( file ) {
            std::string buffer;
            const auto result = file->readText(buffer);
            dalAssert(result);
            dalVerbose(buffer);
        }
        else {
            dalError(u8"Failed to open file: \"한글::텍스트.txt\"");
        }

        dal::testFile();
    }

}


namespace dal {

    // Static

    void Mainloop::giveWhatFilesystemWants(void* androidAssetManager, const char* const sdcardPath) {
        dal::ExternalFuncGod::getinst().giveValue_assetMgr(reinterpret_cast<AAssetManager*>(androidAssetManager));
        dal::ExternalFuncGod::getinst().giveValue_androidStoragePath(sdcardPath);
    }

    bool Mainloop::isWhatFilesystemWantsGiven(void) {
        return true;
    }

    // Public

    Mainloop::Mainloop(const unsigned int winWidth, const unsigned int winHeight)
        // Managers
        : m_resMas(m_task)
        , m_scene(m_resMas, m_phyworld, winWidth, winHeight)
        , m_renderMan(m_scene, m_shader, m_resMas, &m_scene.m_playerCam, winWidth, winHeight)
        // Contexts
        , m_contexts(initContexts(winWidth, winHeight, m_shader, m_renderMan, m_scene, m_task, m_phyworld))
        , m_currentContext(m_contexts.front().get())
        // Misc
        , m_flagQuit(false)
    {
        // This might be done already by SceneGraph or OverlayMaster but still...
        {
            GlobalStateGod::getinst().setWinSize(winWidth, winHeight);
        }

        // Check filesystem init
        {
            if ( !isWhatFilesystemWantsGiven() ) {
                dalAbort("Please call Mainloop::giveWhatFilesystemWants before constructor!");
            }
        }

        // Camera
        {
            this->m_scene.m_playerCam.m_pos = { 0.0f, 3.0f, 3.0f };
        }

        // Widgets
        {
            LuaState::giveDependencies(this, &this->m_renderMan);
        }

        // Animation modifier
        /* {
            auto& animModel = this->m_scene.m_entities.get<cpnt::AnimatedModel>(this->m_scene.m_player);
            const auto& trans = this->m_scene.m_entities.get<cpnt::Transform>(this->m_scene.m_player);
            const auto& skeleton = animModel.m_model->getSkeletonInterf();

            while ( skeleton.isEmpty() ) {
                this->m_task.update();
            }

            std::unordered_set<jointID_t> hairJointIDs;
            std::unordered_map<jointID_t, PhysicsEntity> particles;
            std::unordered_map<jointID_t, glm::vec3> localPoses;

            const auto hairRootIndex = [&](void) {
                for ( int i = 0; i < skeleton.getSize(); ++i ) {
                    if ( 1 == skeleton.at(i).jointType() ) {
                        return i;
                    }
                }
                dalAbort("Failed to find hair root joint.");
            }();
            hairJointIDs.emplace(hairRootIndex);

            for ( jointID_t i = hairRootIndex + 1; ; ++i ) {
                const auto& jointInfo = animModel.m_model->getSkeletonInterf().at(i);
                const auto parentID = jointInfo.parentIndex();
                if ( hairJointIDs.end() == hairJointIDs.find(parentID) )
                    break;

                hairJointIDs.emplace(i);
                const auto& addedLocalPos = localPoses.emplace(i, jointInfo.localPos());
                auto addedParticle = particles.emplace(i, this->m_phyworld.newParticle()); dalAssert(addedParticle.second);

                const auto thisLocalPos = addedLocalPos.first->second;

                // POTENTIAL BUG!!
                // The z value jointInfo.m_boneOffset is minus of what it should be.
                if ( hairRootIndex == parentID ) {
                    this->m_phyworld.registerUnaryMod(
                        std::shared_ptr<dal::UnaryPhyModifier>{new ParticleToEntity{
                            this->m_scene.m_player, this->m_scene.m_entities,
                            glm::translate(glm::mat4{1.f}, thisLocalPos)
                        }},
                        addedParticle.first->second
                    );
                }
                else {
                    auto parentParticle = particles.find(parentID); dalAssert(particles.end() != parentParticle);
                    const auto parentLocalPos = localPoses.at(parentID);

                    this->m_phyworld.registerBinaryMod(
                        std::shared_ptr<dal::BinaryPhyModifier>{new FixedPointSpringPulling{ 10, glm::distance(parentLocalPos, thisLocalPos) / 10.0 }},
                        parentParticle->second, addedParticle.first->second
                    );

                    animModel.m_animState.addModifier(i, std::shared_ptr<dal::IJointModifier>{new HairJointPhysics{
                        addedParticle.first->second, this->m_phyworld, this->m_scene.m_player, this->m_scene.m_entities
                        }});

                    std::shared_ptr<UnaryPhyModifier> gravity{ new ParticleGravity{3} };
                    this->m_phyworld.registerUnaryMod(gravity, addedParticle.first->second);
                }

                // Draw balls to debug.
                {
                    const auto entity = this->m_scene.addObj_static("asset::pbr_ball.dmd");
                    auto& transform = this->m_scene.m_entities.get<cpnt::Transform>(entity);
                    transform.setScale(0.02);

                    auto& enttCtrl = this->m_scene.m_entities.assign<cpnt::EntityCtrl>(entity);
                    enttCtrl.m_ctrler.reset(new EntityToParticle{ addedParticle.first->second, this->m_phyworld });
                }
            }
        } */

        // Misc
        {
            this->m_timer.setCapFPS(0);
            assertUserdataFolder();
        }

        // Test
        {
            test(this->m_timer.getElapsed());
        }
    }

    Mainloop::~Mainloop(void) {

    }

    int Mainloop::update(void) {
        if ( this->m_flagQuit ) {
            return -1;
        }

        const auto deltaTime = m_timer.checkGetElapsed();
        this->m_currentContext = this->m_currentContext->update(deltaTime);

        return 0;
    }

    void Mainloop::onResize(unsigned int width, unsigned int height) {
        GlobalStateGod::getinst().setWinSize(width, height);

        this->m_renderMan.onWinResize(width, height);
        this->m_scene.onResize(width, height);

        for ( auto& cnxt : this->m_contexts ) {
            cnxt->onWinResize(width, height);
        }

        dalVerbose("Resize : {} x {}"_format(width, height));
    }

}
