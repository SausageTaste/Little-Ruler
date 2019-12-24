#include "d_opengl_renderunit.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>


namespace {

    template<class T> struct always_false : std::false_type {};


    auto glfunc(void) {
        return QOpenGLContext::currentContext()->extraFunctions();
    }

}


// Mesh
namespace dal::gl {

    Mesh::Mesh(Mesh&& other) noexcept
        : m_vao(other.m_vao)
        , m_numVertices(other.m_numVertices)
    {
        for ( unsigned i = 0; i < 5; ++i ) {
            this->m_buffers[i] = other.m_buffers[i];
        }

        other.setAllToZero();
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        this->invalidate();

        this->m_vao = other.m_vao;
        this->m_numVertices = other.m_numVertices;
        for ( unsigned i = 0; i < 5; ++i ) {
            this->m_buffers[i] = other.m_buffers[i];
        }

        other.setAllToZero();
        return *this;
    }

    Mesh::~Mesh(void) {
        this->invalidate();
    }


    void Mesh::draw(void) const {
        if ( !this->isReady() ) {
            throw std::runtime_error{ "MeshStatic::renderDepthmap called without being built." };
        }

        auto f = glfunc();

        f->glBindVertexArray(this->m_vao);
        f->glDrawArrays(GL_TRIANGLES, 0, this->m_numVertices);
        f->glBindVertexArray(0);
    }

    void Mesh::initStatic(const size_t numVert, const float_t* const vertices, const float_t* const texcoords, const float_t* const normals) {
        if ( this->isReady() ) {
            throw std::runtime_error("MeshStatic's data already built.");
        }

        this->createBuffers(3);
        glfunc()->glBindVertexArray(this->m_vao);

        // Vertices
        {
            const auto arraySize = numVert * sizeof(float_t) * 3;
            this->fillBufferData<0, gl::Type::gl_float>(vertices, arraySize, 3);
        }

        // Tex coords
        {
            const auto arraySize = numVert * sizeof(float_t) * 2;
            this->fillBufferData<1, gl::Type::gl_float>(texcoords, arraySize, 2);
        }

        // Normals
        {
            const auto arraySize = numVert * sizeof(float_t) * 3;
            this->fillBufferData<2, gl::Type::gl_float>(normals, arraySize, 3);
        }

        glfunc()->glBindVertexArray(0);
        this->m_numVertices = numVert;
    }

    void Mesh::initAnimated(const size_t numVert, const float_t* const vertices, const float_t* const texcoords,
        const float_t* const normals, const int_t* const boneids, const float* const weights)
    {
        if ( this->isReady() ) {
            throw std::runtime_error("MeshStatic's data already built.");
        }

        this->createBuffers(5);
        glfunc()->glBindVertexArray(this->m_vao);

        // Vertices
        {
            const auto arraySize = numVert * sizeof(float_t) * 3;
            this->fillBufferData<0, gl::Type::gl_float>(vertices, arraySize, 3);
        }

        // Tex coords
        {
            const auto arraySize = numVert * sizeof(float_t) * 2;
            this->fillBufferData<1, gl::Type::gl_float>(texcoords, arraySize, 2);
        }

        // Normals
        {
            const auto arraySize = numVert * sizeof(float_t) * 3;
            this->fillBufferData<2, gl::Type::gl_float>(normals, arraySize, 3);
        }

        // bone ids
        {
            const auto arraySize = numVert * sizeof(int_t) * 3;
            this->fillBufferData<3, gl::Type::gl_int>(boneids, arraySize, 3);
        }

        // weights
        {
            const auto arraySize = numVert * sizeof(float_t) * 3;
            this->fillBufferData<4, gl::Type::gl_float>(weights, arraySize, 3);
        }

        glfunc()->glBindVertexArray(0);
        this->m_numVertices = numVert;
    }


    // Private

    bool Mesh::isReady(void) const {
        return this->m_numVertices != 0;
    }

    template <unsigned int _Index, gl::Type _dataType>
    void Mesh::fillBufferData(const void* const arr, const size_t arraySize, const size_t tupleSize) {
        static_assert(_Index < 5, "Mesh buffer index out of bound.");
        assert(0 != this->m_buffers[_Index]);

        auto f = glfunc();

        f->glBindBuffer(GL_ARRAY_BUFFER, this->m_buffers[_Index]);
        f->glBufferData(GL_ARRAY_BUFFER, arraySize, arr, GL_STATIC_DRAW);

        if constexpr ( gl::Type::gl_float == _dataType ) {
            f->glVertexAttribPointer(_Index, tupleSize, GL_FLOAT, GL_FALSE, 0, nullptr);
        }
        else if constexpr ( gl::Type::gl_int == _dataType ) {
            f->glVertexAttribIPointer(_Index, tupleSize, GL_INT, 0, nullptr);
        }
        else {
            static_assert(always_false<void>::value, "Not supported data type for mesh vertex buffer.");
        }

        f->glEnableVertexAttribArray(_Index);
    }

    void Mesh::createBuffers(const unsigned numBufs) {
        auto f = glfunc();

        f->glGenVertexArrays(1, &this->m_vao);
        if ( this->m_vao <= 0 ) {
            throw std::runtime_error{ "Failed to generate vertex array." };
        }

        f->glGenBuffers(numBufs, this->m_buffers);
        for ( int i = 0; i < numBufs; i++ ) {
            if ( 0 == this->m_buffers[i] ) {
                throw std::runtime_error{ "Failed to generate beffer." };
            }
        }
    }

    void Mesh::invalidate(void) {
        if ( !this->isReady() )
            return;

        auto f = glfunc();

        f->glDeleteBuffers(5, this->m_buffers);
        f->glDeleteVertexArrays(1, &this->m_vao);

        this->setAllToZero();
    }

    void Mesh::setAllToZero(void) {
        this->m_vao = 0;
        this->m_numVertices = 0;
        for ( unsigned int i = 0; i < 5; ++i ) {
            this->m_buffers[i] = 0;
        }
    }

}
