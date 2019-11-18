#include "d_context_main.h"

#include "o_widget_textbox.h"
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

    } g_fpsCounter;

}


namespace dal {

    InGameCxt::InGameCxt(ShaderMaster& shaders, RenderMaster& renMas, SceneGraph& scene,
        const unsigned width, const unsigned height)
        : m_shaders(shaders)
        , m_renMas(renMas)
        , m_scene(scene)
        , m_crtlWidget(static_cast<float>(width), static_cast<float>(height))
        , m_winWidth(width)
        , m_winHeight(height)
    {

    }

    InGameCxt::~InGameCxt(void) {

    }

    IContext* InGameCxt::update(const float deltaTime) {
        // FPS counter
        {
            
        }

        // Process Input
        {
            {
                auto& tq = dal::TouchEvtQueueGod::getinst();

                for ( unsigned int i = 0; i < tq.getSize(); i++ ) {
                    const auto& e = tq.at(i);
                    this->m_crtlWidget.onTouch(e);
                }  // for

                tq.clear();
            }

            {
                auto& kq = dal::KeyboardEvtQueueGod::getinst();
                const auto kqSize = kq.getSize();

                for ( unsigned int i = 0; i < kqSize; ++i ) {
                    const auto& e = kq.at(i);
                    this->m_crtlWidget.onKeyInput(e, kq.getKeyStates());
                }

                kq.clear();
            }

            {
                auto& state = this->m_scene.m_entities.get<cpnt::CharacterState>(this->m_scene.m_player);
                const auto winSize = GlobalStateGod::getinst().getWinSizeFloat();
                const auto info = this->m_crtlWidget.getMoveInfo(deltaTime, winSize.x, winSize.y);
                state.update(deltaTime, info);
            }
        }

        TaskGod::getinst().update();

        this->m_scene.update(deltaTime);
        this->m_renMas.update(deltaTime);
        this->m_renMas.render(this->m_scene.m_entities);

        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            auto& uniloc = this->m_shaders.useOverlay();

            this->m_crtlWidget.render(uniloc, this->m_winWidth, this->m_winHeight);
            g_fpsCounter.render(uniloc, this->m_winWidth, this->m_winHeight);
        }

        return this;
    }

    void InGameCxt::onWinResize(const unsigned width, const unsigned height) {
        this->m_crtlWidget.onParentResize(width, height);
        this->m_winWidth = width;
        this->m_winHeight = height;
    }

}
