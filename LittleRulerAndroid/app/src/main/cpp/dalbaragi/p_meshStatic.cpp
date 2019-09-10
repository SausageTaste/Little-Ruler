#include "p_meshStatic.h"

#include <string>

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace dal {

    int MeshStatic::buildData(const float* const vertices, const float* const texcoords, const float* const normals, const int numVertices) {
        /* Check if data is wrong. */
        {
            if ( this->isReady() ) {
                dalError("MeshStatic's data already built.");
                return -1;
            }
        }

        this->createBuffers();
        this->bindVAO();
        size_t vramUsage = 0;

        // Vertices
        {
            const auto arraySize = numVertices * sizeof(float) * 3;
            this->fillBufferData<0, GL_FLOAT>(vertices, arraySize, 3);
        }

        // Tex coords
        {
            const auto arraySize = numVertices * sizeof(float) * 2;
            this->fillBufferData<1, GL_FLOAT>(texcoords, arraySize, 2);
        }

        // Normals
        {
            const auto arraySize = numVertices * sizeof(float) * 3;
            this->fillBufferData<2, GL_FLOAT>(normals, arraySize, 3);
        }

        /* Finish */
        {
            this->setNumVert(numVertices);
        }

        return 0;
    }

    void MeshAnimated::buildData(const float* const vertices, const float* const texcoords, const float* const normals,
        const int32_t* const boneids, const float* const weights, const size_t numVertices)
    {
        if ( this->isReady() ) {
            dalAbort("MeshStatic's data already built.");
        }

        this->createBuffers();
        this->bindVAO();

        // Vertices
        {
            const auto arraySize = numVertices * sizeof(float) * 3;
            this->fillBufferData<0, GL_FLOAT>(vertices, arraySize, 3);
        }

        // Tex coords
        {
            const auto arraySize = numVertices * sizeof(float) * 2;
            this->fillBufferData<1, GL_FLOAT>(texcoords, arraySize, 2);
        }

        // Normals
        {
            const auto arraySize = numVertices * sizeof(float) * 3;
            this->fillBufferData<2, GL_FLOAT>(normals, arraySize, 3);
        }

        // bone ids
        {
            const auto arraySize = numVertices * sizeof(int32_t) * 3;
            this->fillBufferData<3, GL_INT>(boneids, arraySize, 3);
        }

        // weights
        {
            const auto arraySize = numVertices * sizeof(float) * 3;
            this->fillBufferData<4, GL_FLOAT>(weights, arraySize, 3);
        }

        /* Finish */
        {
            this->unbindVAO();
            this->setNumVert(numVertices);
        }
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Nvidia cannot draw shadow with those lines, but Galaxy S7 needs it.
#ifdef __ANDROID__
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
#endif
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
