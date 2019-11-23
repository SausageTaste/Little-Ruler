#include "d_context_main.h"

#include "p_render_master.h"
#include "c_input_apply.h"
#include "s_logger_god.h"
#include "o_widget_textbox.h"
#include "o_widget_texview.h"
#include "u_timer.h"


namespace {

    class FPSCounter : public dal::Widget2 {

    private:
        dal::Label2 m_label;

        dal::Timer m_timerForFPSReport;
        size_t m_frameAccum;

    public:
        FPSCounter(void)
            : dal::Widget2(nullptr)
            , m_label(this)
            , m_frameAccum(0)
        {
            this->setPos(10.0f, 10.0f);
            this->setSize(50.0f, 20.0f);
        }

        virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override {
            this->update();
            this->m_label.render(uniloc, width, height);
        }

    private:
        void update(void) {
            ++this->m_frameAccum;
            const auto elapsedForFPS = this->m_timerForFPSReport.getElapsed();
            if ( elapsedForFPS > 0.1f ) {
                const auto fps = static_cast<unsigned int>(static_cast<float>(this->m_frameAccum) / elapsedForFPS);
                this->m_label.setText(std::to_string(fps));
                this->m_timerForFPSReport.check();
                this->m_frameAccum = 0;
            }
        }

    protected:
        virtual void onScrSpaceBoxUpdate(void) override {
            this->m_label.setSize(this->getSize());
            this->m_label.setPos(this->getPos());
        };

    };

}


namespace {

    class InGameCxt : public dal::IContext {

    private:
        dal::ShaderMaster& m_shaders;
        dal::RenderMaster& m_renMas;
        dal::SceneGraph& m_scene;
        dal::TaskMaster& m_task;

        // Contexts
        dal::IContext* m_cnxtPauseMenu;

        dal::PlayerControlWidget m_crtlWidget;
        FPSCounter m_fcounter;

        unsigned m_winWidth, m_winHeight;

    public:
        InGameCxt(
            const unsigned width, const unsigned height,
            dal::ShaderMaster& shaders, dal::RenderMaster& renMas, dal::SceneGraph& scene, dal::TaskMaster& taskMas
        )
            : m_shaders(shaders)
            , m_renMas(renMas)
            , m_scene(scene)
            , m_task(taskMas)
            , m_cnxtPauseMenu(nullptr)
            , m_crtlWidget(static_cast<float>(width), static_cast<float>(height))
            , m_winWidth(width)
            , m_winHeight(height)
        {

        }

        virtual ~InGameCxt(void) override {

        }

        virtual IContext* update(const float deltaTime) override {
            dal::IContext* nextContext = this;

            // Process Input
            {
                {
                    auto& tq = dal::TouchEvtQueueGod::getinst();

                    for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                        const auto& e = tq.at(i);
                        this->m_crtlWidget.onTouch(e);
                    }

                    tq.clear();
                }

                {
                    auto& kq = dal::KeyboardEvtQueueGod::getinst();
                    const auto kqSize = kq.getSize();

                    for ( unsigned int i = 0; i < kqSize; ++i ) {
                        const auto& e = kq.at(i);

                        if ( dal::KeySpec::escape == e.m_key && dal::KeyActionType::up == e.m_actionType ) {
                            nextContext = this->m_cnxtPauseMenu;
                            dalVerbose("ingame -> pause");
                        }

                        this->m_crtlWidget.onKeyInput(e, kq.getKeyStates());
                    }

                    kq.clear();
                }

                {
                    auto& state = this->m_scene.m_entities.get<dal::cpnt::CharacterState>(this->m_scene.m_player);
                    const auto winSize = dal::GlobalStateGod::getinst().getWinSizeFloat();
                    const auto info = this->m_crtlWidget.getMoveInfo(deltaTime, winSize.x, winSize.y);
                    state.update(deltaTime, info);
                }
            }

            this->m_task.update();

            this->m_scene.update(deltaTime);
            this->m_renMas.update(deltaTime);
            this->m_renMas.render(this->m_scene.m_entities);

            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                auto& uniloc = this->m_shaders.useOverlay();

                this->m_crtlWidget.render(uniloc, this->m_winWidth, this->m_winHeight);
                this->m_fcounter.render(uniloc, this->m_winWidth, this->m_winHeight);
            }

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            this->m_crtlWidget.onParentResize(width, height);
            this->m_winWidth = width;
            this->m_winHeight = height;
        }

        void registerContexts(dal::IContext* const pauseMenu) {
            this->m_cnxtPauseMenu = pauseMenu;
        }

    };

}


namespace {

    class PauseMenu : public dal::IContext {

    private:
        dal::ShaderMaster& m_shaders;
        dal::TaskMaster& m_task;

        // Contexts
        dal::IContext* m_cnxtIngame;

        dal::ColoredTile m_red;

        unsigned m_winWidth, m_winHeight;

    public:
        PauseMenu(const unsigned width, const unsigned height, dal::ShaderMaster& shaders, dal::TaskMaster& taskMas)
            : m_shaders(shaders)
            , m_task(taskMas)
            , m_cnxtIngame(nullptr)
            , m_red(nullptr, 1, 0, 0, 1)
            , m_winWidth(width)
            , m_winHeight(height)
        {
            this->m_red.setSize(width, height);
            this->m_red.setPos(0, 0);
        }

        virtual dal::IContext* update(const float deltaTime) override {
            dal::IContext* nextContext = this;

            // Process Input
            {
                {
                    auto& tq = dal::TouchEvtQueueGod::getinst();

                    for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                        const auto& e = tq.at(i);
                    }

                    tq.clear();
                }

                {
                    auto& kq = dal::KeyboardEvtQueueGod::getinst();
                    const auto kqSize = kq.getSize();

                    for ( unsigned int i = 0; i < kqSize; ++i ) {
                        const auto& e = kq.at(i);

                        if ( dal::KeySpec::escape == e.m_key && dal::KeyActionType::up == e.m_actionType ) {
                            nextContext = this->m_cnxtIngame;
                            dalVerbose("pause -> ingame");
                        }
                    }

                    kq.clear();
                }
            }

            this->m_task.update();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            auto& uniloc = this->m_shaders.useOverlay();
            this->m_red.render(uniloc, this->m_winWidth, this->m_winHeight);

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            this->m_red.onParentResize(width, height);
            this->m_winWidth = width;
            this->m_winHeight = height;
        }

        void registerContexts(dal::IContext* const ingame) {
            this->m_cnxtIngame = ingame;
        }

    };

}


namespace dal {

    std::vector <std::unique_ptr<IContext>> initContexts(const unsigned width, const unsigned height,
        ShaderMaster& shaders, RenderMaster& renMas, SceneGraph& scene, TaskMaster& taskMas)
    {
        std::vector <std::unique_ptr<IContext>> result;

        std::unique_ptr<InGameCxt> ingame{ new InGameCxt{ width, height, shaders, renMas, scene, taskMas } };
        std::unique_ptr<PauseMenu> pause{ new PauseMenu{ width, height, shaders, taskMas } };

        ingame->registerContexts(pause.get());
        pause->registerContexts(ingame.get());

        result.push_back(std::unique_ptr<IContext>{ ingame.release() });
        result.push_back(std::unique_ptr<IContext>{ pause.release() });

        return result;
    }

}
