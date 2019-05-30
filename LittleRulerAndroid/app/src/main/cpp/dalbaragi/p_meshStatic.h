#pragma once

#include <string>

#include "p_dalopengl.h"


namespace dal {

    class MeshStatic {
        //////// Variables ////////

    private:
        std::string mName;

        GLuint mVao = 0;

        GLuint mVertexArrayBuffer = 0;
        GLuint mTexCoordArrayBuffer = 0;
        GLuint mNormalArrayBuffe = 0;

        unsigned int mVertexSize = 0;

        //////// Functions ////////

    public:
        void draw(void) const;

        int buildData(
            const float* const vertices, const int vertSize,
            const float* const texcors, const int texcorSize,
            const float* const normals, const int norSize
        );
        void destroyData(void);
        bool isReady(void) const;

        void setName(const char* const name);

    private:
        void createBuffers(void);

        void bindVAO(void) const;
        static void unbindVAO(void);

    };

}