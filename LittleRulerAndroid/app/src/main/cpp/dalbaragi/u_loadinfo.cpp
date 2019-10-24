#include "u_loadinfo.h"

#include <fmt/format.h>

#include "s_logger_god.h"


namespace dal::binfo {

    bool ImageFileData::checkValidity(void) const {
        if ( (this->m_width * this->m_height * this->m_pixSize) != this->m_buf.size() ) {
            dalWarn(fmt::format("Invalid dimension -> {} * {} * {} =/= {}", this->m_width, this->m_height, this->m_pixSize, this->m_buf.size()));
            return false;
        }
        else {
            return true;
        }
    }

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

    void ImageFileData::rotate90(void) {
        const auto widthInBytes = this->m_width * this->m_pixSize;

        std::vector<uint8_t> newBuf;
        newBuf.reserve(this->m_buf.size());

        for ( size_t w = 0; w < this->m_width; ++w ) {
            for ( size_t h = 0; h < this->m_height; ++h ) {
                const auto hf = this->m_height - h - 1;
                const auto index = w * this->m_pixSize + hf * widthInBytes;
                for ( size_t k = 0; k < this->m_pixSize; ++k ) {
                    newBuf.push_back(this->m_buf[index + k]);
                }
            }
        }

        std::swap(this->m_buf, newBuf);
        std::swap(this->m_width, this->m_height);
    }

    void ImageFileData::rotate180(void) {
        const auto widthInBytes = this->m_width * this->m_pixSize;

        std::vector<uint8_t> newBuf;
        newBuf.reserve(this->m_buf.size());

        for ( size_t h = 0; h < this->m_height; ++h ) {
            const auto hf = this->m_height - h - 1;
            for ( size_t w = 0; w < this->m_width; ++w ) {
                const auto wf = this->m_width - w - 1;
                const auto index = wf * this->m_pixSize + hf * widthInBytes;
                for ( size_t k = 0; k < this->m_pixSize; ++k ) {
                    newBuf.push_back(this->m_buf[index + k]);
                }
            }
        }

        std::swap(this->m_buf, newBuf);
    }

    void ImageFileData::rotate270(void) {
        const auto widthInBytes = this->m_width * this->m_pixSize;

        std::vector<uint8_t> newBuf;
        newBuf.reserve(this->m_buf.size());

        for ( size_t w = 0; w < this->m_width; ++w ) {
            const auto wf = this->m_width - w - 1;
            for ( size_t h = 0; h < this->m_height; ++h ) {
                const auto index = wf * this->m_pixSize + h * widthInBytes;
                for ( size_t k = 0; k < this->m_pixSize; ++k ) {
                    newBuf.push_back(this->m_buf[index + k]);
                }
            }
        }

        std::swap(this->m_buf, newBuf);
        std::swap(this->m_width, this->m_height);
    }

    bool ImageFileData::hasTransparency(void) const {
        if ( 4 == this->m_pixSize ) {
            const auto numPixels = this->m_width * this->m_height;
            for ( size_t i = 0; i < numPixels; ++i ) {
                const auto alphaValue = this->m_buf[4 * i + 3];
                if ( alphaValue != 255 ) {
                    return true;
                }
            }
        }

        return false;
    }

    void ImageFileData::correctSRGB(void) {
        const auto numPixels = this->m_width * this->m_height;
        const auto pixSize = this->m_pixSize < 3 ? this->m_pixSize : 3;

        for ( size_t i = 0; i < numPixels; ++i ) {
            for ( size_t j = 0; j < pixSize; ++j ) {
                const auto r = static_cast<double>(this->m_buf[this->m_pixSize * i + j]) / 256.0;
                this->m_buf[this->m_pixSize * i + j] = static_cast<uint8_t>(std::pow(r, 2.2) * 256.0);
            }
        }
    }

}
