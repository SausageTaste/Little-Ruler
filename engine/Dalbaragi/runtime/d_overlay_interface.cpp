#include "d_overlay_interface.h"

#include <fmt/format.h>

#include "d_logger.h"
#include "u_fileutils.h"


namespace {

    class RealQuadRenderer {

    private:
        GLuint mBufferObj;
        GLuint mVao;

        RealQuadRenderer(void) {
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
        static RealQuadRenderer& getinst(void) {
            static RealQuadRenderer inst;
            return inst;
        }

        void drawArray(void) {
            glBindVertexArray(this->mVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    };


    void renderQuadOverlay(const dal::UniRender_Overlay& uniloc, const dal::OverlayDrawInfo& info)
    {
        uniloc.bottomLeft(info.m_pos);
        uniloc.rectSize(info.m_dimension);
        uniloc.colorDefault(info.m_color);
        uniloc.texOffset(info.m_texOffset);
        uniloc.texScale(info.m_texScale);

        if ( nullptr != info.m_albedoMap ) {
            auto albedomap = dynamic_cast<const dal::OverlayTexInterf*>(info.m_albedoMap);
            albedomap->m_tex.sendUniform(uniloc.colorMap());
        }
        else {
            uniloc.colorMap().setFlagHas(false);
        }

        if ( nullptr != info.m_maskMap ) {
            auto maskmap = dynamic_cast<const dal::OverlayTexInterf*>(info.m_maskMap);
            maskmap->m_tex.sendUniform(uniloc.maskMap());
        }
        else {
            uniloc.maskMap().setFlagHas(false);
        }

        RealQuadRenderer::getinst().drawArray();
    }

}


namespace dal {

    void OverlayTexInterf::init_maskMap(const uint8_t* const image, const unsigned width, const unsigned height) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Text looks broken without this.
        this->m_tex.init_maskMap(image, width, height);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }

}


namespace dal {

    void drawOverlay(const OverlayDrawInfo& info, const void* const uniloc) {
        dalAssert(nullptr != uniloc);
        renderQuadOverlay(*reinterpret_cast<const UniRender_Overlay*>(uniloc), info);
    }

    bool loadFileBuf(const char* const respath, std::vector<uint8_t>& result) {
        if ( !dal::loadFileBuffer(respath, result) ) {
            dalAbort(fmt::format("Failed to load font file: {}", respath));
        }

        return true;
    }

    OverlayTexture* genOverlayTexture(void) {
        return new OverlayTexInterf;
    }

}
