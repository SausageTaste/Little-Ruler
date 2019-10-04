#pragma once

#include <memory>

#include "p_uniloc.h"
#include "p_meshStatic.h"


namespace dal {

    class Skybox {

    private:
        GLuint m_vao = 0, m_vbo = 0;
        std::shared_ptr<const CubeMap> m_cubeMap = nullptr;

    public:
        Skybox(void);

        void setCubeMap(std::shared_ptr<const CubeMap> cubemap) {
            this->m_cubeMap = cubemap;
        }

        void render(const UnilocSkybox& uniloc) const;
        void sendUniform(const UniInterfLightedMesh& uniloc) const;

    };

}
