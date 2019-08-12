#include "p_meshStatic.h"

#include <string>

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace dal {

    void MeshStatic::draw(void) const {
        dalAssert(this->isReady());

        this->bindVAO();
        glDrawArrays(GL_TRIANGLES, 0, this->mVertexSize);
        this->unbindVAO();
    }

    int MeshStatic::buildData(
        const float* const vertices, const int vertSize,
        const float* const texcors, const int texcorSize,
        const float* const normals, const int norSize
    ) {
        /* Check if data is wrong. */
        {
            if ( this->isReady() ) {
                dalError("MeshStatic's data already built.");
                return -1;
            }

            const auto numOfVertex = vertSize / 3;

            if ( numOfVertex != (texcorSize / 2) ) {
                dalError("\'texCoords\' have different number of vertices -> vertSize : {}, texcorSize : {}"_format(vertSize, texcorSize));
                return -1;
            }

            if ( numOfVertex != (norSize / 3) ) {
                dalError("\'normals\' have different number of vertices -> vertSize : {}, norSize : {}"_format(vertSize, norSize));
                return -1;
            }
        }

        this->createBuffers();
        this->bindVAO();
        size_t  vramUsage = 0;

        /* Vertices */
        {
            auto size = vertSize * sizeof(float);

            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->mVertexArrayBuffer);
            glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        }

        /* TexCoords */
        {
            auto size = texcorSize * sizeof(float);
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->mTexCoordArrayBuffer);
            glBufferData(GL_ARRAY_BUFFER, size, texcors, GL_STATIC_DRAW);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        }

        /* Normals */
        {
            auto size = norSize * sizeof(float);
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->mNormalArrayBuffe);
            glBufferData(GL_ARRAY_BUFFER, size, normals, GL_STATIC_DRAW);

            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(2);
        }

        /* Finish */
        {
            this->unbindVAO();
            this->mVertexSize = vertSize / 3;
        }

        return 0;
    }

    void MeshStatic::destroyData(void) {
        GLuint bufferIds[3] = {
            this->mVertexArrayBuffer,
            this->mTexCoordArrayBuffer,
            this->mNormalArrayBuffe,
        };
        glDeleteBuffers(3, bufferIds);

        this->mVertexArrayBuffer = 0;
        this->mTexCoordArrayBuffer = 0;
        this->mNormalArrayBuffe = 0;

        glDeleteVertexArrays(1, &this->mVao);
        this->mVao = 0;

        this->mVertexSize = 0;

        dalInfo("destroyed MeshStatic that has {} vertices."_format(this->mVertexSize));
    }

    bool MeshStatic::isReady(void) const {
        return mVertexSize != 0;
    }

    //// Private ////

    void MeshStatic::createBuffers(void) {
        glGenVertexArrays(1, &this->mVao);
        if ( this->mVao <= 0 ) {
            dalAbort("Failed to generate vertex array.")
        }

        GLuint bufferIds[3];
        glGenBuffers(3, bufferIds);

        for ( int i = 0; i < 3; i++ ) {
            if ( bufferIds[i] == 0 ) {
                dalAbort("Failed to generate beffer.");
            }
        }

        this->mVertexArrayBuffer = bufferIds[0];
        this->mTexCoordArrayBuffer = bufferIds[1];
        this->mNormalArrayBuffe = bufferIds[2];
    }

    void MeshStatic::bindVAO(void) const {
        glBindVertexArray(this->mVao);
    }

    void MeshStatic::unbindVAO(void) {
        glBindVertexArray(0);
    }

}


namespace dal {

    void MeshAnimated::draw(void) const {
#ifdef _DEBUG
        if ( !this->isReady() )
            dalError("MeshStatic::renderDepthmap called without being built.");
#endif

        this->bindVAO();
        glDrawArrays(GL_TRIANGLES, 0, this->m_numVertices);
        this->unbindVAO();
    }

    void MeshAnimated::buildData(const float* const vertices, const float* const texcors, const float* const normals,
        const int32_t* const boneids, const float* const weights, const size_t numVertices)
    {
        if ( this->isReady() ) {
            dalAbort("MeshStatic's data already built.");
        }

        this->createBuffers();
        this->bindVAO();
        size_t vramUsage = 0;

        // Vertices
        {
            const auto size = numVertices * sizeof(float) * 3;
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[0]);
            glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        }

        // TexCoords
        {
            const auto size = numVertices * sizeof(float) * 2;
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[1]);
            glBufferData(GL_ARRAY_BUFFER, size, texcors, GL_STATIC_DRAW);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        }

        // Normals
        {
            const auto size = numVertices * sizeof(float) * 3;
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[2]);
            glBufferData(GL_ARRAY_BUFFER, size, normals, GL_STATIC_DRAW);

            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);
        }

        // bone ids
        {
            const auto size = numVertices * sizeof(float) * 3;
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[3]);
            glBufferData(GL_ARRAY_BUFFER, size, boneids, GL_STATIC_DRAW);

            glVertexAttribIPointer(3, 3, GL_INT, 0, nullptr);
            glEnableVertexAttribArray(3);
        }

        // weights
        {
            const auto size = numVertices * sizeof(float) * 3;
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[4]);
            glBufferData(GL_ARRAY_BUFFER, size, weights, GL_STATIC_DRAW);

            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(4);
        }

        /* Finish */
        {
            this->unbindVAO();
            this->m_numVertices = numVertices;
        }
    }

    void MeshAnimated::destroyData(void) {
        glDeleteBuffers(5, this->m_buffers);

        for ( int i = 0; i < 5; i++ ) {
            this->m_buffers[i] = 0;
        }

        glDeleteVertexArrays(1, &this->m_vao);
        this->m_vao = 0;

        this->m_numVertices = 0;
    }

    bool MeshAnimated::isReady(void) const {
        return this->m_numVertices != 0;
    }

    //// Private ////

    void MeshAnimated::createBuffers(void) {
        glGenVertexArrays(1, &this->m_vao);
        if ( this->m_vao <= 0 ) {
            dalAbort("Failed to generate vertex array.")
        }

        glGenBuffers(5, this->m_buffers);

        for ( int i = 0; i < 5; i++ ) {
            if ( 0 == this->m_buffers[i] ) {
                dalAbort("Failed to generate beffer.");
            }
        }
    }

    void MeshAnimated::bindVAO(void) const {
        glBindVertexArray(this->m_vao);
    }

    void MeshAnimated::unbindVAO(void) {
        glBindVertexArray(0);
    }

}


// Texture
namespace dal {

    Texture::Texture(const GLuint id)
        : m_texID(id)
    {

    }

    Texture::Texture(Texture&& other) noexcept
        : m_texID(other.m_texID)
    {
        other.m_texID = 0;
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        this->invalidate();

        this->m_texID = other.m_texID;
        other.m_texID = 0;

        return *this;
    }

    Texture::~Texture(void) {
        this->invalidate();
    }


    void Texture::init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
        this->genTexture("init_diffueMap");

        glBindTexture(GL_TEXTURE_2D, m_texID);

#if BLOCKY_TEXTURE == 0
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::init_diffueMap3(const uint8_t* const image, const unsigned int width, const unsigned int height) {
        this->genTexture("init_diffueMap");

        glBindTexture(GL_TEXTURE_2D, m_texID);

#if BLOCKY_TEXTURE == 0
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::init_depthMap(const unsigned int width, const unsigned int height) {
        this->genTexture("init_depthMap");

        glBindTexture(GL_TEXTURE_2D, m_texID);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
        } glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
        this->genTexture("init_maskMap");

        glBindTexture(GL_TEXTURE_2D, m_texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void Texture::initAttach_colorMap(const unsigned int width, const unsigned int height) {
        this->genTexture("init_texAttachment");

        glBindTexture(GL_TEXTURE_2D, this->m_texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->m_texID, 0);
        //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    }


    void Texture::invalidate(void) {
        if ( this->m_texID != 0 ) {
            glDeleteTextures(1, &this->m_texID);
            this->m_texID = 0;
        }
    }

    void Texture::reset(const GLuint id) {
        this->invalidate();
        this->m_texID = id;
    }

    bool Texture::isReady(void) const {
        return this->m_texID != 0;
    }

    void Texture::sendUniform(const GLint uniloc_sampler, const GLint uniloc_has, const unsigned int index) const {
        if ( this->isReady() ) {
            glUniform1i(uniloc_has, 1);
            glActiveTexture(GL_TEXTURE0 + index);
            glBindTexture(GL_TEXTURE_2D, this->m_texID);
            glUniform1i(uniloc_sampler, index);
        }
        else {
            glUniform1i(uniloc_has, 0);
        }
    }

    void Texture::sendUniform(const SamplerInterf& uniloc) const {
        if ( this->isReady() ) {
            uniloc.setFlagHas(true);
            glActiveTexture(GL_TEXTURE0 + uniloc.getUnitIndex());
            glBindTexture(GL_TEXTURE_2D, this->m_texID);
            glUniform1i(uniloc.getSamplerLoc(), uniloc.getUnitIndex());
        }
        else {
            uniloc.setFlagHas(false);
        }
    }

    // Private

    void Texture::genTexture(const char* const str4Log) {
        this->invalidate();

        glGenTextures(1, &this->m_texID);
        if ( this->m_texID == 0 ) {
            dalAbort("Failed to init dal::Texture::init_depthMap::{}"_format(str4Log));
        }
    }

}


// Material
namespace dal {

    Material::Material(void)
        : m_shininess(32.0f)
        , m_specularStrength(1.0f)
        , m_diffuseColor(1.0f, 1.0f, 1.0f)
        , m_texScale(1.0f, 1.0f)
        , m_diffuseMap(nullptr)
    {

    }

    void Material::setTexScale(float x, float y) {
        this->m_texScale.x = x;
        this->m_texScale.y = y;
    }

    void Material::setDiffuseMap(const Texture* const tex) {
        this->m_diffuseMap = tex;
    }

    void Material::sendUniform(const UniInterfLightedMesh& unilocLight) const {
        unilocLight.shininess(this->m_shininess);
        unilocLight.specularStrength(this->m_specularStrength);
        unilocLight.texScale(this->m_texScale);
    }

    void Material::sendUniform(const UniInterfLightedMesh& unilocLight, const SamplerInterf& samplerInterf) const {
        unilocLight.shininess(this->m_shininess);
        unilocLight.specularStrength(this->m_specularStrength);
        unilocLight.texScale(this->m_texScale);

        if ( nullptr != this->m_diffuseMap ) {
            this->m_diffuseMap->sendUniform(samplerInterf);
        }
    }

}