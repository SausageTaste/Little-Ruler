#pragma once

#include <glm/glm.hpp>


namespace dal {

    template <typename T>
    class AABB_2D {

    private:
        glm::tvec2<T> m_pos{ 0 };
        glm::tvec2<T> m_size{ 0 };  // Elements of m_size are always positive numbers

    public:
        auto pos(void) -> glm::tvec2<T>& {
            return this->m_pos;
        }
        auto pos(void) const -> const glm::tvec2<T>& {
            return this->m_pos;
        }
        auto size(void) -> glm::tvec2<T>& {
            return this->m_size;
        }
        auto size(void) const -> const glm::tvec2<T>& {
            return this->m_size;
        }

        auto width(void) const -> T {
            return this->size().x;
        }
        auto height(void) const -> T {
            return this->size().y;
        }

        auto point00(void) const -> glm::tvec2<T> {
            return this->pos();
        }
        auto point01(void) const -> glm::tvec2<T> {
            return glm::vec2{
                this->pos().x,
                this->pos().y + this->height()
            };
        }
        auto point10(void) const -> glm::tvec2<T> {
            return glm::vec2{
                this->pos().x + this->width(),
                this->pos().y
            };
        }
        auto point11(void) const -> glm::tvec2<T> {
            return this->pos() + this->size();
        }

        template <typename U>
        void setPosSize(const glm::tvec2<U>& pos, const glm::tvec2<U>& size) {
            this->m_pos = pos;
            this->m_size = size;
            this->validate();
        }
        template <typename U>
        void setAs(const AABB_2D<U>& other) {
            this->m_pos = other.pos();
            this->m_size = other.size();
        }
        template <typename U>
        void setPoints(const glm::tvec2<U>& p0, const glm::tvec2<U>& p1) {
            this->m_pos = p0;
            this->m_size = p1 - p0;
            this->validate();
        }

        auto isInside(const float x, const float y) const -> bool {
            const auto p1 = this->point00();
            const auto p2 = this->point11();

            if ( x < p1.x )
                return false;
            if ( y < p1.y )
                return false;
            if ( x > p2.x )
                return false;
            if ( y > p2.y )
                return false;

            return true;
        }
        auto isInside(const glm::vec2& v) const -> bool {
            return this->isInside(v.x, v.y);
        }

    private:
        void validate(void) {
            // it fails for unsigned number types
            static_assert(static_cast<T>(-1) < static_cast<T>(0));

            for ( int i = 0; i < 2; ++i ) {
                if ( this->m_size[i] < static_cast<T>(0) ) {
                    this->m_pos[i] += this->m_size[i];
                    this->m_size[i] *= static_cast<T>(-1);
                }
            }
        }

    };

}
