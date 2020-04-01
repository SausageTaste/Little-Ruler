#include "p_meshStatic.h"

#include <string>

#include <fmt/format.h>

#include <d_logger.h>


#define DAL_BLOCKY_TEXTURE false


using namespace fmt::literals;


namespace dal {

    int MeshStatic::buildData(const float* const vertices, const float* const texcoords, const float* const normals, const size_t numVertices) {
        /* Check if data is wrong. */
        {
            if ( this->isReady() ) {
                dalError("MeshStatic's data already built.");
                return -1;
            }
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


// ITexture
namespace dal {

    ITexture::ITexture(const GLuint id)
        : m_texID(id)
    {

    }

    ITexture::ITexture(ITexture&& other) noexcept
        : m_texID(other.m_texID)
    {
        other.m_texID = 0;
    }

    ITexture& ITexture::operator=(ITexture&& other) noexcept {
        this->invalidate();

        this->m_texID = other.m_texID;
        other.m_texID = 0;

        return *this;
    }

    ITexture::~ITexture(void) {
        this->invalidate();
    }

    void ITexture::invalidate(void) {
        if ( this->m_texID != 0 ) {
            glDeleteTextures(1, &this->m_texID);
            this->m_texID = 0;
        }
    }

    void ITexture::reset(const GLuint id) {
        this->invalidate();
        this->m_texID = id;
    }

    bool ITexture::isReady(void) const {
        return this->m_texID != 0;
    }

    // Protected

    void ITexture::genTexture(const char* const str4Log) {
        this->invalidate();

        glGenTextures(1, &this->m_texID);
        if ( this->get() == 0 ) {
            dalAbort("Failed to init dal::Texture::init_depthMap::{}"_format(str4Log));
        }
    }

}


// Texture
namespace dal {

    void Texture::init_diffuseMap(ImageData& image) {
        this->genTexture("Texture::init_diffueMap");
        glBindTexture(GL_TEXTURE_2D, this->get());

#if DAL_BLOCKY_TEXTURE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

        switch ( image.pixSize() ) {

        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, image.width(), image.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image.data());
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
            break;
        default:
            dalError("Not supported pixel size: {}"_format(image.pixSize()));
            return;

        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::init_depthMap(const unsigned int width, const unsigned int height) {
        this->genTexture("Texture::init_depthMap");

        glBindTexture(GL_TEXTURE_2D, this->get());
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
        this->genTexture("Texture::init_maskMap");

        glBindTexture(GL_TEXTURE_2D, this->get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    void Texture::initAttach_colorMap(const unsigned int width, const unsigned int height) {
        this->genTexture("Texture::initAttach_colorMap");

        glBindTexture(GL_TEXTURE_2D, this->get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->get(), 0);
        //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    }


    void Texture::resize_colorMap(const unsigned int width, const unsigned int height) {
        glBindTexture(GL_TEXTURE_2D, this->get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    void Texture::sendUniform(const GLint uniloc_sampler, const GLint uniloc_has, const unsigned int index) const {
        if ( this->isReady() ) {
            glUniform1i(uniloc_has, 1);
            glActiveTexture(GL_TEXTURE0 + index);
            glBindTexture(GL_TEXTURE_2D, this->get());
            glUniform1i(uniloc_sampler, index);
        }
        else {
            glUniform1i(uniloc_has, 0);
        }
    }

    void Texture::sendUniform(const SamplerInterf& uniloc) const {
        if ( this->isReady() ) {
            uniloc.setFlagHas(true);
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(uniloc.getUnitIndex()));
            glBindTexture(GL_TEXTURE_2D, this->get());
            glUniform1i(uniloc.getSamplerLoc(), uniloc.getUnitIndex());
        }
        else {
            uniloc.setFlagHas(false);
        }
    }

}


// Cube map
namespace dal {

    void CubeMap::init(const CubeMap::CubeMapData& data) {
        this->genTexture("CubeMap::init");
        glBindTexture(GL_TEXTURE_CUBE_MAP, this->get());

#if DAL_BLOCKY_TEXTURE
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

        for ( unsigned int i = 0; i < 6; i++ ) {
            const auto [buf, width, height, pixSize] = data.at(i);

            switch ( pixSize ) {

            case 1:
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);
                break;
            case 3:
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
                break;
            case 4:
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
                break;
            default:
                dalError("Not supported pixel size: {}"_format(pixSize));
                return;

            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void CubeMap::initAttach_colorMap(const unsigned width, const unsigned height) {
        this->genTexture("CubeMap::initAttach_colorMap");
        glBindTexture(GL_TEXTURE_CUBE_MAP, this->get());

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

#if DAL_BLOCKY_TEXTURE
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

        for ( unsigned int i = 0; i < 6; i++ ) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    void CubeMap::sendUniform(const SamplerInterf& uniloc) const {
        if ( this->isReady() ) {
            uniloc.setFlagHas(true);
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(uniloc.getUnitIndex()));
            glBindTexture(GL_TEXTURE_CUBE_MAP, this->get());
            glUniform1i(uniloc.getSamplerLoc(), uniloc.getUnitIndex());
        }
        else {
            uniloc.setFlagHas(false);
        }
    }

}

// Material
namespace dal {

    Material::Material(void)
        : m_roughness(0.5f)
        , m_metallic(0.f)
        , m_texScale(1.0f, 1.0f)
    {

    }

    void Material::sendUniform(const UniInterfLightedMesh& uniloc) const {
        uniloc.texScale(this->m_texScale);
        uniloc.roughness(this->m_roughness);
        uniloc.metallic(this->m_metallic);
    }

    void Material::sendUniform(const UniInterfLightedMesh& unilocLight, const SamplerInterf& samplerInterf) const {
        this->sendUniform(unilocLight);

        if ( this->m_diffuseMap ) {
            this->m_diffuseMap->sendUniform(samplerInterf);
        }
    }

    void Material::sendUniform(const UniInterf_Lighting& uniloc) const {
        uniloc.roughness(this->m_roughness);
        uniloc.metallic(this->m_metallic);
    }

    void Material::sendUniform(const UniInterf_Lightmap& uniloc) const {
        if ( this->m_diffuseMap )
            this->m_diffuseMap->sendUniform(uniloc.diffuseMap());

        if ( this->m_roughnessMap )
            this->m_roughnessMap->sendUniform(uniloc.roughnessMap());
        else
            uniloc.roughnessMap().setFlagHas(false);

        if ( this->m_metallicMap )
            this->m_metallicMap->sendUniform(uniloc.metallicMap());
        else
            uniloc.metallicMap().setFlagHas(false);
    }

}
