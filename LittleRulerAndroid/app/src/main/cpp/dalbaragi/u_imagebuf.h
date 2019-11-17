#pragma once

#include <vector>


namespace dal {

    class ImageFileData {

    private:
        std::vector<uint8_t> m_buf;
        size_t m_width = 0, m_height = 0, m_pixSize = 0;

    public:
        // Getters
        auto& vector(void) {
            return this->m_buf;
        }
        auto& vector(void) const {
            return this->m_buf;
        }
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

        // Setters
        void setDimensions(const size_t width, const size_t height, const size_t pixSize) {
            this->m_width = width;
            this->m_height = height;
            this->m_pixSize = pixSize;
        }

        // Calc info
        bool checkValidity(void) const;
        bool hasTransparency(void) const;

        // Manipulate
        void flipX(void);
        void flipY(void);

        void rotate90(void);
        void rotate180(void);
        void rotate270(void);

        void correctSRGB(void);

    };

}
