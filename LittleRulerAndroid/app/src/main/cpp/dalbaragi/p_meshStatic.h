#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_dalopengl.h"
#include "p_uniloc.h"


namespace dal {

    class MeshStatic {
        //////// Variables ////////

    private:
        GLuint mVao = 0;

        GLuint mVertexArrayBuffer = 0;
        GLuint mTexCoordArrayBuffer = 0;
        GLuint mNormalArrayBuffe = 0;

        unsigned int mVertexSize = 0;

        //////// Functions ////////

    public:
        MeshStatic(const MeshStatic&) = delete;
        MeshStatic& operator=(const MeshStatic&) = delete;

    public:
        MeshStatic(void) = default;
        MeshStatic(MeshStatic&& other) noexcept;
        MeshStatic& operator=(MeshStatic&& other) noexcept;
        ~MeshStatic(void);

        void draw(void) const;

        int buildData(
            const float* const vertices, const int vertSize,
            const float* const texcors, const int texcorSize,
            const float* const normals, const int norSize
        );
        void invalidate(void);
        bool isReady(void) const;

    private:
        void createBuffers(void);
        void bindVAO(void) const;
        static void unbindVAO(void);

        void setAllToZero(void);

    };


    class MeshAnimated {

    private:
        GLuint m_vao = 0;
        GLuint m_buffers[5] = { 0 };
        // vertices, texcoords, normals, bone ids, weights

        unsigned int m_numVertices = 0;

    public:
        MeshAnimated(const MeshAnimated&) = delete;
        MeshAnimated& operator=(const MeshAnimated&) = delete;

    public:
        MeshAnimated(void) = default;
        MeshAnimated(MeshAnimated&& other) noexcept;
        MeshAnimated& operator=(MeshAnimated&& other) noexcept;
        ~MeshAnimated(void);

    public:
        void draw(void) const;

        void buildData(const float* const vertices, const float* const texcors, const float* const normals,
            const int32_t* const boneids, const float* const weights, const size_t numVertices);
        void invalidate(void);
        bool isReady(void) const;

    private:
        void createBuffers(void);
        void bindVAO(void) const;
        static void unbindVAO(void);

        void setAllToZero(void);

    };


    class Texture {

        //////// Attribs ////////

    private:
        GLuint m_texID = 0;

        //////// Methods ////////

    public:
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

    public:
        Texture(void) = default;
        Texture(const GLuint id);

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        ~Texture(void);

        void init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
        void init_diffueMap3(const uint8_t* const image, const unsigned int width, const unsigned int height);
        void init_depthMap(const unsigned int width, const unsigned int height);
        void init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
        void initAttach_colorMap(const unsigned int width, const unsigned int height);

        void invalidate(void);
        void reset(const GLuint id);
        bool isReady(void) const;
        GLuint get(void) {
            return m_texID;
        }

        void sendUniform(const GLint uniloc_sampler, const GLint uniloc_has, const unsigned int index) const;
        void sendUniform(const SamplerInterf& uniloc) const;

    private:
        void genTexture(const char* const str4Log);


    };


    class Material {

    public:
        float m_shininess;
        float m_specularStrength;
        glm::vec3 m_diffuseColor;

    private:
        glm::vec2 m_texScale;
        const Texture* m_diffuseMap;

    public:
        Material(void);

        // If paremeter value is 0, old value remains.
        void setTexScale(float x, float y);
        void setDiffuseMap(const Texture* const tex);

        void sendUniform(const UniInterfLightedMesh& unilocLight) const;
        void sendUniform(const UniInterfLightedMesh& unilocLight, const SamplerInterf& samplerInterf) const;

    };

}