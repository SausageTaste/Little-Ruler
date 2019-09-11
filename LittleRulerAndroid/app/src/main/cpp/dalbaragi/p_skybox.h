#pragma once

#include "p_uniloc.h"
#include "p_meshStatic.h"


namespace dal {

    class Skybox {

    private:
        GLuint m_vao = 0, m_vbo = 0;
        CubeMap m_cubeMap;

    public:
        Skybox(void);

        void render(const UnilocSkybox& uniloc) const;

    };

}
