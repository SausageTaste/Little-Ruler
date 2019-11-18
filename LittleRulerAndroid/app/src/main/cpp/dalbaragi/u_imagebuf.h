#pragma once

#include <vector>

#include <glm/glm.hpp>


namespace dal {

    class ImageFileData {

    private:
        std::vector<uint8_t> m_buf;
        size_t m_width = 0, m_height = 0, m_pixSize = 0;

    public:
        // Getters
        auto data(void) {
            return this->m_buf.data();
        }
        auto data(void) const {
            return this->m_buf.data();
        }
        auto size(void) const {
            return this->m_buf.size();
        }

        auto width(void) const {
            return this->m_width;
        }
        auto height(void) const {
            return this->m_height;
        }
        auto pixSize(void) const {
            return this->m_pixSize;
        }

        glm::vec4 rgba(const size_t x, const size_t y) const;
        glm::vec4 rgba_f(const float x, const float y) const;

        // Setters
        bool set(const size_t width, const size_t height, const size_t pixSize, std::vector<uint8_t>&& buffer) {
            this->m_width = width;
            this->m_height = height;
            this->m_pixSize = pixSize;
            this->m_buf = std::move(buffer);

            return this->checkValidity();
        }

        // Calc info
        bool hasTransparency(void) const;

        // Manipulate
        void flipX(void);
        void flipY(void);

        void rotate90(void);
        void rotate180(void);
        void rotate270(void);

        void correctSRGB(void);

    private:
        bool checkValidity(void) const;
        size_t pixOffset(const size_t x, const size_t y) const {
            return y * this->width() + x;
        }

    };

}
