#include "u_loadinfo.h"

#include "s_logger_god.h"


namespace dal::binfo {

    void ImageFileData::flipX(void) {
        const auto lineSizeInBytes = this->m_width * this->m_pixSize;
        dalAssert((lineSizeInBytes * this->m_height) == this->m_buf.size());
        std::vector<uint8_t> flipped;
        flipped.reserve(this->m_buf.size());

        for ( size_t i = 0; i < this->m_height; ++i ) {
            for ( size_t j = 0; j < this->m_width; ++j ) {
                const auto index = lineSizeInBytes * i + (this->m_width - j - 1) * this->m_pixSize;
                for ( size_t k = 0; k < this->m_pixSize; ++k ) {
                    flipped.push_back(this->m_buf[index + k]);
                }
            }
        }

        std::swap(this->m_buf, flipped);
    }

    void ImageFileData::flipY(void) {
        const auto lineSizeInBytes = this->m_width * this->m_pixSize;
        dalAssert((lineSizeInBytes * this->m_height) == this->m_buf.size());
        std::vector<uint8_t> flipped;
        flipped.reserve(this->m_buf.size());

        for ( size_t i = 0; i < this->m_height; ++i ) {
            const auto flippedHeight = this->m_height - i - 1;
            for ( size_t j = 0; j < lineSizeInBytes; ++j ) {
                const auto index = lineSizeInBytes * flippedHeight + j;
                flipped.push_back(this->m_buf[index]);
            }
        }

        std::swap(this->m_buf, flipped);
    }

}