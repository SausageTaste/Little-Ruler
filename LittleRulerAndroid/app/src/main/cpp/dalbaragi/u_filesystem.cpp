#include "u_filesystem.h"

#include <fstream>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "s_configs.h"

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>  // mkdir
#elif defined(__ANDROID__)
#include <dirent.h>
#include <sys/stat.h>

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

    bool isdir_stat(const char* const rawpath) {
        struct stat st;
        stat(rawpath, &st);
        return static_cast<bool>(st.st_mode & S_IFDIR);
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

        std::string makeWinResPath(const dal::ResPathInfo& resPathInfo) {
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

        class DirNode {

        public:
            std::string m_name;
            std::vector<DirNode> m_subfolders;

        public:
            const DirNode* findChild(const std::string& name) const {
                for ( auto& child : this->m_subfolders ) {
                    if ( name == child.m_name ){
                        return &child;
                    }
                }

                return nullptr;
            }

            const DirNode* findByPath(const std::string& assetPath) const {
                if ( assetPath.empty() ) {
                    return this;
                }

                const auto pathDivided = split(assetPath, '/');
                const DirNode* node = this;

                for ( const auto& part : pathDivided ) {
                    node = node->findChild(part.c_str());
                    if ( nullptr == node ) {
                        return nullptr;
                    }
                }

                return node;
            }

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

        std::string makeAndroidStoragePath(const dal::ResPathInfo& resPathInfo) {
            const auto& storagePath = dal::ExternalFuncGod::getinst().getAndroidStoragePath();

            if ( LOG_FOLDER_NAME == resPathInfo.m_package ) {
                return fmt::format( "{}{}/{}/{}", storagePath, resPathInfo.m_package,
                        resPathInfo.m_intermPath, resPathInfo.m_finalPath );
            }
            else {
                return fmt::format( "{}{}/{}/{}/{}", storagePath, USERDATA_FOLDER_NAME,
                                    resPathInfo.m_package, resPathInfo.m_intermPath,
                                    resPathInfo.m_finalPath );
            }
        }

        std::string makeAssetPath(const dal::ResPathInfo& resPathInfo) {
            return resPathInfo.m_intermPath + resPathInfo.m_finalPath;;
        }

        bool isfile(const char* const path) {
            struct stat path_stat;
            stat( path, &path_stat );
            return S_ISREG( path_stat.st_mode );
        }

        bool isfolder(const char* const path) {
            return isdir_stat(path);
        }

    }

#endif

}


// Path
namespace dal {

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

    std::optional<ResPathInfo> resolvePath(const std::string& package, const std::string& dir, const std::string& fname) {
        // dir must start with / or empty.

        const auto folPath = package + dir;

        for ( const auto& child : dal::listfile(folPath.c_str()) ) {
            if ( fname == child ) {
                ResPathInfo result{ package, dir, fname, false };
                if ( !result.m_intermPath.empty() ) {
                    result.m_intermPath = result.m_intermPath.substr(1);
                    result.m_intermPath.push_back('/');
                }
                return result;
            }
        }

        for ( const auto& child : dal::listfolder(folPath.c_str()) ) {
            auto result = resolvePath(package, dir + '/' + child, fname);
            if ( result ) {
                return result;
            }
        }

        return std::nullopt;
    }

    std::string findExtension(const std::string& path) {
        const auto found = path.rfind('.');
        if ( std::string::npos != found ) {
            return path.substr(found + 1);
        }
        else {
            return "";
        }
    }

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
            const auto assetPath = resPathInfo.m_intermPath + resPathInfo.m_finalPath;

            // Files
            android::listfile_asset(assetPath, result);

            // Folders
            const auto dirNode = android::g_assetFolders.findByPath(assetPath.c_str());
            if ( nullptr != dirNode ) {
                for ( const auto& child : dirNode->m_subfolders ) {
                    result.push_back(child.m_name);
                }
            }
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
            const auto assetPath = resPathInfo.m_intermPath + resPathInfo.m_finalPath;
            const auto dirNode = android::g_assetFolders.findByPath(assetPath.c_str());
            if ( nullptr != dirNode ) {
                for ( const auto& child : dirNode->m_subfolders ) {
                    result.push_back(child.m_name);
                }
            }
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
        return isfolder(resPath) || isfile(resPath);
#endif

    }

    bool isfile(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto absPath = win::makeWinResPath(resPathInfo);
        return win::getDirType(absPath.c_str()) == win::WinDirType::file;
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == resPathInfo.m_package ) {
            const auto assetPath = resPathInfo.m_intermPath + resPathInfo.m_finalPath;
            return android::isfile_asset(assetPath.c_str());
        }
        else {
            const auto strgPath = android::makeAndroidStoragePath(resPathInfo);
            return android::isfile(strgPath.c_str());
        }
#endif

    }

    bool isfolder(const char* const resPath) {
        const auto resPathInfo = parseResPath(resPath);

#if defined(_WIN32)
        const auto absPath = win::makeWinResPath(resPathInfo);
        return win::getDirType(absPath.c_str()) == win::WinDirType::folder;
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == resPathInfo.m_package ) {
            const auto assetPath = resPathInfo.m_intermPath + resPathInfo.m_finalPath;
            return nullptr != android::g_assetFolders.findByPath(assetPath);
        }
        else {
            const auto strgPath = android::makeAndroidStoragePath(resPathInfo);
            return android::isfolder(strgPath.c_str());
        }
#endif

    }

}


// FileStream
namespace {

    class STDFileStream : public dal::IFileStream {

    private:
        std::fstream m_file;

    public:
        virtual ~STDFileStream(void) override {
            this->close();
        }

        virtual bool open(const char* const path, const dal::FileMode2 mode) override {
            this->close();
            this->m_file.open(path, this->convertOpenMode(mode));
            return this->isOpen();
        }

        virtual void close(void) override {
            if ( this->isOpen() ) {
                this->m_file.close();
            }
        }

        virtual size_t read(uint8_t* const buf, const size_t bufSize) override {
            const auto remaining = this->getSize() - this->tell();
            const auto sizeToRead = bufSize < remaining ? bufSize : remaining;
            if ( sizeToRead <= 0 ) {
                return 0;
            }

            this->m_file.read(reinterpret_cast<char*>(buf), sizeToRead);
            const auto readSize = this->m_file.gcount();
            dalAssert(readSize >= 0);

            if ( !this->m_file ) {
                if ( readSize < sizeToRead ) {
                    dalError(fmt::format("File not read completely. {} out of {} has been read. Err flags are \"{}\".",
                        readSize, sizeToRead, this->makeErrStr(this->m_file.rdstate())));
                }
                else {
                    dalError(fmt::format("File not read completely. Err flags are \"{}\".", this->makeErrStr(this->m_file.rdstate())));
                }
            }

            return static_cast<size_t>(readSize);
        }

        virtual bool readText(std::string& buffer) override {
            const auto fileSize = this->getSize();
            buffer.reserve(fileSize);
            this->m_file.seekg(0, std::ios_base::beg);

            buffer.assign((std::istreambuf_iterator<char>(this->m_file)), (std::istreambuf_iterator<char>()));

            return true;
        }

        virtual bool write(const uint8_t* const buf, const size_t bufSize) override {
            this->m_file.write(reinterpret_cast<const char*>(buf), bufSize);
            return true;
        }

        virtual bool write(const char* const str) override {
            this->m_file.write(str, std::strlen(str));
            return true;
        }

        virtual bool write(const std::string& str) override {
            this->m_file.write(str.c_str(), str.size());
            return true;
        }

        virtual size_t getSize(void) override {
            const auto lastPos = this->m_file.tellg();
            this->m_file.seekg(0, std::ios::end);
            const auto fileSize = static_cast<size_t>(this->m_file.tellg());
            this->m_file.seekg(lastPos, std::ios::beg);
            return fileSize;
        }

        virtual bool isOpen(void) override {
            return this->m_file.is_open();
        }

        virtual bool seek(const size_t offset, const dal::Whence2 whence = dal::Whence2::beg) override {
            switch ( whence ) {

            case dal::Whence2::beg:
                this->m_file.seekg(offset, std::ios_base::beg);
                break;
            case dal::Whence2::cur:
                this->m_file.seekg(offset, std::ios_base::cur);
                break;
            case dal::Whence2::end:
                this->m_file.seekg(offset, std::ios_base::end);
                break;

            }

            return !this->m_file.fail();
        }

        virtual size_t tell(void) override {
            return static_cast<size_t>(this->m_file.tellg());
        }

    private:
        static std::ios_base::openmode convertOpenMode(const dal::FileMode2 mode) {
            switch ( mode ) {

            case dal::FileMode2::read:
                return std::ios::in;
            case dal::FileMode2::write:
                return std::ios::out;
            case dal::FileMode2::append:
                return (std::ios::out | std::ios::app);
            case dal::FileMode2::bread:
                return (std::ios::in | std::ios::binary);
            case dal::FileMode2::bwrite:
                return (std::ios::out | std::ios::binary);
            case dal::FileMode2::bappend:
                return (std::ios::out | std::ios::app | std::ios::binary);

            }
        }

        static std::string makeErrStr(const unsigned int errFlag) {
            std::string buffer;
            buffer.reserve(15);

            if ( 0 != (errFlag & std::fstream::eofbit) )
                buffer += "eof, ";
            if ( 0 != (errFlag & std::fstream::failbit) )
                buffer += "fail, ";
            if ( 0 != (errFlag & std::fstream::badbit) )
                buffer += "bad, ";

            if ( !buffer.empty() ) {
                buffer.resize(buffer.size() - 2);
            }

            return buffer;
        }

    };


#ifdef __ANDROID__

    class AssetSteam : public dal::IFileStream {

    private:
        AAsset* m_asset = nullptr;
        size_t m_fileSize = 0;

    public:
        virtual ~AssetSteam(void) override {
            this->close();
        }

        virtual bool open(const char* const path, const dal::FileMode2 mode) override {
            this->close();

            switch ( mode ) {
            case dal::FileMode2::read:
            case dal::FileMode2::bread:
                break;
            case dal::FileMode2::write:
            case dal::FileMode2::append:
            case dal::FileMode2::bwrite:
            case dal::FileMode2::bappend:
                dalError("Cannot open Asset as write mode: "s + path);
                return false;
            }

            const auto assetMgr = dal::ExternalFuncGod::getinst().getAssetMgr();
            this->m_asset = AAssetManager_open(assetMgr, path, AASSET_MODE_UNKNOWN);
            if ( nullptr == this->m_asset ) {
                return false;
            }

            this->m_fileSize = static_cast<size_t>(AAsset_getLength64(this->m_asset));
            if ( this->m_fileSize <= 0 ) {
                dalWarn("File contents' length is 0 for: "s + path);
            }

            return true;
        }

        virtual void close(void) override {
            if ( this->isOpen() ) AAsset_close(this->m_asset);
            this->m_asset = nullptr;
            this->m_fileSize = 0;
        }

        virtual size_t read(uint8_t* const buf, const size_t bufSize) override {
            // Android asset manager implicitly read beyond file range WTF!!!
            const auto remaining = this->m_fileSize - this->tell();
            auto sizeToRead = bufSize < remaining ? bufSize : remaining;
            if ( sizeToRead <= 0 ) return 0;

            const auto readBytes = AAsset_read(this->m_asset, buf, sizeToRead);
            if ( readBytes < 0 ) {
                dalError("Failed to read asset.");
                return 0;
            }
            else if ( 0 == readBytes ) {
                dalError("Tried to read after end of asset.");
                return 0;
            }
            else {
                assert(readBytes == sizeToRead);
                return static_cast<size_t>(readBytes);
            }
        }

        virtual bool readText(std::string& buffer) override {
            const auto fileSize = this->getSize();
            auto buf = std::unique_ptr<uint8_t[]>{ new uint8_t[fileSize + 1] };
            const auto readSize = this->read(buf.get(), fileSize);
            buf[readSize] = '\0';
            buffer = reinterpret_cast<const char*>(buf.get());

            return true;
        }

        virtual bool write(const uint8_t* const buf, const size_t bufSize) override {
            dalAbort("Writing is illegal on assets.");
        }

        virtual bool write(const char* const str) override {
            dalAbort("Writing is illegal on assets.");
        }

        virtual bool write(const std::string& str) override {
            dalAbort("Writing is illegal on assets.");
        }

        virtual size_t getSize(void) override {
            return this->m_fileSize;
        }

        virtual bool isOpen(void) override {
            return nullptr != this->m_asset;
        }

        virtual bool seek(const size_t offset, const dal::Whence2 whence = dal::Whence2::beg) override {
            decltype(SEEK_SET) cwhence;

            switch ( whence ) {
            case dal::Whence2::beg:
                cwhence = SEEK_SET;
                break;
            case dal::Whence2::cur:
                cwhence = SEEK_CUR;
                break;
            case dal::Whence2::end:
                cwhence = SEEK_END;
                break;
            }

            return AAsset_seek(this->m_asset, static_cast<off_t>(offset), cwhence) != -1;
        }

        virtual size_t tell(void) override {
            const auto curPos = AAsset_getRemainingLength(this->m_asset);
            return this->m_fileSize - static_cast<size_t>(curPos);
        }

    };

#endif

}


namespace {

    void assertDir(const char* const path) {
        if ( isdir_stat(path) ) {
            return;
        }

#if defined(_WIN32)
        const auto res = _mkdir(path);
#elif defined(__ANDROID__)
        const auto res = mkdir(path, 0);
#endif
        if ( 0 != res ) {
            switch ( errno ) {

            case EEXIST:
                dalWarn("Checked isdir but dir already exists upon _mkdir for userdata.");
                break;
            case ENOENT:
                dalAbort(fmt::format("Invalid path name in assertDir_userdata: {}", path));
            case EROFS:
                dalAbort(fmt::format("Parent folder is read only: {}", path));
            default:
                dalAbort(fmt::format("Unknown errno for _mkdir in assertDir_userdata: {}", errno));

            }
        }
        else {
            dalInfo(fmt::format("Folder created: {}", path));
        }
    }

    void assertDir_userdata(void) {

#if defined(_WIN32)
        const auto path = win::getResFolderPath() + USERDATA_FOLDER_NAME;
#elif defined(__ANDROID__)
        const auto path = dal::ExternalFuncGod::getinst().getAndroidStoragePath() + USERDATA_FOLDER_NAME;
#endif
        assertDir(path.c_str());

    }

    void assertDir_log(void) {

#if defined(_WIN32)
        const auto path = win::getResFolderPath() + LOG_FOLDER_NAME;
#elif defined(__ANDROID__)
        const auto path = dal::ExternalFuncGod::getinst().getAndroidStoragePath() + LOG_FOLDER_NAME;
#endif
        assertDir(path.c_str());

    }


    template <typename _StreamTyp, std::string(_PathFunc)(const dal::ResPathInfo&)>
    std::unique_ptr<dal::IFileStream> fileopen_general(const dal::ResPathInfo& pathinfo, const dal::FileMode2 mode) {
        const std::string filePath = _PathFunc(pathinfo);
        std::unique_ptr<dal::IFileStream> file{ new _StreamTyp };

        if ( file->open(filePath.c_str(), mode) ) {
            return file;
        }

        const auto isWriteMode = (dal::FileMode2::write == mode) || (dal::FileMode2::bwrite == mode);

        if ( pathinfo.m_isResolveMode && !isWriteMode ) {
            const auto newPathInfo = dal::resolvePath(pathinfo.m_package, "", pathinfo.m_finalPath);
            if ( !newPathInfo ) {
                return { nullptr };
            }
            const auto nweFilePath = _PathFunc(*newPathInfo);

            if ( file->open(nweFilePath.c_str(), mode) ) {
                return file;
            }
            else {
                return { nullptr };
            }
        }
        else {
            return { nullptr };
        }
    }

}


namespace dal {

    std::unique_ptr<IFileStream> fileopen(const char* const resPath, const FileMode2 mode) {
        const auto pathinfo = parseResPath(resPath);

        if ( pathinfo.m_finalPath.empty() ) {
            return { nullptr };
        }
        else if ( pathinfo.m_package.empty() ) {
            return { nullptr };
        }
      
#if defined(_WIN32)
        return fileopen_general<STDFileStream, win::makeWinResPath>(pathinfo, mode);
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == pathinfo.m_package ) {
            return fileopen_general<AssetSteam, android::makeAssetPath>(pathinfo, mode);
        }
        else {
            return fileopen_general<STDFileStream, android::makeAndroidStoragePath>(pathinfo, mode);
        }
#endif

    }

}
