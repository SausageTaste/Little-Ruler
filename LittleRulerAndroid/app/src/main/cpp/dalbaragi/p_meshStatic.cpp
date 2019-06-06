#include "p_meshStatic.h"

#include <string>

#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {

    void MeshStatic::draw(void) const {
#ifdef _DEBUG
        if ( !this->isReady() )
            LoggerGod::getinst().putError("MeshStatic::renderDepthmap called without being built.", __LINE__, __func__, __FILE__);
#endif

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
                LoggerGod::getinst().putError("MeshStatic's data already built.", __LINE__, __func__, __FILE__);
                return -1;
            }

            const auto numOfVertex = vertSize / 3;

            if ( numOfVertex != (texcorSize / 2) ) {
                LoggerGod::getinst().putError(std::string("\'texCoords\' have different number of vertices: " + std::to_string(vertSize) + ", " + std::to_string(texcorSize)), __LINE__, __func__, __FILE__);
                return -1;
            }

            if ( numOfVertex != (norSize / 3) ) {
                LoggerGod::getinst().putError("\'normals\' have different number of vertices: "s + std::to_string(vertSize) + ", "s + std::to_string(norSize), __LINE__, __func__, __FILE__);
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

            //LoggerGod::getinst().putInfo("created MeshStatic with "s + to_string(this->mVertexSize) + " vertices."s);
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

        LoggerGod::getinst().putInfo("destroyed MeshStatic with "s + std::to_string(this->mVertexSize) + " vertices."s, __LINE__, __func__, __FILE__);
    }

    bool MeshStatic::isReady(void) const {
        return mVertexSize != 0;
    }

    void MeshStatic::setName(const char* const name) {
        mName = name;
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
            glBufferData(GL_ARRAY_BUFFER, size, normals, GL_STATIC_DRAW);

            glVertexAttribIPointer(3, 3, GL_INT, 0, nullptr);
            glEnableVertexAttribArray(3);
        }

        // weights
        {
            const auto size = numVertices * sizeof(float) * 3;
            vramUsage += size;

            glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[4]);
            glBufferData(GL_ARRAY_BUFFER, size, normals, GL_STATIC_DRAW);

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

    Texture::Texture(Texture&& other) noexcept {
        this->m_texID = other.m_texID;
        other.m_texID = 0;
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        this->m_texID = other.m_texID;
        other.m_texID = 0;

        return *this;
    }

    Texture::~Texture(void) {
        if ( this->isReady() ) this->deleteTex();
    }


    void Texture::init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

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
        glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

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
        glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void Texture::initAttach_colorMap(const unsigned int width, const unsigned int height) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
        this->genTexture("init_texAttachment");

        glBindTexture(GL_TEXTURE_2D, this->m_texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->m_texID, 0);
        //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    }

    void Texture::deleteTex(void) {
        glDeleteTextures(1, &this->m_texID);
        this->m_texID = 0;
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

    bool Texture::isReady(void) const {
        return this->m_texID != 0;
    }

    // Getters

    GLuint Texture::get(void) {
        return m_texID;
    }

    // Private

    void Texture::genTexture(const char* const str4Log) {
        if ( this->isReady() ) {
            this->deleteTex();
        }

        glGenTextures(1, &m_texID);
        if ( m_texID == 0 ) {
            dalAbort("Failed to init dal::Texture::init_depthMap::"s + str4Log);
        }
    }

}


// Material
namespace dal {

    void Material::setTexScale(float x, float y) {
        this->m_texScale.x = x;
        this->m_texScale.y = y;
    }

    void Material::setDiffuseMap(Texture* const tex) {
        this->m_diffuseMap = tex;
    }

    void Material::sendUniform(const UnilocGeneral& uniloc) const {
        glUniform1f(uniloc.uShininess, this->m_shininess);
        glUniform1f(uniloc.uSpecularStrength, this->m_specularStrength);

        glUniform1f(uniloc.uTexScaleX, this->m_texScale.x);
        glUniform1f(uniloc.uTexScaleY, this->m_texScale.y);

        glUniform3f(uniloc.uDiffuseColor, this->m_diffuseColor.x, this->m_diffuseColor.y, this->m_diffuseColor.z);

        if ( nullptr == this->m_diffuseMap ) {
            glUniform1i(uniloc.uHasDiffuseMap, 0);
        }
        else {
            this->m_diffuseMap->sendUniform(uniloc.uDiffuseMap, uniloc.uHasDiffuseMap, 0);
        }
    }

    void Material::sendUniform(const UnilocWaterry& uniloc) const {
        glUniform1f(uniloc.uShininess, this->m_shininess);
        glUniform1f(uniloc.uSpecularStrength, this->m_specularStrength);

        glUniform1f(uniloc.uTexScaleX, this->m_texScale.x);
        glUniform1f(uniloc.uTexScaleY, this->m_texScale.y);

        glUniform3f(uniloc.uDiffuseColor, this->m_diffuseColor.x, this->m_diffuseColor.y, this->m_diffuseColor.z);

        if ( nullptr == this->m_diffuseMap ) {
            glUniform1i(uniloc.uHasDiffuseMap, 0);
        }
        else {
            this->m_diffuseMap->sendUniform(uniloc.uDiffuseMap, uniloc.uHasDiffuseMap, 0);
        }
    }

}