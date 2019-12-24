#include "d_opengl_shader.h"

#include <iostream>

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>


namespace {

    auto glfunc(void) {
        return QOpenGLContext::currentContext()->extraFunctions();
    }

}


// Shader utils
namespace {

    enum class ShaderType { vertex, fragment };

    class ShaderRAII {

    private:
        GLuint m_id = 0;

    public:
        ShaderRAII(const ShaderRAII&) = delete;
        ShaderRAII& operator=(const ShaderRAII&) = delete;

    public:
        ShaderRAII(void) = default;
        explicit ShaderRAII(const GLuint shader) noexcept
            : m_id(shader)
        {

        }
        ShaderRAII(ShaderRAII&& other) noexcept
            : m_id(other.m_id)
        {
            other.m_id = 0;
        }

        ShaderRAII& operator=(ShaderRAII&& other) noexcept {
            this->invalidate();
            this->m_id = other.m_id;
            other.m_id = 0;
            return *this;
        }
        ShaderRAII& operator=(const GLuint shader) noexcept {
            this->invalidate();
            this->m_id = shader;
            return *this;
        }

        GLuint operator*(void) const {
            return this->m_id;
        }

        ~ShaderRAII(void) {
            this->invalidate();
        }

        void reset(const GLuint shader) {
            this->invalidate();
            this->m_id = shader;
        }

        GLuint get(void) const {
            return this->m_id;
        }
        bool isValid(void) const {
            return 0 != this->m_id;
        }

    private:
        void invalidate(void) {
            if ( 0 != this->m_id ) {
                glfunc()->glDeleteShader(this->m_id);
                this->m_id = 0;
            }
        }

    };

    constexpr auto mapShaderType(ShaderType typ) {
        if ( ShaderType::vertex == typ ) {
            return GL_VERTEX_SHADER;
        }
        else if ( ShaderType::fragment == typ ) {
            return GL_FRAGMENT_SHADER;
        }
        else {
            assert(false && "Unknown element of dal::gl::ShaderType.");
        }
    }

    ShaderRAII compileShader(const ShaderType shaderType, const char* src) {
        auto f = glfunc();
        ShaderRAII shaderID;

        // Create shader
        {
            const auto shaderEnum = mapShaderType(shaderType);
            shaderID = f->glCreateShader(shaderEnum);

            assert(shaderID.isValid() && "Failed to create shader object.");
        }

        f->glShaderSource(*shaderID, 1, &src, nullptr);
        f->glCompileShader(*shaderID);

        GLint success = GL_FALSE;
        f->glGetShaderiv(*shaderID, GL_COMPILE_STATUS, &success);
        if ( GL_TRUE != success ) {
            constexpr auto SHADER_COMPILER_LOG_BUF_SIZE = 2048;
            char log[SHADER_COMPILER_LOG_BUF_SIZE];
            GLsizei length = 0;
            f->glGetShaderInfoLog(*shaderID, SHADER_COMPILER_LOG_BUF_SIZE, &length, log);

            std::string errmsg = "Shader compile failed. Error message from OpenGL is\n";
            errmsg += log;
            errmsg += "\n\nAnd shader source is\n\n";
            errmsg += (src);
            errmsg += '\n';

            std::cout << errmsg.c_str() << '\n';
            assert(false);
        }

        return std::move(shaderID);
    }

}


// Shader
namespace dal::gl {

    void Shader::init(const char* const vertSrc, const char* const fragSrc) {
        auto f = glfunc();

        this->m_program = f->glCreateProgram();
        assert(0 != this->m_program);

        const auto vertShader = compileShader(ShaderType::vertex, vertSrc);
        const auto fragShader = compileShader(ShaderType::fragment, fragSrc);

        f->glAttachShader(this->m_program, *vertShader);
        f->glAttachShader(this->m_program, *fragShader);

        f->glLinkProgram(this->m_program);

        GLint programSuccess = GL_TRUE;
        f->glGetProgramiv(this->m_program, GL_LINK_STATUS, &programSuccess);
        if ( programSuccess != GL_TRUE ) {
            constexpr GLsizei BUF_SIZE = 512;
            char log[BUF_SIZE];
            GLsizei length = 0;
            f->glGetProgramInfoLog(this->m_program, BUF_SIZE, &length, log);

            const auto errmsg = std::string{ "ShaderProgram linking error occured. Here's log:\n" } + log + '\n';
            assert(false && errmsg.c_str());
        }
    }

    void Shader::use(void) const {
        glfunc()->glUseProgram(this->m_program);
    }

    gl::int_t Shader::getUniloc(const char* const name) const {
        return glfunc()->glGetUniformLocation(this->m_program, name);
    }

}


namespace dal::gl::univar {

    void Int::operator<<(const gl::int_t x) const {
        glfunc()->glUniform1i(this->loc(), x);
    }

    void Float::operator<<(const gl::float_t x) const {
        glfunc()->glUniform1f(this->loc(), x);
    }

    void Vec3::operator<<(const glm::vec3& v) const {
        this->send(v.x, v.y, v.z);
    }
    void Vec3::send(const float x, const float y, const float z) const {
        glfunc()->glUniform3f(this->loc(), x, y, z);
    }

    void Mat4::operator<<(const glm::mat4& m) const {
        glfunc()->glUniformMatrix4fv(this->loc(), 1, GL_FALSE, &m[0][0]);
    }

}