#include "p_skybox.h"

#include "u_fileclass.h"
#include "s_logger_god.h"


// Skybox
namespace dal {

    Skybox::Skybox(void) {
        {
            const float skyboxVertices[] = {
                // positions
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                -1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                 1.0f,  1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f
            };

            glGenVertexArrays(1, &this->m_vao);
            glGenBuffers(1, &this->m_vbo);
            glBindVertexArray(this->m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        }

        {
            const char* imageNames[] = {
                "asset::darkskies_rt.tga",
                "asset::darkskies_lf.tga",
                "asset::darkskies_up.tga",
                "asset::darkskies_dn.tga",
                "asset::darkskies_ft.tga",
                "asset::darkskies_bk.tga",
            };
            dal::binfo::ImageFileData imageInfos[6];
            CubeMap::CubeMapData data;

            for ( int i = 0; i < 6; ++i ) {
                auto& info = imageInfos[i];
                const auto res = futil::getRes_image(imageNames[i], info);
                dalAssert(res);
                if ( i == 2 ) {
                    info.rotate90();
                }
                else if ( 3 == i ) {
                    info.rotate270();
                }
                else {
                    info.rotate180();
                }

                data.set(i, info.m_buf.data(), info.m_width, info.m_height, info.m_pixSize);
            }

            this->m_cubeMap.init(data);
        }
    }

    void Skybox::render(const UnilocSkybox& uniloc) const {
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        // ... set view and projection matrix
        glBindVertexArray(this->m_vao);
        this->m_cubeMap.sendUniform(uniloc.getSkyboxTexLoc());
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

}
