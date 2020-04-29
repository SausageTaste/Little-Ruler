#pragma once

#include <vector>
#include <string>
#include <unordered_map>


namespace dal {

    enum class ShaderType { vertex, fragment };


    class ShaderPreprocessor {

    private:
        enum class Precision { nodef, highp, mediump, lowp };
        enum class Defined { parse_fail, ignore_this, include };

    private:
        std::unordered_map<std::string, std::string> m_soures;

    public:
        std::vector<std::string> m_defines;

    public:
        std::string operator[](const std::string& key);
        void clear(void);

    private:
        std::string preprocess(std::string src);
        std::string& getOrLoadSrc(const std::string& fileName);

        // Static
        static Defined parseDefine(const std::string& text, std::vector<std::string>& results);
        static ShaderType determineShaderType(const std::string& fileName);
        static std::string makeHeader(const Precision precision, const std::vector<std::string>& defines);

    };

}
