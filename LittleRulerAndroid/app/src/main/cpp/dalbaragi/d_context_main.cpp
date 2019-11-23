#include "d_context_main.h"

#include <fmt/format.h>

#include "p_render_master.h"
#include "c_input_apply.h"
#include "s_logger_god.h"
#include "o_widget_textbox.h"
#include "o_widget_texview.h"
#include "o_widgetmanager.h"
#include "u_timer.h"
#include "u_luascript.h"


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


    class LuaConsole : public dal::Widget2 {

    private:
        class TextStreamChannel : public dal::ILoggingChannel {

        private:
            dal::StringBufferBasic& m_texStream;

        public:
            TextStreamChannel(dal::StringBufferBasic& texStream)
                : m_texStream(texStream)
            {

            }

            virtual void verbose(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = fmt::format("[VERBO] {}\n", str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void debug(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = fmt::format("[DEBUG] {}\n", str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void info(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = fmt::format("[INFO] {}\n", str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void warn(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = fmt::format("[WARN] {}\n", str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void error(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = fmt::format("[ERROR] {}\n", str);
                this->m_texStream.append(text.data(), text.size());
            }

            virtual void fatal(const char* const str, const int line, const char* const func, const char* const file) override {
                const auto text = fmt::format("[FATAL] {}\n", str);
                this->m_texStream.append(text.data(), text.size());
            }

        };

    private:
        dal::WidgetInputDispatcher m_dispatcher;
        dal::LineEdit m_lineEdit;
        dal::TextBox m_textBox;
        glm::vec4 m_bgColor;
        Widget2* m_focused;
        dal::LuaState m_luaState;
        dal::StringBufferBasic m_strbuf;
        TextStreamChannel m_stream;

    public:
        LuaConsole(void)
            : dal::Widget2(nullptr)
            , m_lineEdit(this)
            , m_textBox(this)
            , m_bgColor(0.0f, 0.0f, 0.0f, 1.0f)
            , m_focused(nullptr)
            , m_stream(m_strbuf)
        {
            this->m_lineEdit.setHeight(20.0f);
            this->m_lineEdit.setCallbackOnEnter([this](const char* const text) {
                this->m_luaState.exec(text);
                });

            this->m_luaState.replaceStrbuf(&this->m_strbuf);
            this->m_textBox.replaceBuffer(&this->m_strbuf);
            dal::LoggerGod::getinst().addChannel(&this->m_stream);

            this->setPos(10.0f, 50.0f);
            this->setSize(300.0f, 300.0f);
        }

        ~LuaConsole(void) {
            dal::LoggerGod::getinst().deleteChannel(&this->m_stream);
        }

        virtual void render(const dal::UnilocOverlay& uniloc, const float width, const float height) override {
            dal::QuadRenderInfo qinfo;
            std::tie(qinfo.m_devSpcP1, qinfo.m_devSpcP2) = this->makeDeviceSpace(width, height);
            qinfo.m_color = this->m_bgColor;
            dal::renderQuadOverlay(uniloc, qinfo);

            this->m_lineEdit.render(uniloc, width, height);
            this->m_textBox.render(uniloc, width, height);
        }

        virtual dal::InputCtrlFlag onTouch(const dal::TouchEvent& e) override {
            dal::Widget2* widgetArr[2] = { &this->m_lineEdit, &this->m_textBox };
            const auto [flag, focused] = this->m_dispatcher.dispatch(widgetArr, widgetArr + 2, e);

            this->m_focused = dal::resolveNewFocus(this->m_focused, focused);

            return flag;
        }

        virtual dal::InputCtrlFlag onKeyInput(const dal::KeyboardEvent& e, const dal::KeyStatesRegistry& keyStates) override {
            if ( &this->m_lineEdit == this->m_focused ) {
                const auto iter = &this->m_focused;
                const auto end = iter + 1;
                return this->m_dispatcher.dispatch(iter, end, e, keyStates);
            }

            return dal::InputCtrlFlag::ignored;
        }

        virtual void onFocusChange(const bool v) override {
            if ( !v ) {
                this->m_lineEdit.onFocusChange(false);
                this->m_textBox.onFocusChange(false);
                this->m_focused = nullptr;
            }
        }

        void exec(const char* const str) {
            this->m_luaState.exec(str);
        }

    protected:
        virtual void onScrSpaceBoxUpdate(void) override {
            constexpr float INNER_MARGIN = 5.0f;

            const auto pp1 = this->getPoint00();
            const auto pp2 = this->getPoint11();

            {
                this->m_lineEdit.setPos(pp1.x + INNER_MARGIN, pp2.y - INNER_MARGIN - this->m_lineEdit.getHeight());
                this->m_lineEdit.setWidth(this->getWidth() - INNER_MARGIN - INNER_MARGIN);
            }

            {
                this->m_textBox.setPos(pp1.x + INNER_MARGIN, pp1.y + INNER_MARGIN);
                this->m_textBox.setSize(
                    this->getWidth() - INNER_MARGIN - INNER_MARGIN,
                    this->getHeight() - this->m_lineEdit.getHeight() - INNER_MARGIN - INNER_MARGIN - INNER_MARGIN
                );
            }
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
        LuaConsole m_luaConsole;

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
                        this->m_luaConsole.onTouch(e);
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

                        this->m_luaConsole.onKeyInput(e, kq.getKeyStates());
                    }

                    kq.clear();
                }
            }

            this->m_task.update();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            auto& uniloc = this->m_shaders.useOverlay();
            this->m_red.render(uniloc, this->m_winWidth, this->m_winHeight);
            this->m_luaConsole.render(uniloc, this->m_winWidth, this->m_winHeight);

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            this->m_red.onParentResize(width, height);
            this->m_luaConsole.onParentResize(width, height);

            this->m_winWidth = width;
            this->m_winHeight = height;
        }

        void registerContexts(dal::IContext* const ingame) {
            this->m_cnxtIngame = ingame;
        }

    };

}


namespace {

    class TitleScreen : public dal::IContext {

    private:
        dal::ShaderMaster& m_shaders;
        dal::TaskMaster& m_task;

        // Contexts
        dal::IContext* m_cnxtIngame;

        dal::ColoredTile m_background;
        dal::LineEdit m_lineedit;

        dal::Timer m_timer;
        unsigned m_winWidth, m_winHeight;

    public:
        TitleScreen(const unsigned width, const unsigned height, dal::ShaderMaster& shaders, dal::TaskMaster& taskMas)
            : m_shaders(shaders)
            , m_task(taskMas)
            , m_cnxtIngame(nullptr)
            , m_background(nullptr, 0.1f, 0.1f, 0.1f, 1.f)
            , m_lineedit(nullptr)
            , m_winWidth(width)
            , m_winHeight(height)
        {
            this->m_background.setSize(width, height);
            this->m_background.setPos(0, 0);

            this->m_lineedit.setPos(30, 30);
            this->m_lineedit.setSize(500, 500);
            this->m_lineedit.setBackgroundColor(0.1f, 0.1f, 0.1f, 1.f);
            this->m_lineedit.setText("Little Ruler");
        }

        virtual dal::IContext* update(const float deltaTime) override {
            dal::IContext* nextContext = this;

            // Process Input
            {
                {
                    auto& tq = dal::TouchEvtQueueGod::getinst();

                    for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                        const auto& e = tq.at(i);
                        if ( dal::TouchActionType::down == e.m_actionType ) {
                            nextContext = this->m_cnxtIngame;
                        }
                    }

                    tq.clear();
                }

                {
                    auto& kq = dal::KeyboardEvtQueueGod::getinst();
                    const auto kqSize = kq.getSize();

                    for ( unsigned int i = 0; i < kqSize; ++i ) {
                        const auto& e = kq.at(i);

                        if ( dal::KeyActionType::down == e.m_actionType ) {
                            nextContext = this->m_cnxtIngame;
                        }
                    }

                    kq.clear();
                }
            }

            this->m_task.update();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            auto& uniloc = this->m_shaders.useOverlay();
            this->m_background.render(uniloc, this->m_winWidth, this->m_winHeight);
            this->m_lineedit.render(uniloc, this->m_winWidth, this->m_winHeight);

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            this->m_background.onParentResize(width, height);
            this->m_lineedit.onParentResize(width, height);

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

        std::unique_ptr<TitleScreen> title{ new TitleScreen{ width, height, shaders, taskMas } };
        std::unique_ptr<InGameCxt> ingame{ new InGameCxt{ width, height, shaders, renMas, scene, taskMas } };
        std::unique_ptr<PauseMenu> pause{ new PauseMenu{ width, height, shaders, taskMas } };

        title->registerContexts(ingame.get());
        ingame->registerContexts(pause.get());
        pause->registerContexts(ingame.get());

        result.push_back(std::unique_ptr<IContext>{ title.release() });
        result.push_back(std::unique_ptr<IContext>{ ingame.release() });
        result.push_back(std::unique_ptr<IContext>{ pause.release() });

        return result;
    }

}
