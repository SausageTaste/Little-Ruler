#pragma once

#include <string>

#include <glm/glm.hpp>

#include "d_opengl.h"


// Shader
namespace dal::gl {

    class Shader {

    private:
        gl::uint_t m_program = 0;

    public:
        void init(const char* const vertSrc, const char* const fragSrc);
        void init(const std::string& vertSrc, const std::string& fragSrc) {
            this->init(vertSrc.c_str(), fragSrc.c_str());
        }
        void use(void) const;
        gl::int_t getUniloc(const char* const name) const;
        gl::int_t getUniloc(const std::string& name) const {
            return this->getUniloc(name.c_str());
        }

    };

}


// Uniform variable templates
namespace dal::gl::univar {

    class IUnivar {

    private:
        gl::int_t m_loc = -1;

    public:
        void operator>>(const gl::int_t loc) {
            this->m_loc = loc;
        }
        operator bool(void) const {
            return -1 != this->loc();
        }

        void invalidate(void) {
            this->m_loc = -1;
        }

    protected:
        gl::int_t loc(void) const {
            return this->m_loc;
        }

    };


    class Bool : public IUnivar {

    public:
        void operator<<(const bool x) const;

    };

    class Int : public IUnivar {

    public:
        void operator<<(const gl::int_t x) const;

    };

    class Float : public IUnivar {

    public:
        void operator<<(const gl::float_t x) const;

    };

    class Vec3 : public IUnivar {

    public:
        void operator<<(const glm::vec3& v) const;
        void send(const float x, const float y, const float z) const;

    };

    class Mat4 : public IUnivar {

    public:
        void operator<<(const glm::mat4& m) const;

    };

}
