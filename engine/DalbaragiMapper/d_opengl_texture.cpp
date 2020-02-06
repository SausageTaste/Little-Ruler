#include "d_opengl_texture.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <d_logger.h>


namespace {

    auto glfunc(void) {
        return QOpenGLContext::currentContext()->extraFunctions();
    }

}


// ITexture
namespace dal::gl {

    ITexture::ITexture(const gl::uint_t id)
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
            glfunc()->glDeleteTextures(1, &this->m_texID);
            this->m_texID = 0;
        }
    }

    void ITexture::reset(const gl::uint_t id) {
        this->invalidate();
        this->m_texID = id;
    }

    bool ITexture::isReady(void) const {
        return this->m_texID != 0;
    }

    // Protected

    void ITexture::genTexture(const char* const str4Log) {
        using namespace std::string_literals;

        this->invalidate();

        glfunc()->glGenTextures(1, &this->m_texID);
        if ( this->get() == 0 ) {
            dalAbort("Failed to init dal::Texture::init_depthMap::{}"s + (str4Log));
        }
    }

}


// Texture
namespace dal::gl {

    void Texture::init_image(ImageData& image) {
        using namespace std::string_literals;

        auto f = glfunc();

        this->genTexture("Texture::init_diffueMap");
        f->glBindTexture(GL_TEXTURE_2D, this->get());

#if DAL_BLOCKY_TEXTURE
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

        switch ( image.pixSize() ) {

        case 1:
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, image.width(), image.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, image.data());
            break;
        case 3:
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
            break;
        case 4:
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
            break;
        default:
            dalError("Not supported pixel size: {}"s + std::to_string(image.pixSize()));
            return;

        }

        f->glGenerateMipmap(GL_TEXTURE_2D);
        f->glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::sendUniform(const SamplerInterf& uniloc) const {
        if ( this->isReady() ) {
            auto f = glfunc();

            uniloc.sendFlagHas(true);
            f->glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(uniloc.getUnitIndex()));
            f->glBindTexture(GL_TEXTURE_2D, this->get());
            f->glUniform1i(uniloc.getSamplerLoc(), uniloc.getUnitIndex());
        }
        else {
            uniloc.sendFlagHas(false);
        }
    }

}


// Cube map
namespace dal::gl {

    void CubeMap::init(const CubeMap::BuildInfo& data) {
        using namespace std::string_literals;

        auto f = glfunc();

        this->genTexture("CubeMap::init");
        f->glBindTexture(GL_TEXTURE_CUBE_MAP, this->get());

#if DAL_BLOCKY_TEXTURE
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

        for ( unsigned int i = 0; i < 6; i++ ) {
            const auto [buf, width, height, pixSize] = data.at(i);

            switch ( pixSize ) {

            case 1:
                f->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf);
                break;
            case 3:
                f->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
                break;
            case 4:
                f->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
                break;
            default:
                dalError("Not supported pixel size: {}"s + std::to_string(pixSize));
                return;

            }
        }

        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void CubeMap::initAttach_colorMap(const unsigned width, const unsigned height) {
        auto f = glfunc();

        this->genTexture("CubeMap::initAttach_colorMap");
        f->glBindTexture(GL_TEXTURE_CUBE_MAP, this->get());

        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

#if DAL_BLOCKY_TEXTURE
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

        for ( unsigned int i = 0; i < 6; i++ ) {
            f->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        f->glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    void CubeMap::sendUniform(const SamplerInterf& uniloc) const {
        if ( this->isReady() ) {
            auto f = glfunc();

            uniloc.sendFlagHas(true);
            f->glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(uniloc.getUnitIndex()));
            f->glBindTexture(GL_TEXTURE_CUBE_MAP, this->get());
            f->glUniform1i(uniloc.getSamplerLoc(), uniloc.getUnitIndex());
        }
        else {
            uniloc.sendFlagHas(false);
        }
    }

}
