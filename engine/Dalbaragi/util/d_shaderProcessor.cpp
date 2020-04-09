#include "d_shaderProcessor.h"

#include <array>
#include <unordered_map>
#include <memory>

#include <fmt/format.h>

#include "d_logger.h"
#include "u_fileutils.h"


using namespace fmt::literals;


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

}


// ShaderLoader
namespace dal {

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

    std::string ShaderPreprocessor::operator[](const std::string& key) {
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

    void ShaderPreprocessor::clear(void) {
        this->m_soures.clear();
        this->m_soures.reserve(0);
    }

    // Private

    /*
    const char* ShaderPreprocessor::loadNewShader(const std::string& fileName) {
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

    std::string ShaderPreprocessor::preprocess(std::string src) {
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
    const std::string& ShaderPreprocessor::orderShaderSrc(const std::string& fileName) {
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

    std::string& ShaderPreprocessor::getOrLoadSrc(const std::string& fileName) {
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

    ShaderPreprocessor::Defined ShaderPreprocessor::parseDefine(const std::string& text, std::vector<std::string>& results) {
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
            std::vector<std::string> ignoreList{
                "#ifdef GL_ES", "#endif", "#else", "#ifndef GL_ES", "#ifndef GL_ES", "#ifdef DAL_NORMAL_MAPPING"
            };

            for ( const auto& toIgnore : ignoreList ) {
                const auto head = text.find(toIgnore);
                if ( std::string::npos != head )
                    return Defined::ignore_this;
            }
        }

        dalError("Unknown define in shader: {}"_format(text));
        return Defined::parse_fail;
    }

    ShaderType ShaderPreprocessor::determineShaderType(const std::string& fileName) {
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

    std::string ShaderPreprocessor::makeHeader(const Precision precision) {
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

        fileHeader += "\n#define DAL_NORMAL_MAPPING\n\n";

        return fileHeader;
    }

}
