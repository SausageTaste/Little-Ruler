#pragma once

#include <string>
#include <utility>

#include <glm/glm.hpp>

#include "p_meshStatic.h"
#include "p_uniloc.h"
#include "s_input_queue.h"
//#include "u_timer.h"


namespace dal {

    glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight);
    glm::vec2 screen2device(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight);
    glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight);
    glm::vec2 device2screen(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight);
    glm::vec2 size2device(const glm::vec2& size, const glm::vec2& parentSize);


    struct QuadRenderInfo {
        glm::vec4 m_color{ 1.0f };
        glm::vec2 m_bottomLeftNormalized;
        glm::vec2 m_rectSize;
        glm::vec2 m_texOffset{ 0.f, 0.f };
        glm::vec2 m_texScale{ 1.0f, 1.0f };
        const Texture* m_diffuseMap = nullptr;
        const Texture* m_maskMap = nullptr;
        bool m_upsideDown_diffuse = false;
        bool m_upsideDown_mask = false;
    };

    void renderQuadOverlay(const UnilocOverlay& uniloc, const QuadRenderInfo& info);


    enum class InputCtrlFlag { ignored, consumed, owned };


    class IScreenSpaceBox {
        // Upper left corner is (0, 0)
    private:
        // m_pos represents upper left point of the box.
        glm::vec2 m_pos, m_size{ 1.0f, 1.0f };

    public:
        virtual ~IScreenSpaceBox(void) = default;

        void copy(const IScreenSpaceBox& other) {
            this->m_pos = other.m_pos;
            this->m_size = other.m_size;
        }

        float getPosX(void) const {
            return this->m_pos.x;
        }
        float getPosY(void) const {
            return this->m_pos.y;
        }
        float getWidth(void) const {
            return this->m_size.x;
        }
        float getHeight(void) const {
            return this->m_size.y;
        }
        glm::vec2 getPos(void) const {
            return this->m_pos;
        }
        glm::vec2 getSize(void) const {
            return this->m_size;
        }

        // 00 is upper left, 10 is upper right, 11 is below right.
        glm::vec2 getPoint00(void) const {
            return this->getPos();
        }
        glm::vec2 getPoint01(void) const {
            return glm::vec2{
                this->getPosX(),
                this->getPosY() + this->getHeight()
            };
        }
        glm::vec2 getPoint10(void) const {
            return glm::vec2{
                this->getPosX() + this->getWidth(),
                this->getPosY()
            };
        }
        glm::vec2 getPoint11(void) const {
            return glm::vec2{
                this->getPosX() + this->getWidth(),
                this->getPosY() + this->getHeight()
            };
        }

        void setPosX(const float x) {
            this->m_pos.x = x;
            this->onScrSpaceBoxUpdate();
        }
        void setPosY(const float y) {
            this->m_pos.y = y;
            this->onScrSpaceBoxUpdate();
        }
        void setWidth(const float x) {
            this->m_size.x = x;
            this->onScrSpaceBoxUpdate();
        }
        void setHeight(const float y) {
            this->m_size.y = y;
            this->onScrSpaceBoxUpdate();
        }
        void setPos(const float x, const float y) {
            this->m_pos.x = x;
            this->m_pos.y = y;
            this->onScrSpaceBoxUpdate();
        }
        void setPos(const glm::vec2& v) {
            this->setPos(v.x, v.y);
        }
        void setSize(const float x, const float y) {
            this->m_size.x = x;
            this->m_size.y = y;
            this->onScrSpaceBoxUpdate();
        }
        void setSize(const glm::vec2& v) {
            this->setSize(v.x, v.y);
        }

        bool isPointInside(const float x, const float y) const;
        bool isPointInside(const glm::vec2& v) const {
            return this->isPointInside(v.x, v.y);
        }

        std::pair<glm::vec2, glm::vec2> makePosSize(const float width, const float height) const;

    protected:
        virtual void onScrSpaceBoxUpdate(void) {};

    };


    class Widget2 : public IScreenSpaceBox {

    private:
        Widget2* m_parent;

    public:
        Widget2(const Widget2&) = delete;
        Widget2& operator=(const Widget2&) = delete;

    public:
        Widget2(Widget2* const parent);

        Widget2* getParent(void) {
            return this->m_parent;
        }
        const Widget2* getParent(void) const {
            return this->m_parent;
        }

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) {}
        virtual InputCtrlFlag onTouch(const TouchEvent& e) {
            return InputCtrlFlag::ignored;
        }
        virtual InputCtrlFlag onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) {
            return InputCtrlFlag::ignored;
        }
        virtual void onParentResize(const float width, const float height) {}
        virtual void onFocusChange(const bool v) {}

    };

}
