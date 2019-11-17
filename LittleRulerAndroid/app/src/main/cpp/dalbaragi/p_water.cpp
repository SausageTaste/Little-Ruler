#include "p_water.h"

#include <array>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "u_fileutils.h"


using namespace fmt::literals;


namespace {

    dal::Texture* loadTex(const char* const respath) {
        dal::binfo::ImageFileData image;
        if ( !dal::loadFileImage(respath, image) ) {
            dalAbort("Failed to load a map for water: {}"_format(respath));
        }
        assert(4 == image.m_pixSize);

        auto tex = new dal::Texture();
        tex->init_diffuseMap(image);

        return tex;
    }

    dal::Texture* getDUDVMap(void) {
        static dal::Texture* tex = loadTex("asset::waterDUDV.png");
        return tex;
    }

    dal::Texture* getWaterNormalMap(void) {
        static dal::Texture* tex = loadTex("asset::matchingNormalMap.png");
        return tex;
    }

}


namespace {

    GLuint genFramebuffer(void) {
        GLuint fbuffer;

        glGenFramebuffers(1, &fbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, fbuffer);
        //glDrawBuffer(GL_COLOR_ATTACHMENT0);

        return fbuffer;
    }

    /*
    GLuint genTextureAttachment(const unsigned int width, const unsigned int height) {
        GLuint texture;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

        return texture;
    }
    */

    GLuint genDepthTextureAttachment(const unsigned int width, const unsigned int height) {
        GLuint texture;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Eunchae's computer cannot draw shadow with those lines.
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
        glClear(GL_DEPTH_BUFFER_BIT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

        return texture;
    }

    GLuint genDepthBufferAttachment(const unsigned int width, const unsigned int height) {
        GLuint depthBuffer;

        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        return depthBuffer;
    }

    void bindFrameBuffer(const GLuint frameBuffer, const unsigned int width, const unsigned int height) {
        //glBindTexture(GL_TEXTURE_2D, 0);  //To make sure the texture isn't bound
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glViewport(0, 0, width, height);
    }

    bool checkFramebuffer(void) {
        const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        switch ( status ) {

        case GL_FRAMEBUFFER_COMPLETE:
            return true;

        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            dalError("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            dalError("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            dalError("GL_FRAMEBUFFER_UNSUPPORTED");
            return false;

        default:
            dalError("Unknown gl framebuffer error");
            return false;

        }

    }

}


namespace dal {

    void deleteFramebuffer(const GLuint fbo) {
        glDeleteFramebuffers(1, &fbo);
    }

    void deleteRenderbuffer(const GLuint rbo) {
        glDeleteRenderbuffers(1, &rbo);
    }

}


namespace dal {

    WaterFramebuffer::WaterFramebuffer(const unsigned int winWidth, const unsigned int winHeight)
        : m_winWidth(static_cast<float>(winWidth))
        , m_winHeight(static_cast<float>(winHeight))
        , m_reflecScale(0.5f)
        , m_refracScale(0.5f)
    {
        const auto REFLECTION_WIDTH = static_cast<GLsizei>(this->m_winWidth  * this->m_reflecScale);
        const auto REFLECTION_HEIGHT = static_cast<GLsizei>(this->m_winHeight * this->m_reflecScale);
        const auto REFRACTION_WIDTH = static_cast<GLsizei>(this->m_winWidth  * this->m_refracScale);
        const auto REFRACTION_HEIGHT = static_cast<GLsizei>(this->m_winHeight * this->m_refracScale);

        this->recreateFbuffer(REFLECTION_WIDTH, REFLECTION_HEIGHT, REFRACTION_WIDTH, REFRACTION_HEIGHT);
    }


    void WaterFramebuffer::sendUniform(const UnilocWaterry& uniloc) const {
        this->m_reflectionTexture.sendUniform(uniloc.getReflectionTex());
        this->m_refractionTexture.sendUniform(uniloc.getRefractionTex());
        this->m_refractionDepthTexture.sendUniform(uniloc.getDepthMap());
    }

    void WaterFramebuffer::bindReflectionFrameBuffer(void) const {  //call before rendering to this FBO
        bindFrameBuffer(
            this->m_reflectionFrameBuffer.get(),
            static_cast<int>(this->m_winWidth  * this->m_reflecScale),
            static_cast<int>(this->m_winHeight  * this->m_reflecScale)
        );
    }

    void WaterFramebuffer::bindRefractionFrameBuffer(void) const {  //call before rendering to this FBO
        bindFrameBuffer(this->m_refractionFrameBuffer.get(),
            static_cast<int>(this->m_winWidth * this->m_refracScale),
            static_cast<int>(this->m_winHeight * this->m_refracScale)
        );
    }


    void WaterFramebuffer::onWinResize(const unsigned int winWidth, const unsigned int winHeight) {
        this->m_winWidth = static_cast<float>(winWidth);
        this->m_winHeight = static_cast<float>(winHeight);

        const GLsizei bansa_width = static_cast<GLsizei>(this->m_winWidth  * this->m_reflecScale);
        const GLsizei bansa_height = static_cast<GLsizei>(this->m_winHeight * this->m_reflecScale);
        const GLsizei gooljul_width = static_cast<GLsizei>(this->m_winWidth  * this->m_refracScale);
        const GLsizei gooljul_height = static_cast<GLsizei>(this->m_winHeight * this->m_refracScale);

        this->recreateFbuffer(bansa_width, bansa_height, gooljul_width, gooljul_height);
    }

    // Private

    void WaterFramebuffer::resizeOnlyTextures(const unsigned int bansa_width, const unsigned int bansa_height, const unsigned int gooljul_width, const unsigned int gooljul_height) {
        {
            glBindTexture(GL_TEXTURE_2D, this->m_reflectionTexture.get());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bansa_width, bansa_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }

        {
            glBindTexture(GL_TEXTURE_2D, this->m_refractionTexture.get());
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gooljul_width, gooljul_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        {
            glBindRenderbuffer(GL_RENDERBUFFER, this->m_reflectionDepthBuffer.get());
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bansa_width, bansa_height);
        }

        {
            glBindRenderbuffer(GL_RENDERBUFFER, this->m_refractionDepthTexture.get());
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gooljul_width, gooljul_height);
        }

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void WaterFramebuffer::recreateFbuffer(const unsigned int reflecWidth, const unsigned int reflecHeight, const unsigned int refracWidth, const unsigned int refracHeight) {
        {
            this->m_reflectionFrameBuffer.reset(genFramebuffer());
            this->m_reflectionTexture.initAttach_colorMap(reflecWidth, reflecHeight);
            this->m_reflectionDepthBuffer.reset(genDepthBufferAttachment(reflecWidth, reflecHeight));
        }

        if ( !checkFramebuffer() ) dalError("Framebuffer creation failed for reflection.");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        {
            this->m_refractionFrameBuffer.reset(genFramebuffer());
            this->m_refractionTexture.initAttach_colorMap(refracWidth, refracHeight);
            //this->m_refractionDepthTexture.reset(genDepthBufferAttachment(refracWidth, refracHeight));
            this->m_refractionDepthTexture.reset(genDepthTextureAttachment(refracWidth, refracHeight));
        }

        if ( !checkFramebuffer() ) dalError("Framebuffer creation failed for reflection.");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}


namespace dal {

    WaterRenderer::WaterRenderer(const dlb::WaterPlane& info, const unsigned int winWidth, const unsigned int winHeight)
        : m_fbuffer(winWidth, winHeight)
        , m_depthColor(info.m_deepColor)
        , m_height(info.m_centerPos.y)
        , m_moveSpeed(info.m_flowSpeed)
        , m_waveStreng(info.m_waveStreng)
        , m_darkestDepthPoint(info.m_darkestDepth)
        , m_reflectivity(info.m_reflectivity)
        , m_moveFactor(0.0f)
        , m_dudvMap(getDUDVMap())
        , m_normalMap(getWaterNormalMap())
    {
        constexpr float TEX_SCALE_FACTOR = 0.05f;

        glm::vec2 size{ info.m_width, info.m_height };
        this->initMesh(info.m_centerPos, size);

        this->m_material.m_roughness = 0.2f;
        this->m_material.m_metallic = 0.f;
        this->m_material.m_texScale = glm::vec2{ info.m_width * TEX_SCALE_FACTOR, info.m_height * TEX_SCALE_FACTOR };
    }


    void WaterRenderer::render(const UnilocWaterry& uniloc) {
        const auto deltaTime = this->m_localTimer.checkGetElapsed();
        this->m_moveFactor += this->m_moveSpeed * deltaTime;
        this->m_moveFactor = fmod(this->m_moveFactor, 1.0f);
        uniloc.dudvFactor(this->m_moveFactor);

        uniloc.waveStrength(this->m_waveStreng);
        uniloc.deepColor(this->m_depthColor);
        uniloc.darkestDepthPoint(this->m_darkestDepthPoint);
        uniloc.reflectivity(this->m_reflectivity);

        this->m_material.sendUniform(uniloc.m_lightedMesh);

        this->m_fbuffer.sendUniform(uniloc);
        this->m_dudvMap->sendUniform(uniloc.getDUDVMap());
        this->m_normalMap->sendUniform(uniloc.getNormalMap());

        uniloc.m_lightedMesh.modelMat(glm::mat4{ 1.0f });
        this->m_mesh.draw();
    }


    void WaterRenderer::startRenderOnReflec(const UnilocGeneral& uniloc, const ICamera& cam) const {
        uniloc.m_planeClip.flagDoClip(true);
        uniloc.m_planeClip.clipPlane(0.f, 1.f, 0.f, -this->m_height + 0.1f);

        auto [reflectedPos, reflectedMat] = cam.makeReflected(this->m_height);

        uniloc.m_lightedMesh.viewMat(reflectedMat);
        uniloc.m_lightedMesh.viewPos(reflectedPos);

        this->m_fbuffer.bindReflectionFrameBuffer();
    }

    void WaterRenderer::startRenderOnReflec(const UniInterfGeometry& uniloc, const ICamera& cam) const {
        auto [reflectedPos, reflectedMat] = cam.makeReflected(this->m_height);
        uniloc.viewMat(glm::mat4(glm::mat3(reflectedMat)));

        this->m_fbuffer.bindReflectionFrameBuffer();
    }

    void WaterRenderer::startRenderOnRefrac(const UnilocGeneral& uniloc, const ICamera& cam) const {
        uniloc.m_planeClip.flagDoClip(true);
        uniloc.m_planeClip.clipPlane(0.f, -1.f, 0.f, this->m_height);

        uniloc.m_lightedMesh.viewMat(cam.getViewMat());
        uniloc.m_lightedMesh.viewPos(cam.m_pos);

        this->m_fbuffer.bindRefractionFrameBuffer();
    }

    // Private

    void WaterRenderer::initMesh(const glm::vec3& pos, const glm::vec2& size) {
        const auto halfSize = size * 0.5f;

        std::array<float, 18> vertices{
            pos.x - halfSize.x, pos.y, pos.z - halfSize.y,
            pos.x - halfSize.x, pos.y, pos.z + halfSize.y,
            pos.x + halfSize.x, pos.y, pos.z + halfSize.y,
            pos.x - halfSize.x, pos.y, pos.z - halfSize.y,
            pos.x + halfSize.x, pos.y, pos.z + halfSize.y,
            pos.x + halfSize.x, pos.y, pos.z - halfSize.y
        };
        std::array<float, 12> texcoords{
            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1
        };
        std::array<float, 18> normals{
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0
        };

        this->m_mesh.buildData(
            vertices.data(),
            texcoords.data(),
            normals.data(),
            vertices.size() / 3
        );
    }

}
