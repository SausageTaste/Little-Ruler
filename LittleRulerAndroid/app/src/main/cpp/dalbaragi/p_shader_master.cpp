#include "p_shader_master.h"

#include <array>
#include <unordered_map>
#include <memory>

#include <fmt/format.h>

#include "s_logger_god.h"
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


// ShaderLoader
namespace dal {

    class ShaderLoader {

    private:
        enum class Precision { nodef, highp, mediump, lowp };
        enum class Defined { parse_fail, ignore_this, include };

    private:
        std::unordered_map<std::string, std::string> m_soures;

    public:
        /*
        void init(void) {
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
        */

        std::string operator[](const std::string& key) {
            std::string buffer;

            {
                const auto shaderType = determineShaderType(key);

                switch ( shaderType ) {
                case ShaderType::vertex:
#if defined(_WIN32)
                    buffer = makeHeader(Precision::nodef);
#elif defined(__ANDROID__)
                    buffer = makeHeader(Precision::nodef);
#endif
                    break;
                case ShaderType::fragment:
#if defined(_WIN32)
                    buffer = makeHeader(Precision::nodef);
#elif defined(__ANDROID__)
                    buffer = makeHeader(Precision::mediump);
#endif
                    break;
                }
            }

            {
                const auto iter = this->m_soures.find(key);
                if ( this->m_soures.end() == iter ) {
                    buffer += this->getOrLoadSrc(key);
                }
                else {
                    buffer += iter->second;
                }
            }

            return buffer;
        }

        void clear(void) {
            this->m_soures.clear();
            this->m_soures.reserve(0);
        }

    private:
        /*
        const char* loadNewShader(const std::string& fileName) {
            std::string buffer;

            {
                const auto shaderType = determineShaderType(fileName);

                switch ( shaderType ) {
                    case ShaderType::vertex:
                        buffer = makeHeader(Precision::nodef);
                        break;
                    case ShaderType::fragment:
#if defined(_WIN32)
                        buffer = makeHeader(Precision::nodef);
#elif defined(__ANDROID__)
                        buffer = makeHeader(Precision::mediump);
#endif
                        break;
                }
            }

            {
                auto file = dal::resopen("asset::glsl/"s + fileName, dal::FileMode::read);
                if ( nullptr == file ) dalAbort("Failed to load shader source file: "s + fileName);
                std::string fileBuf;
                if ( !file->readText(fileBuf) ) dalAbort("Failed to read shader source file: "s + fileName);
                buffer += this->preprocess(fileBuf);
            }

            auto [iter, success] = this->m_soures.emplace(fileName, std::move(buffer));
            dalAssertm(success, "Failed to add element into ShaderLoader::m_soures.");

            return iter->second.c_str();
        }
        */

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
                    const auto& toInclude = this->getOrLoadSrc(args[0]);
                    src = src.substr(0, head) + toInclude + src.substr(tail, src.size() - tail);
                    lastTail = head + toInclude.size() + 1;
                }
                else if ( Defined::ignore_this == resDef ) {
                    lastTail = tail + 1;
                    continue;
                }
                else if ( Defined::parse_fail == resDef ) {
                    dalAbort("Error during preprocessing");
                }
            }

            return src;
        }

        /*
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
        */

        std::string& getOrLoadSrc(const std::string& fileName) {
            const auto found = this->m_soures.find(fileName);

            if ( this->m_soures.end() != found ) {
                return found->second;
            }
            else {
                auto [iter, success] = this->m_soures.emplace(fileName, "");
                dalAssert(success);
                if ( !dal::loadFileText("asset::glsl/{}"_format(fileName).c_str(), iter->second) ) {
                    dalAbort("Failed to load glsl file: {}"_format(fileName));
                }
                iter->second = this->preprocess(iter->second);
                return iter->second;
            }
        }

        // Static

        static Defined parseDefine(const std::string& text, std::vector<std::string>& results) {
            {
                const auto head = text.find("include");
                if ( std::string::npos != head ) {
                    const auto argHead = text.find('<', head);
                    const auto argTail = text.find('>', argHead);
                    if ( std::string::npos == argHead || std::string::npos == argTail ) {
                        dalError("Error during parsing \"#include\": {}"_format(text));
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

            dalError("Unknown define in shader: {}"_format(text));
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
                dalAbort("Can't determine shader type for: {}"_format(fileName));
            }
        }

        static std::string makeHeader(const Precision precision) {
            static const auto types = {
                "float", "sampler2D"
            };

#if defined(_WIN32)
            std::string fileHeader = "#version 330 core\n";
#elif defined(__ANDROID__)
            std::string fileHeader = "#version 300 es\n";
#endif

            if ( Precision::nodef != precision ) {
                const char* pstr = nullptr;

                switch ( precision ) {
                case Precision::highp:
                    pstr = "highp";
                    break;
                case Precision::mediump:
                    pstr = "mediump";
                    break;
                case Precision::lowp:
                    pstr = "lowp";
                    break;
                default:
                    dalAbort("Unkown precision qualifier.");
                }

                for ( auto x : types ) {
                    fileHeader += "precision {} {};\n"_format(pstr, x);
                }
            }

            return fileHeader;
        }

    } g_loader;

}


// ShaderProgram
namespace dal {

    ShaderProgram::ShaderProgram(const char* const vertSrc, const char* const fragSrc) {
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

    ShaderProgram::ShaderProgram(const std::string& vertSrc, const std::string& fragSrc)
        : ShaderProgram(vertSrc.c_str(), fragSrc.c_str())
    {

    }

    GLuint ShaderProgram::get(void) {
        return this->m_id;
    }

    void ShaderProgram::use(void) const {
        glUseProgram(this->m_id);
    }

}


// Shader Master
namespace dal {

    ShaderMaster::ShaderMaster(void)
        : m_general(g_loader["general.vert"], g_loader["general.frag"])
        , m_generalUniloc(m_general.get())
        , m_fscreen(g_loader["fillscreen.vert"], g_loader["fillscreen.frag"])
        , m_fscreenUniloc(m_fscreen.get())
        , m_depthmap(g_loader["depth.vert"], g_loader["depth.frag"])
        , m_depthmapUniloc(m_depthmap.get())
        , m_overlay(g_loader["overlay.vert"], g_loader["overlay.frag"])
        , m_overlayUniloc(m_overlay.get())
        , m_waterry(g_loader["water.vert"], g_loader["water.frag"])
        , m_waterryUniloc(m_waterry.get())
        , m_animate(g_loader["animated.vert"], g_loader["animated.frag"])
        , m_animateUniloc(m_animate.get())
        , m_depthAnime(g_loader["depthanime.vert"], g_loader["depthanime.frag"])
        , m_depthAnimeUniloc(m_depthAnime.get())
        , m_skybox(g_loader["skybox.vert"], g_loader["skybox.frag"])
        , m_skyboxUniloc(m_skybox.get())

        , m_static(g_loader["r_static.vert"], g_loader["r_static.frag"])
    {
        this->m_staticUniloc.set(this->m_static.get());

        g_loader.clear();
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

    const UnilocDepthAnime& ShaderMaster::useDepthAnime(void) const {
        setFor_shadowmap();
        this->m_depthAnime.use();
        return this->m_depthAnimeUniloc;
    }

    const UnilocSkybox& ShaderMaster::useSkybox(void) const {
        setFor_skybox();
        this->m_skybox.use();
        return this->m_skyboxUniloc;
    }


    const UniRender_Static& ShaderMaster::useStatic(void) const {
        setFor_generalRender();
        this->m_static.use();
        return this->m_staticUniloc;
    }

}
