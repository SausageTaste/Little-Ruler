#include "o_widget_base.h"

#include <utility>  // for std::swap

#include "p_dalopengl.h"
#include "m_collider.h"
#include "s_configs.h"
#include "s_logger_god.h"


namespace {

    auto& g_logger = dal::LoggerGod::getinst();


    class RealQuadRenderer {

    private:
        GLuint mBufferObj;
        GLuint mVao;

        RealQuadRenderer(void) {
            glGenVertexArrays(1, &mVao);
            if ( mVao <= 0 ) dalAbort("Failed to generate vertex array.");
            glGenBuffers(1, &mBufferObj);
            if ( mBufferObj <= 0 ) dalAbort("Failed to generate gl buffers.");

            glBindVertexArray(mVao);

            /* Vertices */
            {
                GLfloat vertices[12] = {
                        0, 1,
                        0, 0,
                        1, 0,
                        0, 1,
                        1, 0,
                        1, 1
                };
                auto size = 12 * sizeof(float);

                glBindBuffer(GL_ARRAY_BUFFER, this->mBufferObj);
                glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            }

            glBindVertexArray(0);
        }

    public:
        static RealQuadRenderer& getinst(void) {
            static RealQuadRenderer inst;
            return inst;
        }

        void renderOverlay(void) {
            glBindVertexArray(this->mVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    };


    glm::vec2 screen2device1(const glm::vec2& p, const float winWidth, const float winHeight) {
        return {
             2.0f * p.x / winWidth - 1.0f,
            -2.0f * p.y / winHeight + 1.0f
        };
    }

    glm::vec2 device2screen1(const glm::vec2& p, const float winWidth, const float winHeight) {
        return {
            (p.x + 1.0f) * winWidth / 2.0f,
            (1.0f - p.y) * winHeight / 2.0f
        };
    }

}

// QuadInfo
namespace dal {

    QuadInfo QuadInfo::screen2device(void) const {
        const auto winWidth = static_cast<float>(ConfigsGod::getinst().getWinWidth());
        const auto winHeight = static_cast<float>(ConfigsGod::getinst().getWinHeight());

        QuadInfo newinfo;

        newinfo.p1 = ::screen2device1(this->p1, winWidth, winHeight);
        newinfo.p2 = ::screen2device1(this->p2, winWidth, winHeight);

        /* Validate PointDev order */
        {
            if ( newinfo.p1.x > newinfo.p2.x ) {
                std::swap(newinfo.p1.x, newinfo.p2.x);
            }
            if ( newinfo.p1.y > newinfo.p2.y ) {
                std::swap(newinfo.p1.y, newinfo.p2.y);
            }
        }

        return newinfo;
    }

}


namespace dal {

    ScreenQuad::ScreenQuad(void)
        : m_parent(nullptr)
        , m_xPos(0.0f)
        , m_yPos(0.0f)
        , m_width(5.0f)
        , m_height(5.0f)
        , m_alignMode(AlignMode::upper_left)
    {
        this->updateDeviceSpace();
    }

    bool ScreenQuad::isInside(const float x, const float y) {
        const auto quad = this->makeScreenSpace();

        AABB_2D box;
        box.setPoints(quad.p1, quad.p2);
        return box.isInside({ x, y });
    }

    void ScreenQuad::setPosX(const float v) {
        this->m_xPos = v;

        this->updateDeviceSpace();
    }

    void ScreenQuad::setPosY(const float v) {
        this->m_yPos = v;

        this->updateDeviceSpace();
    }

    void ScreenQuad::setWidth(const float v) {
        this->m_width = v;

        this->updateDeviceSpace();
    }

    void ScreenQuad::setHeight(const float v) {
        this->m_height = v;

        this->updateDeviceSpace();
    }

    void ScreenQuad::setAlignMode(const AlignMode mode) {
        this->m_alignMode = mode;

        this->updateDeviceSpace();
    }

    QuadInfo ScreenQuad::makeScreenSpace(void) const {
        QuadInfo info;

        QuadInfo parInfo;
        if ( nullptr != this->m_parent ) {
            parInfo = this->m_parent->makeScreenSpace();
        }
        else {
            const auto winWidth = ConfigsGod::getinst().getWinWidth();
            const auto winHeight = ConfigsGod::getinst().getWinHeight();
            parInfo.p1 = { 0.0, 0.0 };
            parInfo.p2 = { winWidth, winHeight };
        }

        switch ( this->m_alignMode ) {

        case AlignMode::upper_left:
            info.p1 = {
                parInfo.p1.x + this->m_xPos,
                parInfo.p1.y + this->m_yPos
            };
            info.p2 = {
                parInfo.p1.x + this->m_xPos + this->m_width,
                parInfo.p1.y + this->m_yPos + this->m_height
            };
            break;
        case AlignMode::upper_right:
            info.p1 = {
                parInfo.p2.x - this->m_xPos - this->m_width,
                parInfo.p1.y + this->m_yPos
            };
            info.p2 = {
                parInfo.p2.x - this->m_xPos,
                parInfo.p1.y + this->m_yPos + this->m_height
            };
            break;

        }

        {
            if ( info.p1.x > info.p2.x ) {
                g_logger.putWarn("Swap in ScreenQuad::makeScreenSpace", __LINE__, __func__, __FILE__);
                std::swap(info.p1.x, info.p2.x);
            }
            if ( info.p1.y > info.p2.y ) {
                g_logger.putWarn("Swap in ScreenQuad::makeScreenSpace", __LINE__, __func__, __FILE__);
                std::swap(info.p1.y, info.p2.y);
            }
        }

        return info;
    }

    const QuadInfo& ScreenQuad::getDeviceSpace(void) const {
        return this->m_deviceSpace;
    }

    void ScreenQuad::onResize(const unsigned int width, const unsigned int height) {
        this->updateDeviceSpace();
    }

    // Private

    void ScreenQuad::updateDeviceSpace(void) {
        this->m_deviceSpace = this->makeScreenSpace().screen2device();
    }

}


namespace dal {

    void QuadRenderer::setColor(const float r, const float g, const float b, const float a) {
        this->m_color.r = r;
        this->m_color.g = g;
        this->m_color.b = b;
        this->m_color.a = a;
    }

    void QuadRenderer::setUpsideDown_diffuseMap(const bool v) {
        this->m_upsideDown_diffuseMap = v;
    }

    void QuadRenderer::setUpsideDown_maskMap(const bool v) {
        this->m_upsideDown_maskMap = v;
    }

    void QuadRenderer::renderQuad(const UnilocOverlay& uniloc, const QuadInfo& devSpc) {
        this->statelessRender(
            uniloc, devSpc.p1, devSpc.p2, this->m_color, this->m_diffuseMap, this->m_maskMap,
            this->m_upsideDown_diffuseMap, this->m_upsideDown_maskMap
        );
    }

    void QuadRenderer::renderQuad(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1, const glm::vec2& devSpcP2) {
        this->statelessRender(
            uniloc, devSpcP1, devSpcP2, this->m_color, this->m_diffuseMap, this->m_maskMap,
            this->m_upsideDown_diffuseMap, this->m_upsideDown_maskMap
        );
    }

    void QuadRenderer::statelessRender(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1,
        const glm::vec2& devSpcP2, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap,
        const bool upsideDown_diffuseMap, const bool upsideDown_maskMap
    ) {
        uniloc.point1(devSpcP1);
        uniloc.point2(devSpcP2);
        uniloc.color(color);
        uniloc.upsideDownDiffuseMap(upsideDown_diffuseMap);
        uniloc.upsideDownMaskMap(upsideDown_maskMap);

        if ( nullptr != diffuseMap ) {
            diffuseMap->sendUniform(uniloc.getDiffuseMap());
        }
        else {
            uniloc.getDiffuseMap().setFlagHas(false);
        }

        if ( nullptr != maskMap ) {
            maskMap->sendUniform(uniloc.getMaskMap());
        }
        else {
            uniloc.getMaskMap().setFlagHas(false);
        }

        RealQuadRenderer::getinst().renderOverlay();
    }

    void QuadRenderer::statelessRender(const UnilocOverlay& uniloc, const QuadInfo& devSpc, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap)
    {
        statelessRender(uniloc, devSpc.p1, devSpc.p2, color, diffuseMap, maskMap, upsideDown_diffuseMap, upsideDown_maskMap);
    }

}


// Widget
namespace dal {

    Widget::Widget(Widget* parent)
        : m_parent(parent)
    {
        this->setParent(parent);
    }

}