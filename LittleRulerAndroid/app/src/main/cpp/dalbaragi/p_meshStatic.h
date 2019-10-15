#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_dalopengl.h"
#include "p_uniloc.h"
#include "u_loadinfo.h"


// Meshes
namespace dal {

    template <unsigned int _NumBuffs>
    class IMesh {

    private:
        GLuint m_vao = 0;
        GLuint m_buffers[_NumBuffs] = { 0 };  // vertices, texcoords, normals, bone ids, weights
        size_t m_numVertices = 0;

    public:
        IMesh(const IMesh&) = delete;
        IMesh& operator=(const IMesh&) = delete;

    public:
        IMesh(void) = default;
        IMesh(IMesh&& other) noexcept
            : m_vao(other.m_vao)
            , m_numVertices(other.m_numVertices)
        {
            for ( unsigned int i = 0; i < _NumBuffs; ++i ) {
                this->m_buffers[i] = other.m_buffers[i];
            }

            other.setAllToZero();
        }
        IMesh& operator=(IMesh&& other) noexcept {
            this->invalidate();

            this->m_vao = other.m_vao;
            this->m_numVertices = other.m_numVertices;
            for ( unsigned int i = 0; i < _NumBuffs; ++i ) {
                this->m_buffers[i] = other.m_buffers[i];
            }

            other.setAllToZero();
            return *this;
        }
        ~IMesh(void) {
            this->invalidate();
        }

        void draw(void) const {
#ifdef _DEBUG
            if ( !this->isReady() ) {
                throw std::runtime_error{ "MeshStatic::renderDepthmap called without being built." };
            }
#endif
            this->bindVAO();
            glDrawArrays(GL_TRIANGLES, 0, this->m_numVertices);
            this->unbindVAO();
        }
        bool isReady(void) const {
            return this->m_numVertices != 0;
        }

    protected:
        template <unsigned int _Index, decltype(GL_FLOAT) _GLType>
        void fillBufferData(const void* const arr, const size_t arraySize, const size_t tupleSize) {
            static_assert(_Index < _NumBuffs);
            static_assert(GL_FLOAT == _GLType || GL_INT == _GLType);

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[_Index]);
            glBufferData(GL_ARRAY_BUFFER, arraySize, arr, GL_STATIC_DRAW);

            if ( GL_FLOAT == _GLType ) {
                glVertexAttribPointer(_Index, tupleSize, GL_FLOAT, GL_FALSE, 0, nullptr);
            }
            else if ( GL_INT == _GLType ) {
                glVertexAttribIPointer(_Index, tupleSize, GL_INT, 0, nullptr);
            }

            glEnableVertexAttribArray(_Index);
        }

        void setNumVert(const size_t v) {
            this->m_numVertices = v;
        }

        void createBuffers(void) {
            glGenVertexArrays(1, &this->m_vao);
            if ( this->m_vao <= 0 ) {
                throw std::runtime_error{ "Failed to generate vertex array." };
            }

            glGenBuffers(_NumBuffs, this->m_buffers);
            for ( int i = 0; i < _NumBuffs; i++ ) {
                if ( 0 == this->m_buffers[i] ) {
                    throw std::runtime_error{ "Failed to generate beffer." };
                }
            }
        }
        void invalidate(void) {
            if ( !this->isReady() ) {
                return;
            }

            glDeleteBuffers(_NumBuffs, this->m_buffers);
            for ( int i = 0; i < _NumBuffs; i++ ) {
                this->m_buffers[i] = 0;
            }

            glDeleteVertexArrays(1, &this->m_vao);
            this->m_vao = 0;

            this->m_numVertices = 0;
        }
        void setAllToZero(void) {
            this->m_vao = 0;
            this->m_numVertices = 0;
            for ( unsigned int i = 0; i < _NumBuffs; ++i ) {
                this->m_buffers[i] = 0;
            }
        }

        void bindVAO(void) const {
            glBindVertexArray(this->m_vao);
        }
        static void unbindVAO(void) {
            glBindVertexArray(0);
        }

    };


    class MeshStatic : public IMesh<3> {

    public:
        int buildData(const float* const vertices, const float* const texcoords, const float* const normals, const size_t numVertices);

    };


    class MeshAnimated : public IMesh<5> {

    public:
        void buildData(const float* const vertices, const float* const texcoords, const float* const normals,
            const int32_t* const boneids, const float* const weights, const size_t numVertices);

    };

}


// Textures
namespace dal {

    class ITexture {

    private:
        GLuint m_texID = 0;
        bool m_hasAlpha = false;

    public:
        ITexture(const ITexture&) = delete;
        ITexture& operator=(const ITexture&) = delete;

    public:
        ITexture(void) = default;
        ITexture(const GLuint id);
        ITexture(ITexture&& other) noexcept;
        ITexture& operator=(ITexture&& other) noexcept;
        ~ITexture(void);

        void invalidate(void);
        void reset(const GLuint id);
        bool isReady(void) const;
        GLuint get(void) const noexcept {
            return m_texID;
        }
        bool hasAlpha(void) const {
            return this->m_hasAlpha;
        }

    protected:
        void genTexture(const char* const str4Log);
        void setHasAlpha(const bool v) {
            this->m_hasAlpha = v;
        }

    };


    class Texture : public ITexture {

    public:
        void init_diffuseMap(binfo::ImageFileData& image);
        void init_depthMap(const unsigned int width, const unsigned int height);
        void init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
        void initAttach_colorMap(const unsigned int width, const unsigned int height);

        void resize_colorMap(const unsigned int width, const unsigned int height);

        void sendUniform(const GLint uniloc_sampler, const GLint uniloc_has, const unsigned int index) const;
        void sendUniform(const SamplerInterf& uniloc) const;

    };


    class CubeMap : public ITexture {

    public:
        class CubeMapData {

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
        void init(const CubeMapData& data);

        void sendUniform(const SamplerInterf& uniloc) const;

    };

}


// etc
namespace dal {

    class Material {

    public:
        float m_roughness;
        float m_metallic;

        glm::vec2 m_texScale;
        std::shared_ptr<const Texture> m_diffuseMap, m_roughnessMap, m_metallicMap;

    public:
        Material(void);

        void sendUniform(const UniInterfLightedMesh& uniloc) const;
        void sendUniform(const UniInterfLightedMesh& unilocLight, const SamplerInterf& samplerInterf) const;

        bool hasDiffuseAlpha(void) const {
            if ( nullptr == this->m_diffuseMap ) {
                return false;
            }
            else {
                return this->m_diffuseMap->hasAlpha();
            }
        }

    };

}
