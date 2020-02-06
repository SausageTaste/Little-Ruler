#pragma once

#include "d_opengl.h"


// Mesh
namespace dal::gl {

    class Mesh {

    private:
        uint_t m_vao = 0;
        uint_t m_buffers[5] = { 0 };
        size_t m_numVertices = 0;

    public:
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

    public:
        Mesh(void) = default;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh(void);

        void draw(void) const;

        void initStatic(const size_t numVert, const float_t* const vertices, const float_t* const texcoords, const float_t* const normals);
        void initAnimated(const size_t numVert, const float_t* const vertices, const float_t* const texcoords, const float_t* const normals,
            const int_t* const boneids, const float* const weights);

    private:
        bool isReady(void) const;

        template <unsigned int _Index, gl::Type _dataType>
        void fillBufferData(const void* const arr, const size_t arraySize, const size_t tupleSize);

        void createBuffers(const unsigned numBufs);
        void invalidate(void);
        void setAllToZero(void);

    };

}
