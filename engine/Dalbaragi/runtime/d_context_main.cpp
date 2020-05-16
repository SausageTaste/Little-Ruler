#include "d_context_main.h"

#include <fmt/format.h>

#include <d_logger.h>
#include <d_phyworld.h>
#include <d_widget_view.h>
#include <d_w_text_view.h>
#include <d_widget_manager.h>

#include "p_render_master.h"
#include "c_input_apply.h"
#include "u_timer.h"
#include "u_luascript.h"
#include "d_overlay_interface.h"
#include "d_text_overlay.h"
#include "s_input_queue.h"


namespace {

    class FPSCounter : public dal::Widget2D {

    private:
        dal::Lable m_label;
        dal::Timer m_timerForFPSReport;
        size_t m_frameAccum;

    public:
        FPSCounter(dal::GlyphMaster& glyph)
            : dal::Widget2D(nullptr, dal::drawOverlay)
            , m_label(this, dal::drawOverlay, glyph)
            , m_frameAccum(0)
        {
            this->m_label.setMargin(5);

            this->aabb().setPosSize<float>(10, 10, 50, 30);
            this->onUpdateAABB();
        }

        virtual void render(const float width, const float height, const void* uniloc) override {
            this->update();
            this->m_label.render(width, height, uniloc);
        }

        virtual void onUpdateAABB(void) override {
            this->m_label.aabb().setAs(this->aabb());
            this->m_label.onUpdateAABB();
        }

    private:
        void update(void) {
            ++this->m_frameAccum;
            const auto elapsedForFPS = this->m_timerForFPSReport.getElapsed();
            if ( elapsedForFPS > 0.05f ) {
                const auto fps = static_cast<unsigned int>(static_cast<float>(this->m_frameAccum) / elapsedForFPS);
                this->m_label.setText(std::to_string(fps).c_str());
                this->m_timerForFPSReport.check();
                this->m_frameAccum = 0;
            }
        }

    };


    class LuaConsole : public dal::Widget2D {

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
        dal::InputDispatcher m_dispatcher;

        dal::LineEdit m_lineEdit;
        dal::TextBox m_textBox;
        dal::ColorView m_bg;

        dal::Widget2D* m_focused;
        dal::LuaState m_luaState;
        dal::StringBufferBasic m_strbuf;
        TextStreamChannel m_stream;

        float m_lineEditHeight = 20;

    public:
        LuaConsole(dal::GlyphMaster& glyph)
            : dal::Widget2D(nullptr, dal::drawOverlay)

            , m_lineEdit(nullptr, dal::drawOverlay, glyph)
            , m_textBox(nullptr, dal::drawOverlay, glyph)
            , m_bg(nullptr, dal::drawOverlay)

            , m_focused(nullptr)
            , m_stream(m_strbuf)
        {
            this->m_lineEdit.setCallback_onReturn([this](const char* const text) {
                this->m_luaState.exec(text);
                });

            this->m_bg.m_color = glm::vec4{ 0, 0, 0, 1 };

            this->m_luaState.replaceStrbuf(&this->m_strbuf);
            dal::LoggerGod::getinst().addChannel(&this->m_stream);

            this->m_textBox.m_bg.m_color = glm::vec4{ 0.1, 0.1, 0.1, 1 };
            this->m_lineEdit.setBGColor(0.1, 0.1, 0.1, 1);

            this->aabb().setPosSize(10, 50, 300, 300);
            this->onUpdateAABB();
        }

        ~LuaConsole(void) {
            dal::LoggerGod::getinst().deleteChannel(&this->m_stream);
        }

        virtual void render(const float width, const float height, const void* uniloc) override {
            this->fetchText();

            this->m_bg.render(width, height, uniloc);
            this->m_lineEdit.render(width, height, uniloc);
            this->m_textBox.render(width, height, uniloc);
        }

        virtual auto onTouch(const dal::TouchEvent& e) -> dal::InputDealtFlag override {
            dal::Widget2D* widgetArr[2] = { &this->m_lineEdit, &this->m_textBox };
            const auto [flag, focused] = this->m_dispatcher.dispatch(widgetArr, widgetArr + 2, e);

            this->m_focused = dal::resolveNewFocus(this->m_focused, focused);

            return flag;
        }

        virtual auto onKeyInput(const dal::KeyboardEvent& e, const dal::KeyStatesRegistry& keyStates) -> dal::InputDealtFlag override {
            if ( &this->m_lineEdit == this->m_focused ) {
                const auto iter = &this->m_focused;
                const auto end = iter + 1;
                return this->m_dispatcher.dispatch(iter, end, e, keyStates);
            }

            return dal::InputDealtFlag::ignored;
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
        virtual void onUpdateAABB(void) override {
            constexpr float INNER_MARGIN = 5;

            const auto pp1 = this->aabb().point00();
            const auto pp2 = this->aabb().point11();

            this->m_lineEdit.aabb().setPosSize<float>(
                pp1.x + INNER_MARGIN,
                pp2.y - INNER_MARGIN - this->m_lineEditHeight,
                this->aabb().width() - INNER_MARGIN - INNER_MARGIN,
                this->m_lineEditHeight
                );

            this->m_textBox.aabb().setPosSize<float>(
                pp1.x + INNER_MARGIN,
                pp1.y + INNER_MARGIN,
                this->aabb().width() - INNER_MARGIN - INNER_MARGIN,
                this->aabb().height() - this->m_lineEditHeight - INNER_MARGIN - INNER_MARGIN - INNER_MARGIN
                );

            this->m_bg.aabb().setAs(this->aabb());

            this->m_lineEdit.onUpdateAABB();
            this->m_textBox.onUpdateAABB();
            this->m_bg.onUpdateAABB();
        }

        void fetchText(void) {
            this->m_textBox.m_text.addStr(this->m_strbuf.data());
            this->m_strbuf.clear();
        }

    };

}


namespace {

    class InGameCxt : public dal::IContext {

    private:
        dal::ShaderMaster& m_shaders;
        dal::RenderMaster& m_renMas;
        dal::SceneGraph& m_scene;
        dal::TaskMaster& m_task;
        dal::PhysicsWorld& m_phyworld;

        // Contexts
        dal::IContext* m_cnxtPauseMenu;

        dal::PlayerControlWidget m_crtlWidget;
        FPSCounter m_fcounter;

        unsigned m_winWidth, m_winHeight;

    public:
        InGameCxt(const unsigned width, const unsigned height, dal::Managers& managers)
            : m_shaders(managers.m_shaders)
            , m_renMas(managers.m_renMas)
            , m_scene(managers.m_scene)
            , m_task(managers.m_taskMas)
            , m_phyworld(managers.m_phyworld)
            , m_cnxtPauseMenu(nullptr)
            , m_crtlWidget(static_cast<float>(width), static_cast<float>(height))
            , m_fcounter(managers.m_glyph)
            , m_winWidth(width)
            , m_winHeight(height)
        {

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

            this->m_phyworld.update(deltaTime);
            this->m_scene.update(deltaTime);
            this->m_renMas.update(deltaTime);
            this->m_renMas.render(this->m_scene.m_entities);

            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                auto& uniloc = this->m_shaders.useOverlay();

                this->m_crtlWidget.render(this->m_winWidth, this->m_winHeight, &uniloc);
                this->m_fcounter.render(this->m_winWidth, this->m_winHeight, &uniloc);
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

        dal::ColorView m_red;
        LuaConsole m_luaConsole;

        unsigned m_winWidth, m_winHeight;

    public:
        PauseMenu(const unsigned width, const unsigned height, dal::Managers& managers)
            : m_shaders(managers.m_shaders)
            , m_task(managers.m_taskMas)
            , m_cnxtIngame(nullptr)
            , m_red(nullptr, dal::drawOverlay)
            , m_luaConsole(managers.m_glyph)
            , m_winWidth(width)
            , m_winHeight(height)
        {
            this->m_red.aabb().setPosSize<float>(0, 0, width, height);
            this->m_red.m_color = glm::vec4{ 1, 0, 0, 1 };
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
            this->m_red.render(this->m_winWidth, this->m_winHeight, &uniloc);
            this->m_luaConsole.render(this->m_winWidth, this->m_winHeight, &uniloc);

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            glViewport(0, 0, width, height);

            this->m_red.onParentResize(width, height);
            //this->m_red.aabb().size() = glm::vec2{ width, height };
            this->m_red.aabb().setPosSize<float>(0, 0, width, height);

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

        dal::ColorView m_background;
        dal::LineEdit m_lineedit;

        dal::Timer m_timer;
        unsigned m_winWidth, m_winHeight;

    public:
        TitleScreen(const unsigned width, const unsigned height, dal::Managers& managers)
            : m_shaders(managers.m_shaders)
            , m_task(managers.m_taskMas)
            , m_cnxtIngame(nullptr)
            , m_background(nullptr, dal::drawOverlay)
            , m_lineedit(nullptr, dal::drawOverlay, managers.m_glyph)
            , m_winWidth(width)
            , m_winHeight(height)
        {
            this->m_background.aabb().setPosSize<float>(0, 0, width, height);
            this->m_background.m_color = glm::vec4{ 0.1, 0.1, 0.1, 1 };

            this->m_lineedit.setBGColor(0.1, 0.1, 0.1, 1);
            this->m_lineedit.setText("Little Ruler");
            this->m_lineedit.aabb().setPosSize<float>(30, 30, 5000, 100);
            this->m_lineedit.onUpdateAABB();
        }

        virtual dal::IContext* update(const float deltaTime) override {
            dal::IContext* nextContext = this;

            // Process Input
            {
                {
                    auto& tq = dal::TouchEvtQueueGod::getinst();

                    for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                        const auto& e = tq.at(i);
                        if ( dal::TouchActionType::down == e.m_actionType && this->m_timer.getElapsed() > 1.f ) {
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

                        if ( dal::KeyActionType::down == e.m_actionType && this->m_timer.getElapsed() > 1.f ) {
                            nextContext = this->m_cnxtIngame;
                        }
                    }

                    kq.clear();
                }
            }

            this->m_task.update();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            auto& uniloc = this->m_shaders.useOverlay();
            this->m_background.render(this->m_winWidth, this->m_winHeight, &uniloc);
            this->m_lineedit.render(this->m_winWidth, this->m_winHeight, &uniloc);

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            glViewport(0, 0, width, height);

            this->m_background.onParentResize(width, height);
            this->m_background.aabb().setPosSize<float>(0, 0, width, height);

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

    std::vector<std::unique_ptr<IContext>> initContexts(const unsigned width, const unsigned height, Managers& managers) {
        std::unique_ptr<TitleScreen> title{ new TitleScreen{ width, height, managers } };
        std::unique_ptr<InGameCxt> ingame{ new InGameCxt{ width, height, managers } };
        std::unique_ptr<PauseMenu> pause{ new PauseMenu{ width, height, managers } };

        title->registerContexts(ingame.get());
        ingame->registerContexts(pause.get());
        pause->registerContexts(ingame.get());

        std::vector<std::unique_ptr<IContext>> result;
        result.reserve(3);

        result.push_back(std::unique_ptr<IContext>(title.release()));
        result.push_back(std::unique_ptr<IContext>(ingame.release()));
        result.push_back(std::unique_ptr<IContext>(pause.release()));

        return result;
    }

}
