#include "d_context_main.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <d_phyworld.h>
#include <d_widget_view.h>
#include <d_w_text_view.h>
#include <d_widget_manager.h>
#include <s_configs.h>

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
            this->onUpdateDimens(1);
        }

        virtual void render(const float width, const float height, const void* uniloc) override {
            this->update();
            this->m_label.render(width, height, uniloc);
        }

        virtual void onUpdateDimens(const float scale) override {
            this->m_label.aabb().setAs(this->aabb());
            this->m_label.onUpdateDimens(scale);
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

            this->aabb().setPosSize(10, 10, 128, 128);
            this->onUpdateDimens(1);
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

        virtual void onUpdateDimens(const float scale) override {
            const float INNER_MARGIN = scale * 5;
            const auto lineEditHeight = scale * 20;

            const auto pp1 = this->aabb().point00();
            const auto pp2 = this->aabb().point11();

            this->m_lineEdit.aabb().setPosSize<float>(
                pp1.x + INNER_MARGIN,
                pp2.y - INNER_MARGIN - lineEditHeight,
                this->aabb().width() - INNER_MARGIN - INNER_MARGIN,
                lineEditHeight
                );
            this->m_lineEdit.setMargin(scale * 3);
            this->m_lineEdit.onUpdateDimens(scale);

            this->m_textBox.aabb().setPosSize<float>(
                pp1.x + INNER_MARGIN,
                pp1.y + INNER_MARGIN,
                this->aabb().width() - INNER_MARGIN - INNER_MARGIN,
                this->aabb().height() - lineEditHeight - INNER_MARGIN - INNER_MARGIN - INNER_MARGIN
                );
            this->m_textBox.m_text.m_textSize = this->m_lineEdit.textSize();
            this->m_textBox.onUpdateDimens(scale);

            this->m_bg.aabb().setAs(this->aabb());
            this->m_bg.onUpdateDimens(scale);
        }

        void fetchText(void) {
            this->m_textBox.m_text.addStr(this->m_strbuf.data());
            this->m_strbuf.clear();
        }

    };

}


namespace {

    constexpr float TARGET_CAM_DISTANCE = 3;
    const glm::vec3 TARGET_FOCUS_OFFSET{ 0, 1, 0 };

    void updateCamera(dal::FocusCamera& cam, dal::SceneGraph::CameraProp& camInfo, const dal::MoveInputInfo& moveInfo, const glm::vec3& thisPos) {
        // Apply conrol
        {
            camInfo.m_horizontal += moveInfo.m_view.x;
            camInfo.m_vertical += moveInfo.m_view.y;

            camInfo.m_vertical = glm::clamp(camInfo.m_vertical, glm::radians<float>(-80), glm::radians<float>(80));
        }

        const auto camPosDirec = glm::rotate(glm::mat4{ 1 }, -camInfo.m_horizontal, glm::vec3{ 0, 1, 0 }) *
            glm::rotate(glm::mat4{ 1 }, -camInfo.m_vertical, glm::vec3{ 1, 0, 0 }) * glm::vec4{ 0, 0, -1, 1 };

        cam.setFocusPoint(thisPos + TARGET_FOCUS_OFFSET);
        cam.setPos(cam.focusPoint() + glm::vec3{ camPosDirec } *TARGET_CAM_DISTANCE);
    }

}


namespace {

    class InGameCxt : public dal::IContext {

    private:
        dal::ShaderMaster& m_shaders;
        dal::RenderMaster& m_renMas;
        dal::SceneGraph& m_scene;
        dal::TaskMaster& m_task;
        dal::PhysicsWorld& m_phyworld;
        dal::Config& m_config;

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
            , m_config(managers.m_config)

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

                        if ( this->m_fcounter.aabb().isInside(e.m_pos) && dal::TouchActionType::up == e.m_actionType ) {
                            nextContext = this->m_cnxtPauseMenu;
                            dalVerbose("ingame -> pause");
                        }

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
            }

            const auto winSize = dal::GlobalStateGod::getinst().getWinSizeFloat();
            const auto moveInfo = this->m_crtlWidget.getMoveInfo(deltaTime, winSize.x, winSize.y);

            this->m_scene.m_entities.get<dal::cpnt::CharacterState>(this->m_scene.m_player).update(deltaTime, moveInfo);

            this->m_task.update();
            this->m_phyworld.update(deltaTime);
            this->m_scene.update(deltaTime, this->m_renMas.projMat() * this->m_scene.m_playerCam.viewMat());
            this->m_renMas.update(deltaTime);

            // Camera
            {
                auto& trans = this->m_scene.m_entities.get<dal::cpnt::Transform>(this->m_scene.m_player);
                ::updateCamera(this->m_scene.m_playerCam, this->m_scene.m_playerCamInfo, moveInfo, trans.getPos());
                this->m_scene.m_playerCam.updateViewMat();
            }

            // Render
            {
                this->m_renMas.render(this->m_scene.m_entities);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                auto& uniloc = this->m_shaders.useOverlay();
                this->m_crtlWidget.render(this->m_winWidth, this->m_winHeight, &uniloc);
                this->m_fcounter.render(this->m_winWidth, this->m_winHeight, &uniloc);
            }

            return nextContext;
        }

        virtual void onWinResize(const unsigned width, const unsigned height) override {
            const auto uiScale = this->m_config.m_ui.m_uiScale;

            this->m_fcounter.aabb().setPosSize<float>(10, 10, uiScale * 50, uiScale * 30);
            this->m_fcounter.onUpdateDimens(uiScale);

            this->m_crtlWidget.setSquareLength(uiScale * 20);
            this->m_crtlWidget.onParentResize(width, height);
            this->m_crtlWidget.onUpdateDimens(uiScale);

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
        dal::Config& m_config;

        // Contexts
        dal::IContext* m_cnxtIngame;

        dal::ColorView m_red;
        LuaConsole m_luaConsole;

        unsigned m_winWidth, m_winHeight;

    public:
        PauseMenu(const unsigned width, const unsigned height, dal::Managers& managers)
            : m_shaders(managers.m_shaders)
            , m_task(managers.m_taskMas)
            , m_config(managers.m_config)

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

            const auto uiScale = this->m_config.m_ui.m_uiScale;

            //this->m_red.aabb().size() = glm::vec2{ width, height };
            this->m_red.aabb().setPosSize<float>(0, 0, width, height);

            this->m_luaConsole.aabb().setPosSize<float>(10, 50, uiScale * 600, uiScale * 480);
            this->m_luaConsole.onUpdateDimens(uiScale);

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
            this->m_lineedit.onUpdateDimens(managers.m_config.m_ui.m_uiScale);
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

            this->m_background.aabb().setPosSize<float>(0, 0, width, height);

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
