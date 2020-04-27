#include "p_shader_master.h"

#include <array>
#include <unordered_map>
#include <memory>

#include <fmt/format.h>

#include <d_logger.h>
#include <d_shaderProcessor.h>

#include "u_fileutils.h"


using namespace fmt::literals;


namespace {

    void setFor_generalRender(void) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    void setFor_fillingScreen(void) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    void setFor_shadowmap(void) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(4.0f, 100.0f);
        //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

    void setFor_overlay(void) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_POLYGON_OFFSET_FILL);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    void setFor_water(void) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_POLYGON_OFFSET_FILL);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    void setFor_skybox(void) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

}


namespace {

    std::string makeNumberedText(const std::string& text) {
        std::string buffer;
        size_t head = 0;
        size_t lineNum = 1;

        while ( true ) {
            const auto tail = text.find('\n', head);
            if ( std::string::npos != tail ) {
                const auto line = text.substr(head, tail - head);
                {
                    buffer += "{:0>3}  {}\n"_format(lineNum++, line);
                }
                head = tail + 1;
            }
            else {
                const auto line = text.substr(head);
                {
                    buffer += "{:0>3}  {}\n"_format(lineNum++, line);
                }
                break;
            }
        }

        return buffer;
    }

    class ShaderRAII {

    private:
        const GLuint m_id;

    public:
        explicit ShaderRAII(GLuint shader)
            : m_id(shader)
        {

        }

        ~ShaderRAII(void) {
            glDeleteShader(this->m_id);
        }

        GLuint get(void) const {
            return this->m_id;
        }

    };

    ShaderRAII compileShader2(const dal::ShaderType type, const char* const src) {
        // Returns 0 on error
        GLuint shaderID = 0;

        switch ( type ) {
        case dal::ShaderType::vertex:
            shaderID = glCreateShader(GL_VERTEX_SHADER);
            break;
        case dal::ShaderType::fragment:
            shaderID = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        }

        dalAssertm(shaderID != 0, "Failed to create shader object.");

        glShaderSource(shaderID, 1, &src, NULL);
        glCompileShader(shaderID);

        GLint vShaderCompiled = GL_FALSE;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &vShaderCompiled);
        if ( vShaderCompiled != GL_TRUE ) {
            constexpr auto SHADER_COMPILER_LOG_BUF_SIZE = 2048;
            GLsizei length = 0;
            char log[SHADER_COMPILER_LOG_BUF_SIZE];
            glGetShaderInfoLog(shaderID, SHADER_COMPILER_LOG_BUF_SIZE, &length, log);
            const auto errMsg = "Shader compile failed. Error message from OpenGL is\n{}\n\nAnd shader source is\n\n{}\n"_format(log, makeNumberedText(src));
            dalAbort(errMsg);
        }

        return ShaderRAII{ shaderID };
    }

}


namespace {

    dal::ShaderPreprocessor g_loader;

}


// ShaderProgram
namespace dal {

    ShaderProgram::ShaderProgram(const std::string& vertSrc, const std::string& fragSrc) {
        this->init(vertSrc, fragSrc);
    }

    void ShaderProgram::init(const char* const vertSrc, const char* const fragSrc) {
        dalAssert(0 == this->m_id);
        this->m_id = glCreateProgram();
        dalAssert(0 != this->m_id);

        const auto vertShader = compileShader2(ShaderType::vertex, vertSrc);
        const auto fragShader = compileShader2(ShaderType::fragment, fragSrc);

        glAttachShader(this->m_id, vertShader.get());
        glAttachShader(this->m_id, fragShader.get());

        glLinkProgram(this->m_id);

        GLint programSuccess = GL_TRUE;
        glGetProgramiv(this->m_id, GL_LINK_STATUS, &programSuccess);
        if ( programSuccess != GL_TRUE ) {
            GLsizei length = 0;
            char log[100];
            glGetProgramInfoLog(this->m_id, 100, &length, log);
            dalAbort("ShaderProgram linking error occured. Here's log:\n{}"_format(log));
        }
    }

    void ShaderProgram::init(const std::string& vertSrc, const std::string& fragSrc) {
        this->init(vertSrc.c_str(), fragSrc.c_str());
    }

    GLuint ShaderProgram::get(void) const {
        dalAssert(0 != this->m_id);
        return this->m_id;
    }

    void ShaderProgram::use(void) const {
        glUseProgram(this->get());
    }

}


// Shader Master
namespace dal {

    ShaderMaster::ShaderMaster(void)
        : m_overlay(g_loader["overlay.vert"], g_loader["overlay.frag"])
        , m_overlayUniloc(m_overlay.get())
        , m_skybox(g_loader["skybox.vert"], g_loader["skybox.frag"])
        , m_skyboxUniloc(m_skybox.get())
    {
        this->m_static.init(g_loader["r_static.vert"], g_loader["r_static.frag"]);
        this->m_animated.init(g_loader["r_animated.vert"], g_loader["r_static.frag"]);
        this->m_static_depth.init(g_loader["r_static_depth.vert"], g_loader["r_empty.frag"]);
        this->m_animatedDepth.init(g_loader["r_animated_depth.vert"], g_loader["r_empty.frag"]);
        this->m_static_onWater.init(g_loader["r_static_onwater.vert"], g_loader["r_static_onwater.frag"]);
        this->m_animated_onWater.init(g_loader["r_animated_onwater.vert"], g_loader["r_static_onwater.frag"]);
        this->m_fillScreen.init(g_loader["r_fillscreen.vert"], g_loader["r_fillscreen.frag"]);
        this->m_water.init(g_loader["r_water.vert"], g_loader["r_water.frag"]);

        this->u_static.set(this->m_static.get());
        this->u_animated.set(this->m_animated.get());
        this->u_static_depth.set(this->m_static_depth.get());
        this->u_animatedDepth.set(this->m_animatedDepth.get());
        this->u_static_onWater.set(this->m_static_onWater.get());
        this->u_animated_onWater.set(this->m_animated_onWater.get());
        this->u_fillScreen.set(this->m_fillScreen.get());
        this->u_water.set(this->m_water.get());

        g_loader.clear();
    }

    const UnilocOverlay& ShaderMaster::useOverlay(void) const {
        setFor_overlay();
        this->m_overlay.use();
        return this->m_overlayUniloc;
    }

    const UnilocSkybox& ShaderMaster::useSkybox(void) const {
        setFor_skybox();
        this->m_skybox.use();
        return this->m_skyboxUniloc;
    }


    const UniRender_Static& ShaderMaster::useStatic(void) const {
        setFor_generalRender();
        this->m_static.use();
        return this->u_static;
    }

    const UniRender_Animated& ShaderMaster::useAnimated(void) const {
        setFor_generalRender();
        this->m_animated.use();
        return this->u_animated;
    }

    const UniRender_StaticDepth& ShaderMaster::useStaticDepth(void) const {
        setFor_shadowmap();
        this->m_static_depth.use();
        return this->u_static_depth;
    }

    const UniRender_AnimatedDepth& ShaderMaster::useAnimatedDepth(void) const {
        setFor_shadowmap();
        this->m_animatedDepth.use();
        return this->u_animatedDepth;
    }

    const UniRender_StaticOnWater& ShaderMaster::useStaticOnWater(void) const {
        setFor_generalRender();
        this->m_static_onWater.use();
        return this->u_static_onWater;
    }

    const UniRender_AnimatedOnWater& ShaderMaster::useAnimatedOnWater(void) const {
        setFor_generalRender();
        this->m_animated_onWater.use();
        return this->u_animated_onWater;
    }

    const UniRender_FillScreen& ShaderMaster::useFillScreen(void) const {
        setFor_fillingScreen();
        this->m_fillScreen.use();
        return this->u_fillScreen;
    }

    const UniRender_Water& ShaderMaster::useWater(void) const {
        setFor_water();
        this->m_water.use();
        return this->u_water;
    }

}
