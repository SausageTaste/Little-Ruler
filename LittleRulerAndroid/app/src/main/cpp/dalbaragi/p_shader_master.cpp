#include "p_shader_master.h"

#include <array>
#include <unordered_map>
#include <memory>

#include <fmt/format.h>

#include "u_fileclass.h"
#include "s_logger_god.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

    void setFor_generalRender(void) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void setFor_fillingScreen(void) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void setFor_shadowmap(void) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4.0f, 100.0f);

    }

    void setFor_overlay(void) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void setFor_water(void) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
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

    enum class ShaderType { vertex, fragment };

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

    ShaderRAII compileShader2(const ShaderType type, const char* const src) {
        // Returns 0 on error
        GLuint shaderID = 0;

        switch ( type ) {
        case ShaderType::vertex:
            shaderID = glCreateShader(GL_VERTEX_SHADER);
            break;
        case ShaderType::fragment:
            shaderID = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        }

        dalAssertm(shaderID != 0, "Failed to create shader object.");

        glShaderSource(shaderID, 1, &src, NULL);
        glCompileShader(shaderID);

        GLint vShaderCompiled = GL_FALSE;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &vShaderCompiled);
        if ( vShaderCompiled != GL_TRUE ) {
            constexpr auto k_shaderLogBufSize = 2048;
            GLsizei length = 0;
            char log[k_shaderLogBufSize];
            glGetShaderInfoLog(shaderID, k_shaderLogBufSize, &length, log);
            const auto errMsg = "Shader compile failed. Error message from OpenGL is\n"s + log + "\n\nAnd shader source is\n\n" +
                makeNumberedText(src) + '\n';
            dalAbort(errMsg);
        }

        return ShaderRAII{ shaderID };
    }

}


// ShaderLoader
namespace dal {

    class ShaderLoader {

    private:
        enum class Precision { nodef, high, mediump };
        enum class Defined { parse_fail, ignore_this, include };

    private:
        std::unordered_map<std::string, std::string> m_soures;

    public:
        ShaderLoader(void) {
            constexpr std::array<const char*, 12> k_fileNames = {
                "depth.vert",
                "depth.frag",
                "fillscreen.vert",
                "fillscreen.frag",
                "general.frag",
                "general.vert",
                "overlay.vert",
                "overlay.frag",
                "water.vert",
                "water.frag",
                "animated.vert",
                "animated.frag"
            };

            for ( const auto fileName : k_fileNames ) {
                this->orderShaderSrc(fileName);
            }

            for ( auto& spair : m_soures ) {
                const auto shaderType = determineShaderType(spair.first);

                switch ( shaderType ) {
                case ShaderType::vertex:
                    spair.second = makeHeader(Precision::nodef) + spair.second;
                    break;
                case ShaderType::fragment:
#if defined(_WIN32)
                    spair.second = makeHeader(Precision::nodef) + spair.second;
#elif defined(__ANDROID__)
                    spair.second = makeHeader(Precision::mediump) + spair.second;
#endif
                    break;
                }
            }
        }

        const char* const operator[](const std::string& key) {
            const auto iter = this->m_soures.find(key);
            if ( this->m_soures.end() == iter ) dalAbort("\'{}\' not exist in ShaderLoader."_format(key));
            return iter->second.c_str();
        }

    private:
        std::string preprocess(std::string src) {
            size_t lastTail = 0;
            while ( true ) {
                const auto head = src.find("#", lastTail);
                if ( std::string::npos == head ) break;
                auto tail = src.find("\n", head);
                if ( std::string::npos == tail ) tail = src.size() - 1;

                std::vector<std::string> args;
                const auto resDef = this->parseDefine(src.substr(head, tail - head), args);
                if ( Defined::include == resDef ) {
                    const auto& toInclude = this->orderShaderSrc(args[0]);
                    src = src.substr(0, head) + toInclude + src.substr(tail, src.size() - tail);
                    lastTail = head + toInclude.size() + 1;
                }
                else if ( Defined::ignore_this ==resDef ) {
                    lastTail = tail + 1;
                    continue;
                }
                else if ( Defined::parse_fail == resDef ) {
                    dalAbort("Error during preprocessing");
                }
            }

            return src;
        }

        const std::string& orderShaderSrc(const std::string& fileName) {
            auto found = this->m_soures.find(fileName);
            if ( this->m_soures.end() == found ) {
                auto file = dal::resopen("asset::glsl/"s + fileName, dal::FileMode::read);
                if ( nullptr == file ) dalAbort("Failed to load shader source file: "s + fileName);

                auto iter = this->m_soures.emplace(fileName, "");
                if ( false == iter.second ) {
                    dalAbort("Failed to add element into ShaderLoader::m_soures.");
                }
                auto& buffer = iter.first->second;
                if ( !file->readText(buffer) ) dalAbort("Failed to read shader source file: "s + fileName);

                buffer = this->preprocess(buffer);
                return buffer;
            }
            else {
                return found->second;
            }
        }

        // Static

        static Defined parseDefine(const std::string& text, std::vector<std::string>& results) {
            {
                const auto head = text.find("include");
                if ( std::string::npos != head ) {
                    const auto argHead = text.find('<', head);
                    const auto argTail = text.find('>', argHead);
                    if ( std::string::npos ==  argHead || std::string::npos ==argTail ) {
                        dalError("Error during parsing #include: "s + text);
                        return Defined::parse_fail;
                    }

                    const auto fileToInclude = text.substr(argHead + 1, argTail - argHead - 1);
                    results.clear();
                    results.emplace_back(fileToInclude);

                    return Defined::include;
                }
            }

            {
                const auto head = text.find("#ifdef GL_ES");
                if ( std::string::npos != head ) return Defined::ignore_this;
            }
            {
                const auto head = text.find("#endif");
                if ( std::string::npos != head ) return Defined::ignore_this;
            }
            {
                const auto head = text.find("#else");
                if ( std::string::npos != head ) return Defined::ignore_this;
            }
            {
                const auto head = text.find("#ifndef GL_ES");
                if ( std::string::npos != head ) return Defined::ignore_this;
            }

            dalError("Unknown define in shader: "s + text);
            return Defined::parse_fail;
        }

        static ShaderType determineShaderType(const std::string& fileName) {
            if ( std::string::npos != fileName.find(".vert") ) {
                return ShaderType::vertex;
            }
            else if ( std::string::npos != fileName.find(".frag") ) {
                return ShaderType::fragment;
            }
            else {
                dalAbort("Can't determine shader type for: "s + fileName);
            }
        }

        static std::string makeHeader(const Precision precision) {
#if defined(_WIN32)
            std::string fileHeader = "#version 330 core\n";
#elif defined(__ANDROID__)
            std::string fileHeader = "#version 300 es\n";
#endif
            switch ( precision ) {
            case Precision::nodef:
                break;
            case Precision::high:
                fileHeader += "precision highp float;\n";
                break;
            case Precision::mediump:
                fileHeader += "precision mediump float;\n";
                break;
            }

            return fileHeader;
        }

    };

}


// ShaderProgram2
namespace dal {

    void ShaderProgram2::init(ShaderLoader& loader, const char* const vertSrcName, const char* const fragSrcName) {
        dalAssert(this->m_id == 0);

        this->m_id = glCreateProgram();

        const auto verShader = compileShader2(ShaderType::vertex, loader[vertSrcName]);
        const auto fragShader = compileShader2(ShaderType::fragment, loader[fragSrcName]);

        glAttachShader(this->m_id, verShader.get());
        glAttachShader(this->m_id, fragShader.get());

        glLinkProgram(this->m_id);

        GLint programSuccess = GL_TRUE;
        glGetProgramiv(this->m_id, GL_LINK_STATUS, &programSuccess);
        if ( programSuccess != GL_TRUE ) {
            GLsizei length = 0;
            char log[100];
            glGetProgramInfoLog(this->m_id, 100, &length, log);
            dalAbort("ShaderProgram linking error: "s + log);
        }
    }

    GLuint ShaderProgram2::get(void) {
        return this->m_id;
    }

    void ShaderProgram2::use(void) const {
        glUseProgram(this->m_id);
    }

}


// Shader Master
namespace dal {

    ShaderMaster::ShaderMaster(void) {
        ShaderLoader loader;

        this->m_general.init(loader, "general.vert", "general.frag");
        this->m_generalUniloc.init(this->m_general.get());

        this->m_fscreen.init(loader, "fillscreen.vert", "fillscreen.frag");
        this->m_fscreenUniloc.init(this->m_fscreen.get());

        this->m_depthmap.init(loader, "depth.vert", "depth.frag");
        this->m_depthmapUniloc.init(this->m_depthmap.get());

        this->m_overlay.init(loader, "overlay.vert", "overlay.frag");
        this->m_overlayUniloc.init(this->m_overlay.get());

        this->m_waterry.init(loader, "water.vert", "water.frag");
        this->m_waterryUniloc.init(this->m_waterry.get());

        this->m_animate.init(loader, "animated.vert", "animated.frag");
        this->m_animateUniloc.init(this->m_animate.get());
    }

    const UnilocGeneral& ShaderMaster::useGeneral(void) const {
        setFor_generalRender();
        this->m_general.use();
        return this->m_generalUniloc;
    }

    const UnilocDepthmp& ShaderMaster::useDepthMp(void) const {
        setFor_shadowmap();
        this->m_depthmap.use();
        return this->m_depthmapUniloc;
    }

    const UnilocFScreen& ShaderMaster::useFScreen(void) const {
        setFor_fillingScreen();
        this->m_fscreen.use();
        return this->m_fscreenUniloc;
    }

    const UnilocOverlay& ShaderMaster::useOverlay(void) const {
        setFor_overlay();
        this->m_overlay.use();
        return this->m_overlayUniloc;
    }

    const UnilocWaterry& ShaderMaster::useWaterry(void) const {
        setFor_water();
        this->m_waterry.use();
        return this->m_waterryUniloc;
    }

    const UnilocAnimate& ShaderMaster::useAnimate(void) const {
        setFor_generalRender();
        this->m_animate.use();
        return this->m_animateUniloc;
    }

}