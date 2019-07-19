#pragma once

#include <utility>

#include <glm/glm.hpp>

#include "p_uniloc.h"
#include "s_input_queue.h"


namespace dal {

    enum class InputCtrlFlag { ignored, consumed, owned };


    class IScreenSpaceBox {
        // Upper left corner is (0, 0)
    private:
        // m_pos represents upper left point of the box.
        glm::vec2 m_pos, m_size{ 1.0f, 1.0f };

    public:
        virtual ~IScreenSpaceBox(void) = default;

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

        std::pair<glm::vec2, glm::vec2> makeDeviceSpace(const float width, const float height) const;

    protected:
        virtual void onScrSpaceBoxUpdate(void) {};

    };


    class Widget2 : public IScreenSpaceBox {

    private:
        Widget2* m_parent;
        bool m_flagDraw;

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
        bool getFlagDraw(void) const {
            return this->m_flagDraw;
        }
        void setFlagDraw(const bool x) {
            this->m_flagDraw = x;
        }

        virtual void render(const UnilocOverlay& uniloc, const float width, const float height) {}
        virtual InputCtrlFlag onTouch(const TouchEvent& e) {
            return InputCtrlFlag::ignored;
        }
        virtual InputCtrlFlag onKeyInput(const KeyboardEvent& e) {
            return InputCtrlFlag::ignored;
        }
        virtual void onParentResize(const float width, const float height) {}

    };

}