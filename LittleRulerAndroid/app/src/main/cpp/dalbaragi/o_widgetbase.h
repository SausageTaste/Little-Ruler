#pragma once

#include <utility>

#include <glm/glm.hpp>


namespace dal {

    class ScreenSpaceBox {
        // Upper left corner is (0, 0)
    private:
        // m_pos represents upper left point of the box.
        glm::vec2 m_pos, m_size{ 1.0f, 1.0f };

    public:
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
        }
        void setPosY(const float y) {
            this->m_pos.y = y;
        }
        void setWidth(const float x) {
            this->m_size.x = x;
        }
        void setHeight(const float y) {
            this->m_size.y = y;
        }
        void setPos(const float x, const float y) {
            this->setPosX(x);
            this->setPosY(y);
        }
        void setPos(const glm::vec2& v) {
            this->setPos(v.x, v.y);
        }
        void setSize(const float x, const float y) {
            this->setWidth(x);
            this->setHeight(y);
        }
        void setSize(const glm::vec2& v) {
            this->setSize(v.x, v.y);
        }

        std::pair<glm::vec2, glm::vec2> makeDeviceSpace(const unsigned int winWidth, const unsigned int winHeight) const;

    };


    class Widget2 {

    private:
        Widget2* m_parent;
        glm::vec2 m_pos, m_size;

    public:


    };

}