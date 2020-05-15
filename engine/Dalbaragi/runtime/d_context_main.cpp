#include "d_context_main.h"

#include <fmt/format.h>

#include <d_logger.h>
#include <d_phyworld.h>
#include <d_widget_view.h>

#include "p_render_master.h"
#include "c_input_apply.h"
#include "o_widget_textbox.h"
#include "o_widgetmanager.h"
#include "u_timer.h"
#include "u_luascript.h"
#include "d_overlay_interface.h"
#include "d_text_overlay.h"


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

        virtual void render(const dal::UniRender_Overlay& uniloc, const float width, const float height) override {
            this->update();
            this->m_label.render(uniloc, width, height);
        }

    private:
        void update(void) {
            ++this->m_frameAccum;
            const auto elapsedForFPS = this->m_timerForFPSReport.getElapsed();
            if ( elapsedForFPS > 0.05f ) {
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

    } g_fcounter;


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

        virtual void render(const dal::UniRender_Overlay& uniloc, const float width, const float height) override {
            dal::QuadRenderInfo info;
            std::tie(info.m_bottomLeftNormalized, info.m_rectSize) = this->makePosSize(width, height);
            info.m_color = this->m_bgColor;
            dal::renderQuadOverlay(uniloc, info);

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
        dal::PhysicsWorld& m_phyworld;

        // Contexts
        dal::IContext* m_cnxtPauseMenu;

        dal::PlayerControlWidget m_crtlWidget;
        dal::TextOverlay m_testText;

        unsigned m_winWidth, m_winHeight;

    public:
        InGameCxt(
            const unsigned width, const unsigned height,
            dal::ShaderMaster& shaders, dal::RenderMaster& renMas, dal::SceneGraph& scene, dal::TaskMaster& taskMas, dal::PhysicsWorld& phyworld, dal::GlyphMaster& glyph
        )
            : m_shaders(shaders)
            , m_renMas(renMas)
            , m_scene(scene)
            , m_task(taskMas)
            , m_phyworld(phyworld)
            , m_cnxtPauseMenu(nullptr)
            , m_crtlWidget(static_cast<float>(width), static_cast<float>(height))
            , m_testText(nullptr, glyph, dal::drawOverlay)
            , m_winWidth(width)
            , m_winHeight(height)
        {
            this->m_testText.aabb().setPosSize(30, 60, 256, 128);
            this->m_testText.addStr("Bonjour\n");
            this->m_testText.addStr("\xec\x95\x88\xeb\x85\x95\xed\x95\x98\xec\x84\xb8\xec\x9a\x94\n");  // Korean
            this->m_testText.addStr("\xce\xa7\xce\xb1\xce\xaf\xcf\x81\xce\xb5\xcf\x84\xce\xb5\n");  // Greek
            this->m_testText.addStr("\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf\n");  // Japanese
            this->m_testText.addStr("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\n");  // Russian
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

            this->m_phyworld.update(deltaTime);
            this->m_scene.update(deltaTime);
            this->m_renMas.update(deltaTime);
            this->m_renMas.render(this->m_scene.m_entities);

            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                auto& uniloc = this->m_shaders.useOverlay();

                this->m_crtlWidget.render(this->m_winWidth, this->m_winHeight, &uniloc);
                this->m_testText.render(this->m_winWidth, this->m_winHeight, reinterpret_cast<const void*>(&uniloc));
                g_fcounter.render(uniloc, this->m_winWidth, this->m_winHeight);
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
        PauseMenu(const unsigned width, const unsigned height, dal::ShaderMaster& shaders, dal::TaskMaster& taskMas, dal::GlyphMaster& glyph)
            : m_shaders(shaders)
            , m_task(taskMas)
            , m_cnxtIngame(nullptr)
            , m_red(nullptr, dal::drawOverlay)
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
            this->m_luaConsole.render(uniloc, this->m_winWidth, this->m_winHeight);
            g_fcounter.render(uniloc, this->m_winWidth, this->m_winHeight);

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
        TitleScreen(const unsigned width, const unsigned height, dal::ShaderMaster& shaders, dal::TaskMaster& taskMas, dal::GlyphMaster& glyph)
            : m_shaders(shaders)
            , m_task(taskMas)
            , m_cnxtIngame(nullptr)
            , m_background(nullptr, dal::drawOverlay)
            , m_lineedit(nullptr)
            , m_winWidth(width)
            , m_winHeight(height)
        {
            this->m_background.aabb().setPosSize<float>(0, 0, width, height);
            this->m_background.m_color = glm::vec4{ 0.1, 0.1, 0.1, 1 };

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
            this->m_lineedit.render(uniloc, this->m_winWidth, this->m_winHeight);

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

    std::vector <std::unique_ptr<IContext>> initContexts(const unsigned width, const unsigned height,
        ShaderMaster& shaders, RenderMaster& renMas, SceneGraph& scene, TaskMaster& taskMas, PhysicsWorld& phyworld, GlyphMaster& glyph)
    {
        std::vector <std::unique_ptr<IContext>> result;

        std::unique_ptr<TitleScreen> title{ new TitleScreen{ width, height, shaders, taskMas, glyph } };
        std::unique_ptr<InGameCxt> ingame{ new InGameCxt{ width, height, shaders, renMas, scene, taskMas, phyworld, glyph } };
        std::unique_ptr<PauseMenu> pause{ new PauseMenu{ width, height, shaders, taskMas, glyph } };

        title->registerContexts(ingame.get());
        ingame->registerContexts(pause.get());
        pause->registerContexts(ingame.get());

        result.push_back(std::unique_ptr<IContext>{ title.release() });
        result.push_back(std::unique_ptr<IContext>{ ingame.release() });
        result.push_back(std::unique_ptr<IContext>{ pause.release() });

        return result;
    }

}
