#include "u_filesystem.h"

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"

#if defined(_WIN32)
#include <fstream>
#include <windows.h>
#elif defined(__ANDROID__)
#include <functional>
#include <dirent.h>

#include <android/asset_manager.h>
#endif


using namespace std;


// Util funcs
namespace {

    constexpr char PACKAGE_NAME_ASSET[] = "asset";
    constexpr char RESOURCE_FOLDER_NAME[] = "Resource";
    constexpr char USERDATA_FOLDER_NAME[] = "userdata";
    constexpr char LOG_FOLDER_NAME[] = "log";


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

        bool ignore = false;
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


// Resource path funcs
namespace {

    struct ResPathInfo {
        std::string m_package, m_intermPath, m_finalPath;
        bool m_isResolveMode = false;
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

}


// Windows
namespace {

#if defined(_WIN32)

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

        enum class WinDirType { error, file, folder };

        WinDirType getDirType(const char* const path) {
            const auto flags = GetFileAttributesA(path);

            if ( INVALID_FILE_ATTRIBUTES == flags ) {
                return WinDirType::error;
            }
            else {
                if ( FILE_ATTRIBUTE_DIRECTORY & flags ) {
                    return WinDirType::folder;
                }
                else {
                    return WinDirType::file;
                }
            }
        }

        std::string findResourceFolderPath(void) {
            std::vector<std::string> folders;
            std::string pattern("./");

            while ( listdir(pattern.c_str(), folders) > 0 ) {
                const auto found = [&folders](void) -> bool {
                    for ( auto& item : folders ) {
                        if ( item == RESOURCE_FOLDER_NAME ) 
                            return true;
                    }
                    return false;
                }();

                if ( found ) {
                    return pattern + RESOURCE_FOLDER_NAME + '/';
                }

                pattern.append("../");
            }
        }

        const std::string& getResFolderPath(void) {
            static auto path = findResourceFolderPath();
            return path;
        }

        std::string makeWinResPath(const ResPathInfo& resPathInfo) {
            if ( PACKAGE_NAME_ASSET == resPathInfo.m_package ) {
                return fmt::format("{}{}/{}{}", win::getResFolderPath(), PACKAGE_NAME_ASSET, resPathInfo.m_intermPath, resPathInfo.m_finalPath);
            }
            else if ( LOG_FOLDER_NAME == resPathInfo.m_package ) {
                return fmt::format("{}{}/{}{}", win::getResFolderPath(), LOG_FOLDER_NAME, resPathInfo.m_intermPath, resPathInfo.m_finalPath);
            }
            else {
                return fmt::format("{}{}/{}/{}{}", win::getResFolderPath(), USERDATA_FOLDER_NAME, resPathInfo.m_package, resPathInfo.m_intermPath, resPathInfo.m_finalPath);
            }
        }

    }

#elif defined(__ANDROID__)

    namespace android {

        struct DirNode {
            std::string m_name;
            std::vector<DirNode> m_subfolders;
        };

        DirNode g_assetFolders = {
                PACKAGE_NAME_ASSET, {
                        { "font", {}},
                        { "glsl", {}},
                        { "license", {}},
                        { "map", {}},
                        { "model", {}},
                        { "texture", {
                                {"rustediron1-alt2", {}},
                                {"water", {}},
                                {"skybox", {}}
                        }
                        },
                        { "script", {} }
                }
        };

        size_t listfile_asset(std::string path, std::vector<std::string>& dirs) {
            const auto assetMgr = dal::ExternalFuncGod::getinst().getAssetMgr();

            dirs.clear();
            if ( !path.empty() && path.back() == '/' ) {
                path.pop_back();
            }

            AAssetDir* assetDir = AAssetManager_openDir(assetMgr, path.c_str());
            while ( true ) {
                auto filename = AAssetDir_getNextFileName(assetDir);
                if ( filename == nullptr ) {
                    break;
                }
                dirs.emplace_back(filename);
            }
            AAssetDir_close(assetDir);

            return dirs.size();
        }

        bool isfile_asset(const char* const path) {
            const auto assetMgr = dal::ExternalFuncGod::getinst().getAssetMgr();

            auto opend = AAssetManager_open(assetMgr, path, AASSET_MODE_UNKNOWN);
            if (nullptr != opend) {
                AAsset_close(opend);
                return true;
            }
            else {
                return false;
            }
        }

        size_t listdir_lambda(const char* const path, std::vector<std::string>& dirs, std::function<bool(dirent*)> checkFunc) {
            dirs.clear();
            const auto dir = opendir(path);
            if ( nullptr != dir ) {

                while ( true ) {
                    const auto drnt = readdir(dir);
                    if ( nullptr == drnt ) {
                        break;
                    }

                    if ( "."s == drnt->d_name ) continue;
                    if ( ".."s == drnt->d_name ) continue;

                    if ( checkFunc(drnt) ){
                        dirs.emplace_back(drnt->d_name);
                    }
                }

            }

            return dirs.size();
        }

        size_t listfolder(const char* const path, std::vector<std::string>& dirs) {
            return listdir_lambda(path, dirs, [](dirent* drnt) -> bool {
                return DT_DIR == drnt->d_type;
            });
        }

        size_t listfile(const char* const path, std::vector<std::string>& dirs) {
            return listdir_lambda(path, dirs, [](dirent* drnt) -> bool {
                return DT_REG == drnt->d_type;
            });
        }

        std::string makeAndroidStoragePath(const ResPathInfo& resPathInfo) {
            const auto& storagePath = dal::ExternalFuncGod::getinst().getAndroidStoragePath();
            return fmt::format( "{}{}/{}/{}/{}", storagePath, USERDATA_FOLDER_NAME,
                                resPathInfo.m_package, resPathInfo.m_intermPath,
                                resPathInfo.m_finalPath );
        }

    }

#endif

}


namespace dal {

    std::vector<std::string> listdir(const char* const resPath) {
        std::vector<std::string> result;

        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto winPath = win::makeWinResPath(resPathInfo);
        win::listdir(winPath, result);
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == resPathInfo.m_package ) {
            dalWarn("Not implemented: listdir for assets on Android.");
        }
        else {
            const auto filePath = android::makeAndroidStoragePath(resPathInfo);
            android::listdir_lambda(filePath.c_str(), result, [](auto drnt) { return true; });
        }
#endif

        return result;
    }

    std::vector<std::string> listfile(const char* const resPath) {
        std::vector<std::string> result;

        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto winPath = win::makeWinResPath(resPathInfo);

        std::vector<std::string> dirlist;
        win::listdir(winPath, dirlist);
        result.reserve(dirlist.size());

        for ( auto& x : dirlist ) {
            const auto dirPath = fmt::format("{}/{}", winPath, x);
            if ( win::WinDirType::file == win::getDirType(dirPath.c_str()) ) {
                result.emplace_back(x);
            }
        }
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == resPathInfo.m_package ) {
            const auto assetPath = resPathInfo.m_intermPath + resPathInfo.m_finalPath;
            android::listfile_asset(assetPath, result);
        }
        else {
            const auto filePath = android::makeAndroidStoragePath(resPathInfo);
            android::listfile(filePath.c_str(), result);
        }
#endif

        return result;
    }

    std::vector<std::string> listfolder(const char* const resPath) {
        std::vector<std::string> result, dirlist;

        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto winPath = win::makeWinResPath(resPathInfo);

        win::listdir(winPath, dirlist);
        result.reserve(dirlist.size());

        for ( auto& x : dirlist ) {
            const auto dirPath = fmt::format("{}/{}", winPath, x);
            if ( win::WinDirType::folder == win::getDirType(dirPath.c_str()) ) {
                result.emplace_back(x);
            }
        }
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == resPathInfo.m_package ) {
            dalWarn("Not implemented: listfolder for assets on Android.");
        }
        else {
            const auto filePath = android::makeAndroidStoragePath(resPathInfo);
            android::listfolder(filePath.c_str(), result);
        }
#endif

        return result;
    }

    bool isdir(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto absPath = win::makeWinResPath(resPathInfo);
        return win::getDirType(absPath.c_str()) != win::WinDirType::error;
#elif defined(__ANDROID__)
        return false;
#endif

    }

    bool isfile(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto absPath = win::makeWinResPath(resPathInfo);
        return win::getDirType(absPath.c_str()) == win::WinDirType::file;
#elif defined(__ANDROID__)
        return false;
#endif

    }

    bool isfolder(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto absPath = win::makeWinResPath(resPathInfo);
        return win::getDirType(absPath.c_str()) == win::WinDirType::folder;
#elif defined(__ANDROID__)
        return false;
#endif

    }

}
