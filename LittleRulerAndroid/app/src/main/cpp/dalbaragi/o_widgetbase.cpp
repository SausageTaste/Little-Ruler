#include "o_widgetbase.h"

#include "s_logger_god.h"


// Util classes
namespace {

    class RealQuadRenderer2 {

    private:
        GLuint mBufferObj;
        GLuint mVao;

        RealQuadRenderer2(void) {
            glGenVertexArrays(1, &mVao);
            if ( mVao <= 0 ) {
                dalAbort("Failed to generate vertex array.");
            }
            glGenBuffers(1, &mBufferObj);
            if ( mBufferObj <= 0 ) {
                dalAbort("Failed to generate gl buffers.");
            }

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
        static RealQuadRenderer2& getinst(void) {
            static RealQuadRenderer2 inst;
            return inst;
        }

        void drawArray(void) {
            glBindVertexArray(this->mVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    };

}


// Header functions
namespace dal {

    inline glm::vec2 screen2device(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
             2.0f * p.x / (winWidth) - 1.0f,
            -2.0f * p.y / (winHeight) + 1.0f
        };
    }

    inline glm::vec2 screen2device(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return screen2device(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }

    inline glm::vec2 device2screen(const glm::vec2& p, const float winWidth, const float winHeight) {
        return glm::vec2{
            (p.x + 1.0f) * (winWidth) / 2.0f,
            (1.0f - p.y) * (winHeight) / 2.0f
        };
    }

    inline glm::vec2 device2screen(const glm::vec2& p, const unsigned int winWidth, const unsigned int winHeight) {
        return device2screen(p, static_cast<float>(winWidth), static_cast<float>(winHeight));
    }


    void renderQuadOverlay(const UnilocOverlay& uniloc, const glm::vec2& devSpcP1, const glm::vec2& devSpcP2, const glm::vec4& color,
        const Texture* const diffuseMap, const Texture* const maskMap, const bool upsideDown_diffuseMap, const bool upsideDown_maskMap,
        const glm::vec2& texOffset, const glm::vec2& texScale)
    {
        uniloc.point1(devSpcP1);
        uniloc.point2(devSpcP2);
        uniloc.color(color);
        uniloc.upsideDownDiffuseMap(upsideDown_diffuseMap);
        uniloc.upsideDownMaskMap(upsideDown_maskMap);
        uniloc.texOffset(texOffset);
        uniloc.texScale(texScale);

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

        RealQuadRenderer2::getinst().drawArray();
    }

    void renderQuadOverlay(const UnilocOverlay& uniloc, const QuadRenderInfo& info) {
        renderQuadOverlay(uniloc, info.m_devSpcP1, info.m_devSpcP2, info.m_color, info.m_diffuseMap, info.m_maskMap,
            info.m_upsideDown_diffuse, info.m_upsideDown_mask, info.m_texOffset, info.m_texScale);
    }

}


// ScreenSpaceBox
namespace dal {

    bool IScreenSpaceBox::isPointInside(const float x, const float y) const {
        const auto p1 = this->getPoint00();
        const auto p2 = this->getPoint11();

        if ( x < p1.x ) {
            return false;
        }
        else if ( y < p1.y ) {
            return false;
        }
        else if ( x > p2.x ) {
            return false;
        }
        else if ( y > p2.y ) {
            return false;
        }
        else {
            return true;
        }
    }

    std::pair<glm::vec2, glm::vec2> IScreenSpaceBox::makeDeviceSpace(const float width, const float height) const {
        std::pair<glm::vec2, glm::vec2> result;

        result.first = screen2device(this->getPoint01(), width, height);
        result.second = screen2device(this->getPoint10(), width, height);

        dalAssert(result.first.x <= result.second.x);
        dalAssert(result.first.y <= result.second.y);

        return result;
    }

}


// Widget2
namespace dal {

    Widget2::Widget2(Widget2* const parent)
        : m_parent(parent)
    {

    }

}
