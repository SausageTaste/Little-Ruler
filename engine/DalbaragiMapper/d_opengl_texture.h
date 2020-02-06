#pragma once

#include <u_imagebuf.h>

#include "d_opengl.h"
#include "d_uniloc.h"


namespace dal::gl {

    class ITexture {

    private:
        gl::uint_t m_texID = 0;

    public:
        ITexture(const ITexture&) = delete;
        ITexture& operator=(const ITexture&) = delete;

    public:
        ITexture(void) = default;
        ITexture(const gl::uint_t id);
        ITexture(ITexture&& other) noexcept;
        ITexture& operator=(ITexture&& other) noexcept;
        ~ITexture(void);

        void invalidate(void);
        void reset(const gl::uint_t id);
        bool isReady(void) const;
        gl::uint_t get(void) const noexcept {
            return this->m_texID;
        }

    protected:
        void genTexture(const char* const str4Log);

    };


    class Texture : public ITexture {

    public:
        void init_image(ImageData& image);

        void sendUniform(const SamplerInterf& uniloc) const;

    };


    class CubeMap : public ITexture {

    public:
        class BuildInfo {

        private:
            const uint8_t* m_buffers[6];
            unsigned int m_widthes[6], m_heights[6], m_pixSize[6];

        public:
            std::tuple<const uint8_t*, unsigned int, unsigned int, unsigned int> at(const size_t index) const {
                return std::make_tuple(this->m_buffers[index], this->m_widthes[index], this->m_heights[index], this->m_pixSize[index]);
            }

            void setRight(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->set<0>(buf, width, height, pixSize);
            }
            void setLeft(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->set<1>(buf, width, height, pixSize);
            }
            void setTop(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->set<2>(buf, width, height, pixSize);
            }
            void setButtom(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->set<3>(buf, width, height, pixSize);
            }
            void setBack(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->set<4>(buf, width, height, pixSize);
            }
            void setFront(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->set<5>(buf, width, height, pixSize);
            }

            template <size_t _Index>
            void set(const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->m_buffers[_Index] = buf;
                this->m_widthes[_Index] = width;
                this->m_heights[_Index] = height;
                this->m_pixSize[_Index] = pixSize;
            }

            void set(const size_t index, const uint8_t* const buf, const unsigned int width, const unsigned int height, const unsigned int pixSize) {
                this->m_buffers[index] = buf;
                this->m_widthes[index] = width;
                this->m_heights[index] = height;
                this->m_pixSize[index] = pixSize;
            }

        };

    public:
        void init(const BuildInfo& data);
        void initAttach_colorMap(const unsigned width, const unsigned height);

        void sendUniform(const SamplerInterf& uniloc) const;

    };

}
