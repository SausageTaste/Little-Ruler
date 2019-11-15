#include "u_filesystem.h"

#include <fmt/format.h>

#include "s_logger_god.h"

#if defined(_WIN32)
#include <fstream>
#include <windows.h>
#elif defined(__ANDROID__)

#endif


using namespace std;


// Util funcs
namespace {

    const char* const PACKAGE_NAME_ASSET = "asset";
    const char* const RESOURCE_FOLDER_NAME = "Resource";
    const char* const USERDATA_FOLDER_NAME = "userdata";
    const char* const LOG_FOLDER_NAME = "log";


    template <typename _DelimTyp>
    std::vector<std::string> split(const std::string& str, const _DelimTyp& delim) {
        std::vector<std::string> cont;
        std::size_t current, previous = 0;
        current = str.find(delim);
        while ( current != std::string::npos ) {
            cont.push_back(str.substr(previous, current - previous));
            previous = current + 1;
            current = str.find(delim, previous);
        }
        cont.push_back(str.substr(previous, current - previous));

        return cont;
    }

    std::string removeDuplicates(const std::string& str, const char criteria) {
        std::string result;
        result.reserve(str.size());

        size_t ignore = false;
        for ( const auto c : str ) {
            if ( c == criteria ) {
                if ( !ignore ) {
                    result.push_back(c);
                    ignore = true;
                }
            }
            else {
                result.push_back(c);
                ignore = false;
            }
        }

        return result;
    }

    std::string lstrip(const std::string& str, const char criteria) {
        size_t count = 0;
        for ( const auto c : str ) {
            if ( criteria == c ) {
                ++count;
            }
            else {
                break;
            }
        }

        return str.substr(count);
    }

}


// Windows
namespace {

#ifdef _WIN32
    namespace win {

        size_t listdir(std::string pattern, std::vector<std::string>& con) {
            con.clear();

            if ( pattern.back() != '/' ) pattern.push_back('/');
            pattern.push_back('*');

            WIN32_FIND_DATA data;
            HANDLE hFind = FindFirstFile(pattern.c_str(), &data);
            if ( INVALID_HANDLE_VALUE != hFind ) {
                do {
                    const auto fileName = std::string{ data.cFileName };
                    if ( fileName == "."s ) continue;
                    if ( ".."s == fileName ) continue;

                    con.push_back(data.cFileName);
                    if ( con.back() == "System Volume Information"s ) {
                        con.clear();
                        return 0;
                    }
                } while ( FindNextFile(hFind, &data) != 0 );
                FindClose(hFind);
            }

            return con.size();
        }

        bool isdir(const char* const path) {
            const auto what = GetFileAttributesA(path);

            if ( what == INVALID_FILE_ATTRIBUTES ) {
                return false;
            }
            else {
                return true;
            }
        }

        bool isfolder(const char* const path) {
            const auto what = GetFileAttributesA(path);

            if ( what == INVALID_FILE_ATTRIBUTES ) {
                return false;
            }
            else if ( FILE_ATTRIBUTE_DIRECTORY & what ) {
                return true;
            }
            else {
                return false;
            }
        }

        bool isfile(const char* const path) {
            const auto what = GetFileAttributesA(path);

            if ( what == INVALID_FILE_ATTRIBUTES ) {
                return false;
            }
            else if ( FILE_ATTRIBUTE_DIRECTORY & what ) {
                return false;
            }
            else {
                return true;
            }
        }

        const std::string& getResourceDir_win(void) {
            static std::string path;

            if ( path == "" ) {
                std::vector<std::string> folders;
                std::string pattern("./");

                while ( listdir(pattern.c_str(), folders) > 0 ) {
                    const auto found = [&folders](void) -> bool {
                        for ( auto& item : folders ) {
                            if ( item == RESOURCE_FOLDER_NAME ) return true;
                        }
                        return false;
                    }();

                    if ( found ) {
                        path = pattern + RESOURCE_FOLDER_NAME + '/';
                        break;
                    }

                    pattern.append("../");
                }
            }

            return path;
        }

    }
#endif

}


// Resource path funcs
namespace {

    struct ResPathInfo {
        std::string m_package, m_intermPath, m_finalPath;
        bool m_isResolveMode = false;

        void print(void) const {
            fmt::print("{} - {} - {} ({})\n", this->m_package, this->m_intermPath, this->m_finalPath, this->m_isResolveMode);
        }
    };

    ResPathInfo parseResPath(const std::string& resPath) {
        ResPathInfo result;

        constexpr auto restDivider = [](const std::string& str, const size_t restPos, std::string& intermResult, std::string& finalResult) {
            const auto rightMostDivider = str.rfind('/');
            if ( (rightMostDivider > restPos) && (std::string::npos != rightMostDivider) ) {
                intermResult = str.substr(restPos, rightMostDivider - restPos) + '/';
                intermResult = removeDuplicates(intermResult, '/');
                intermResult = lstrip(intermResult, '/');
                finalResult = str.substr(rightMostDivider + 1);
            }
            else {
                intermResult.clear();
                finalResult = str.substr(restPos);
            }
        };

        const auto colonPos = resPath.find("::");
        if ( std::string::npos != colonPos ) {
            result.m_isResolveMode = true;
            result.m_package = resPath.substr(0, colonPos);
            restDivider(resPath, colonPos + 2, result.m_intermPath, result.m_finalPath);
        }
        else {
            result.m_isResolveMode = false;
            const auto packageDivider = resPath.find('/');
            if ( std::string::npos != packageDivider ) {
                result.m_package = resPath.substr(0, packageDivider);
                restDivider(resPath, packageDivider + 1, result.m_intermPath, result.m_finalPath);
            }
            else {
                result.m_package = resPath;
            }
        }

        return result;
    }

    std::string makeAbsolutePath(const ResPathInfo& path) {
        return fmt::format("{}{}/{}{}", win::getResourceDir_win(), path.m_package, path.m_intermPath, path.m_finalPath);
    }

}


namespace dal {

    std::vector<std::string> listdir(const char* const resPath) {
        std::vector<std::string> result;

        const auto resPathInfo = parseResPath(resPath);
        const auto absPath = makeAbsolutePath(resPathInfo);

        win::listdir(absPath, result);

        return result;
    }

    std::vector<std::string> listfile(const char* const resPath) {
        std::vector<std::string> result, dirlist;

        const auto resPathInfo = parseResPath(resPath);
        const auto absPath = makeAbsolutePath(resPathInfo);

        win::listdir(absPath, dirlist);
        result.reserve(dirlist.size());

        for ( auto& x : dirlist ) {
            const auto dirPath = absPath + '/' + x;
            if ( win::isfile(dirPath.c_str()) ) {
                result.emplace_back(x);
            }
        }

        return result;
    }

    std::vector<std::string> listfolder(const char* const resPath) {
        std::vector<std::string> result, dirlist;

        const auto resPathInfo = parseResPath(resPath);
        const auto absPath = makeAbsolutePath(resPathInfo);

        win::listdir(absPath, dirlist);
        result.reserve(dirlist.size());

        for ( auto& x : dirlist ) {
            const auto dirPath = absPath + '/' + x;
            if ( win::isfolder(dirPath.c_str()) ) {
                result.emplace_back(x);
            }
        }

        return result;
    }

    bool isdir(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);
        const auto absPath = makeAbsolutePath(resPathInfo);
        return win::isdir(absPath.c_str());
    }

    bool isfile(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);
        const auto absPath = makeAbsolutePath(resPathInfo);
        return win::isfile(absPath.c_str());
    }

    bool isfolder(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);
        const auto absPath = makeAbsolutePath(resPathInfo);
        return win::isfolder(absPath.c_str());
    }

}