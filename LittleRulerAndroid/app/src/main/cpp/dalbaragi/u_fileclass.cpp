#ifdef _WIN32
#pragma warning(disable:4996)
// To disable fopen deprecated error which is caused by tga.h
#endif

#include "u_fileclass.h"

#include <fstream>
#include <cassert>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>

#include <tga.h>
#include <lodepng.h>
#include <fmt/format.h>

#include "s_logger_god.h"

#if defined(_WIN32)
#include <fstream>
#include <windows.h>
#include <direct.h>  // mkdir
#elif defined(__ANDROID__)
#include <android/asset_manager.h>
#include <zconf.h> // Just for SEEK_SET
#include <dirent.h>
#endif


#define DAL_PRINT_RESOLVING 0


using namespace std::string_literals;
using namespace fmt::literals;


// Translation unit level globals
namespace {

    const char* const PACKAGE_NAME_ASSET = "asset";
    const char* const RESOURCE_FOLDER_NAME = "Resource";
    const char* const USERDATA_FOLDER_NAME = "userdata";
    const char* const LOG_FOLDER_NAME = "log";

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

}


// Windows
namespace {

#ifdef _WIN32

    size_t getListFolFile_win(std::string pattern, std::vector<std::string>& con) {
        con.clear();

        if ( pattern.back() != '/' ) pattern.push_back('/');
        pattern.push_back('*');

        WIN32_FIND_DATA data;
        HANDLE hFind;
        if ( (hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE ) {
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

    const std::string& getResourceDir_win(void) {
        static std::string path;

        if ( path == "" ) {
            std::vector<std::string> folders;
            std::string pattern("./");

            while ( getListFolFile_win(pattern.c_str(), folders) > 0 ) {
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

    bool isFile_win(const std::string path) {
        std::unique_ptr<FILE, decltype(fclose)*> file{ fopen(path.c_str(), "r"), fclose };
        return nullptr != file;
    }

    bool findRecur_win(std::string& result, std::string path, const std::string& criteria) {
        if ( !path.empty() && path.back() != '/' ) path.push_back('/');

        std::vector<std::string> dirs;
        getListFolFile_win(path, dirs);
        for ( const auto& one : dirs ) {
            auto newPath = path + one;
            if ( isFile_win(newPath) ) {
                if ( one == criteria ) {
                    result = path + one;
                    return true;
                }
            }
            else {
                if ( findRecur_win(result, newPath, criteria) ) {
                    return true;
                }
            }
        }

        return false;
    };

#endif

}


// Android
namespace {

#ifdef __ANDROID__

    AAssetManager* gAssetMgr = nullptr;

    std::string g_storagePath;

    size_t getFileList_asset(std::string path, std::vector<std::string>& dirs) {
        dirs.clear();
        if ( !path.empty() && path.back() == '/' ) path.pop_back();

        AAssetDir* assetDir = AAssetManager_openDir(gAssetMgr, path.c_str());
        while ( true ) {
            auto filename = AAssetDir_getNextFileName(assetDir);
            if ( filename == nullptr ) break;
            dirs.emplace_back(filename);
        }
        AAssetDir_close(assetDir);

        return dirs.size();
    }

    /* unused function
    bool isAssetFile(const char* const path) {
        auto opend = AAssetManager_open(gAssetMgr, path, AASSET_MODE_UNKNOWN);
        if (nullptr == opend) {
            return false;
        }
        else {
            AAsset_close(opend);
            return true;
        }
    }
     */

     // Returns only optional directory.
    bool findMatchingAsset(std::string& result, const DirNode& node, const std::string& accumPath, const std::string& criteria) {
        for ( auto& folNode : node.m_subfolders ) {
            std::string newPath;
            if ( accumPath.empty() ) {
                newPath = folNode.m_name;
            }
            else {
                newPath = accumPath + '/' + folNode.m_name;
            }

            std::vector<std::string> dirs;
            getFileList_asset(newPath, dirs);
            for ( auto& fileName : dirs ) {
#if DAL_PRINT_RESOLVING != 0
				dalVerbose(newPath + '/' + fileName);
#endif
                if ( criteria == fileName ) {
                    result = newPath;
                    return true;
                }
            }

            if ( findMatchingAsset(result, folNode, newPath, criteria) ) {
				return true;
            }
        }

        return false;
    }

    // Rrturns full path of a folder that contains the file.
    bool findMatchingFileName_recursive_internalStorage(std::string& result, const std::string& criteria, const std::string& rootDir) {
        const auto dir = opendir(rootDir.c_str());
        if ( nullptr == dir ) {
            dalWarn("Failed to open directory: "s + rootDir);
            return false;
        }

        while ( true ) {
            const auto drnt = readdir(dir);
            if ( nullptr == drnt ) break;

            if ( "."s == drnt->d_name ) continue;
            if ( ".."s == drnt->d_name ) continue;

            unsigned char dirType = drnt->d_type;
            if ( dirType == DT_DIR ) {
                std::string newFolPath = rootDir + drnt->d_name + "/";
                if ( findMatchingFileName_recursive_internalStorage(result, criteria, newFolPath) )
                    return true;
            }
            else {
                if ( criteria == drnt->d_name ) {
                    result = rootDir;
                    return true;
                }
            }
        }

        return false;
    }

#endif

}


// Common
namespace {

    bool isdir(const char* const path) {
        struct stat st;
        stat(path, &st);
        return static_cast<bool>(st.st_mode & S_IFDIR);
    }

    void assertDir(const char* const path) {
        if ( isdir(path) ) return;

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
                dalAbort("Invalid path name in assertDir_userdata: {}"_format(path));
            case EROFS:
                dalAbort("Parent folder is read only: {}"_format(path));
            default:
                dalAbort("Unknown errno for _mkdir in assertDir_userdata: {}"_format(errno));
            }
        }
        else {
            dalInfo("Folder created: {}"_format(path));
        }
    }

    void assertDir_userdata(void) {

#if defined(_WIN32)
        const auto path = getResourceDir_win() + USERDATA_FOLDER_NAME;
#elif defined(__ANDROID__)
        const auto path = g_storagePath + USERDATA_FOLDER_NAME;
#endif
        assertDir(path.c_str());

    }

    void assertDir_log(void) {

#if defined(_WIN32)
        const auto path = getResourceDir_win() + LOG_FOLDER_NAME;
#elif defined(__ANDROID__)
        const auto path = g_storagePath + LOG_FOLDER_NAME;
#endif
        assertDir(path.c_str());

    }


    std::string makeFstreamErrFlagStr(unsigned int errFlag) {
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

    int makeFsteamOpenmode(const dal::FileMode mode){
        switch ( mode ) {
            case dal::FileMode::read:
                return std::ios::in;
            case dal::FileMode::write:
                return std::ios::out;
            case dal::FileMode::append:
                return (std::ios::out | std::ios::app);
            case dal::FileMode::bread:
                return (std::ios::in | std::ios::binary);
            case dal::FileMode::bwrite:
                return (std::ios::out | std::ios::binary);
            case dal::FileMode::bappend:
                return (std::ios::out | std::ios::app | std::ios::binary);
        }

        dalAbort("Unkown dal::FileMode: "_format(static_cast<unsigned int>(mode)));
    }

}


// FileStream
namespace {

    class STDFileStream : public dal::IResourceStream {

    private:
        std::fstream m_file;

    public:
        virtual ~STDFileStream(void) override {
            this->close();
        }

        virtual bool open(const char* const path, const dal::FileMode mode) override {
            this->close();

            this->m_file.open(path, makeFsteamOpenmode(mode));

            if ( this->isOpen() ) {
                return true;
            }
            else {
                dalError("Failed STDFileStream::open for: {}"_format(path));
                return false;
            }
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
                    dalError("File not read completely. {} out of {} has been read. Err flags are \"{}\"."_format(
                        readSize, sizeToRead, makeFstreamErrFlagStr(this->m_file.rdstate())
                    ));
                }
                else {
                    dalError("File not read completely. Err flags are \"{}\"."_format(
                        makeFstreamErrFlagStr(this->m_file.rdstate())
                    ));
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

        virtual bool seek(const size_t offset, const dal::Whence whence = dal::Whence::beg) override {
            switch ( whence ) {

            case dal::Whence::beg:
                this->m_file.seekg(offset, std::ios_base::beg);
                break;
            case dal::Whence::cur:
                this->m_file.seekg(offset, std::ios_base::cur);
                break;
            case dal::Whence::end:
                this->m_file.seekg(offset, std::ios_base::end);
                break;

            }

            return !this->m_file.fail();
        }

        virtual size_t tell(void) override {
            return static_cast<size_t>(this->m_file.tellg());
        }

    };


#ifdef __ANDROID__

    class AssetSteam : public dal::IResourceStream {

    private:
        AAsset* m_asset = nullptr;
        size_t m_fileSize = 0;

    public:
        virtual ~AssetSteam(void) override {
            this->close();
        }

        virtual bool open(const char* const path, const dal::FileMode mode) override {
            this->close();

            switch ( mode ) {
            case dal::FileMode::read:
            case dal::FileMode::bread:
                break;
            case dal::FileMode::write:
            case dal::FileMode::append:
            case dal::FileMode::bwrite:
            case dal::FileMode::bappend:
                dalError("Cannot open Asset as write mode: "s + path);
                return false;
            }

            this->m_asset = AAssetManager_open(gAssetMgr, path, AASSET_MODE_UNKNOWN);
            if ( nullptr == this->m_asset ) {
                dalError("Failed AssetSteam::open for: "s + path);
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

        virtual bool seek(const size_t offset, const dal::Whence whence = dal::Whence::beg) override {
            decltype(SEEK_SET) cwhence;

            switch ( whence ) {
            case dal::Whence::beg:
                cwhence = SEEK_SET;
                break;
            case dal::Whence::cur:
                cwhence = SEEK_CUR;
                break;
            case dal::Whence::end:
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


// Image reader functions
namespace {

    bool parseImagePNG(dal::binfo::ImageFileData& output, std::vector<uint8_t>& dataBuffer) {
        unsigned int w, h;
        auto error = lodepng::decode(output.m_buf, w, h, dataBuffer);
        if ( error ) {
            dalError("PNG decode error: {}"_format(lodepng_error_text(error)));
            return false;
        }

        output.m_width = static_cast<size_t>(w);
        output.m_height = static_cast<size_t>(h);
        output.m_pixSize = 4;

        // Assert that pixel size is 4.
        dalAssert(output.m_width * output.m_height * output.m_pixSize == output.m_buf.size());

        output.flipY();  // TGA does this automatically but not this.

        return true;
    }

    bool parseImageTGA(dal::binfo::ImageFileData& output, std::vector<uint8_t>& dataBuffer) {
        int w, h, p;
        std::unique_ptr<uint8_t, decltype(free)*> result{
            tga_load_memory(dataBuffer.data(), static_cast<int>(dataBuffer.size()), &w, &h, &p), free
        };

        if ( nullptr == result ) {
            dalError("Failed to parse tga file.");
            return false;
        }

        output.m_width = static_cast<size_t>(w);
        output.m_height = static_cast<size_t>(h);
        output.m_pixSize = static_cast<size_t>(p);

        const auto resArrSize = output.m_width * output.m_height * output.m_pixSize;
        output.m_buf.clear();
        output.m_buf.insert(output.m_buf.begin(), result.get(), result.get() + resArrSize);

        return true;
    }

    const std::unordered_map<std::string, decltype(parseImagePNG)*> IMAGE_PARSER_MAP = {
        { ".png"s, parseImagePNG },
        { ".tga"s, parseImageTGA }
    };

}


// Resource ID
namespace dal {

    ResourceIDString::ResourceIDString(const std::string& resourceID) {
        const auto& pathStr = resourceID;

        size_t excludePos = 0;

        // Package name
        {
            const auto packagePos = pathStr.find("::");

            if ( 0 == packagePos ) {
                this->m_package = "";
                excludePos = 2;
            }
            else if ( std::string::npos == packagePos ) {
                this->m_package = "";
            }
            else {
                this->m_package = pathStr.substr(0, packagePos);
                excludePos = packagePos + 2;
            }
        }

        {
            const auto dirPos = pathStr.rfind("/");
            if ( std::string::npos == dirPos ) {
                this->m_dir = "";
            }
            else {
                this->m_dir = pathStr.substr(excludePos, dirPos + 1 - excludePos);
                excludePos = dirPos + 1;
            }
        }

        {
            const auto extPos = pathStr.rfind(".");
            if ( std::string::npos == extPos ) {
                this->m_bareName = pathStr.substr(excludePos, pathStr.size() - excludePos);
                this->m_ext = "";
            }
            else {
                this->m_bareName = pathStr.substr(excludePos, extPos - excludePos);
                this->m_ext = pathStr.substr(extPos, pathStr.size() - extPos);
            }
        }

        if ( !this->m_dir.empty() && this->m_dir.back() != '/' ) {
            this->m_dir.push_back('/');
        }
    }

    ResourceIDString::ResourceIDString(const char* const resourceID)
        : ResourceID(std::string{ resourceID })
    {

    }

    ResourceIDString::ResourceIDString(const std::string& package, const std::string& optionalDir, const std::string& bareName, const std::string& ext)
        : m_package(package)
        , m_dir(optionalDir)
        , m_bareName(bareName)
        , m_ext(ext)
    {

    }

    const std::string& ResourceIDString::getPackage(void) const {
        return this->m_package;
    }

    const std::string& ResourceIDString::getOptionalDir(void) const {
        return this->m_dir;
    }

    const std::string& ResourceIDString::getBareName(void) const {
        return this->m_bareName;
    }

    const std::string& ResourceIDString::getExt(void) const {
        return this->m_ext;
    }

    std::string ResourceIDString::makeIDStr(void) const {
        return this->m_package + "::" + this->m_dir + this->m_bareName + this->m_ext;
    }

    std::string ResourceIDString::makeFileName(void) const {
        return this->m_bareName + this->m_ext;
    }

    std::string ResourceIDString::makeFilePath(void) const {
        return this->m_dir + this->m_bareName + this->m_ext;
    }

    std::string ResourceIDString::makeBasicForm(void) const {
        return this->m_package + this->m_bareName + this->m_ext;
    }

    void ResourceIDString::setPackage(const std::string& t) {
        this->m_package = t;
    }

    void ResourceIDString::setPackageIfEmpty(const std::string& t) {
        if ( this->m_package.empty() ) {
            this->m_package = t;
        }
    }

    void ResourceIDString::setOptionalDir(const std::string& t) {
        this->m_dir = t;
        if ( !this->m_dir.empty() && '/' != this->m_dir.back() ) {
            this->m_dir.push_back('/');
        }
    }

}


// file utils
namespace dal::futil {

    bool getRes_text(const ResourceID& resID, std::string& buffer) {
        auto file = resopen(resID, FileMode::read);
        if ( nullptr == file ) {
            return false;
        }

        return file->readText(buffer);
    }

    bool getRes_image(const ResourceID& resID, binfo::ImageFileData& data) {
        std::vector<uint8_t> fileBuffer;
        {
            auto file = resopen(resID, FileMode::bread);
            if ( nullptr == file ) {
                return false;
            }

            const auto fileSize = file->getSize();
            fileBuffer.resize(fileSize);
            if ( !file->read(fileBuffer.data(), fileBuffer.size()) ) {
                return false;
            }
        }

        decltype(parseImagePNG)* parseFunc = nullptr;
        std::string selectedFormat;
        {
            if ( fileBuffer.size() > 5 && fileBuffer[1] == 'P' && fileBuffer[2] == 'N' && fileBuffer[3] == 'G') {
                parseFunc = parseImagePNG;
                selectedFormat = ".png";
            }
            else {
                const auto found = IMAGE_PARSER_MAP.find(resID.getExt());
                if (IMAGE_PARSER_MAP.end() == found) {
                    dalError("Not supported image file format: {}"_format(resID.makeIDStr()));
                    return false;
                }
                else {
                    parseFunc = found->second;
                    selectedFormat = found->first;
                }
            }
        }

        if ( !parseFunc(data, fileBuffer) ) {
            dalError("Error while parsing image ({}) : {}"_format(selectedFormat, resID.makeIDStr()));
            return false;
        }
        else {
            return true;
        }
    }

    bool getRes_buffer(const ResourceID& resID, std::vector<uint8_t>& buffer) {
        auto file = resopen(resID, FileMode::bread);
        if ( nullptr == file ) {
            return false;
        }

        const auto fileSize = file->getSize();
        buffer.resize(fileSize);
        return file->read(buffer.data(), buffer.size()) == fileSize;
    }

}


namespace dal {

    bool resolveRes(ResourceID& result) {
        const std::string resolveSucMsg{ "Resource resolved: " };
        const std::string resolveFailMsg{ "Failed to resolve a resouce: " };

        const auto fileName = result.makeFileName();

        if ( result.getPackage().empty() ) {
            dalError("Cannot resolve \"{}\" without package defined."_format(fileName));
            return false;
        }

#if defined(_WIN32)
        std::string path;
        if ( result.getPackage() == PACKAGE_NAME_ASSET ) {
            path = getResourceDir_win() + result.getPackage() + '/';
        }
        else {
            path = getResourceDir_win() + USERDATA_FOLDER_NAME + '/' + result.getPackage() + '/';
        }

        std::string resultStr;
        if ( findRecur_win(resultStr, path, fileName) ) {
            result.setOptionalDir(resultStr.substr(path.size(), resultStr.find(fileName) - path.size()));
            return true;
        }
        else {
            dalError(resolveFailMsg + result.makeIDStr());
            return false;
        }
#elif defined(__ANDROID__)
        if ( PACKAGE_NAME_ASSET == result.getPackage() ) {
            std::string foundStr;
            if ( findMatchingAsset(foundStr, g_assetFolders, "", fileName) ) {
                result.setOptionalDir(foundStr);
                return true;
            }
            else {
                dalError("Failed to resolve an asset \'{}\'"_format(result.makeIDStr()));
                return false;
            }
        }
        else {
            const auto filePath = g_storagePath + USERDATA_FOLDER_NAME + '/' + result.getPackage() + '/';
            std::string bufferResult;
            if ( findMatchingFileName_recursive_internalStorage(bufferResult, fileName, filePath) ) {
                result.setOptionalDir(bufferResult.substr(filePath.size(), bufferResult.size() - filePath.size()));
                return true;
            }
            else {
                dalError("Failed to resolve a resource \"{}\""_format(result.makeIDStr()));
                return false;
            }
        }
#endif

    }

    bool initFilesystem(void* mgr, const char* const sdcardPath) {

#ifdef __ANDROID__
        if ( mgr == nullptr )  return false;
        gAssetMgr = reinterpret_cast<AAssetManager*>(mgr);

        if ( nullptr == sdcardPath ) return false;
        g_storagePath = sdcardPath;
        dalInfo("Storage path set: {}"_format(g_storagePath));
#endif

        return true;
    }

    bool isFilesystemReady(void) {
#ifdef __ANDROID__
        if ( gAssetMgr == nullptr ) return false;
        if ( g_storagePath.empty() ) return false;
#endif
        return true;
    }

    /*
    Input str must be one of followings
    { "wb", "w", "wt", "rb", "r", "rt", "ab", "a", "at" }
    */
    FileMode mapFileMode(const char* const str) {
        // { read, write, append, bread, bwrite, bappend };
        // The order is important!

        constexpr unsigned int NULL_CODE = 4444;

        const size_t inputLen = std::strlen(str);
        dalAssert(0 != inputLen);

        unsigned int workType = NULL_CODE;  // 0 for read, 1 for write, 2 for append.
        unsigned int byteModeFlag = NULL_CODE;  // 0 for text, 1 for byte.

        // Fill variable workType.
        {
            switch ( str[0] ) {
            case 'r':
                workType = 0; break;
            case 'w':
                workType = 1; break;
            case 'a':
                workType = 3; break;
            default:
                dalAbort("Unknown file open mode: {}"_format(str));
            }
        }

        // Fill byteModeFlag
        {
            if ( 1 == inputLen ) {
                byteModeFlag = 0;
            }
            else {
                switch ( str[1] ) {
                case 't':
                    byteModeFlag = 0; break;
                case 'b':
                    byteModeFlag = 1; break;
                default:
                    dalAbort("Unknown file open mode: {}"_format(str));
                }
            }
        }

        const auto enumIndex = workType + 3 * byteModeFlag;
        return static_cast<FileMode>(enumIndex);
    }

    std::unique_ptr<IResourceStream> resopen(ResourceID resID, const FileMode mode) {
        if ( resID.getPackage().empty() ) {
            dalError("Caanot open resource without package specified: {}"_format(resID.makeIDStr()));
            return { nullptr };
        }

        if ( FileMode::read != mode && FileMode::bread != mode )
            goto finishResolve;
        if ( !resID.getOptionalDir().empty() )
            goto finishResolve;

        if ( !resolveRes(resID) ) {
            dalError("Failed to resolve '{}' in fopen."_format(resID.makeIDStr()));
            return { nullptr };
        }

    finishResolve:

#if defined(_WIN32)
        std::string filePath;
        if ( resID.getPackage() == PACKAGE_NAME_ASSET ) {
            filePath = fmt::format("{}{}/{}", getResourceDir_win(), PACKAGE_NAME_ASSET, resID.makeFilePath());
        }
        else if ( resID.getPackage() == LOG_FOLDER_NAME ) {
            filePath = fmt::format("{}{}/{}", getResourceDir_win(), LOG_FOLDER_NAME, resID.makeFilePath());
            assertDir_log();
        }
        else {
            const auto dirToAssert = fmt::format("{}{}/{}", getResourceDir_win(), USERDATA_FOLDER_NAME, resID.getPackage());
            filePath = fmt::format("{}/{}", dirToAssert, resID.makeFilePath());
            assertDir_userdata();
            assertDir(dirToAssert.c_str());
        }

        std::unique_ptr<IResourceStream> file{ new STDFileStream };
        if ( false == file->open(filePath.c_str(), mode) ) {
            dalError("Failed to open file: {}"_format(filePath));
            return { nullptr };
        }

        return file;
#elif defined(__ANDROID__)
        std::string filePath;
        std::unique_ptr<IResourceStream> file;

        if ( PACKAGE_NAME_ASSET == resID.getPackage() ) {
            filePath = resID.makeFilePath();
            file.reset(new AssetSteam);
        }
        else if ( LOG_FOLDER_NAME == resID.getPackage() ) {
            filePath = fmt::format("{}{}/{}", g_storagePath, LOG_FOLDER_NAME, resID.makeFilePath());
            assertDir_log();
            file.reset(new STDFileStream);
        }
        else {
            const auto dirToAssert = fmt::format("{}{}/{}", g_storagePath, USERDATA_FOLDER_NAME, resID.getPackage());
            filePath = fmt::format("{}/{}", dirToAssert, resID.makeFilePath());
            assertDir_userdata();
            assertDir(dirToAssert.c_str());
            file.reset(new STDFileStream);
        }

        if ( !file->open(filePath.c_str(), mode) ) {
            dalError("Failed to open file: {}"_format(resID.makeIDStr()));
            return { nullptr };
        }

        return file;
#endif

    }

    std::vector<std::string> listdir(ResourceID resID) {
        std::vector<std::string> result;

#if defined(_WIN32)
        {
            std::string filePath;
            if ( resID.getPackage() == PACKAGE_NAME_ASSET ) {
                filePath = fmt::format("{}{}/{}", getResourceDir_win(), PACKAGE_NAME_ASSET, resID.getOptionalDir());
            }
            else if ( resID.getPackage() == LOG_FOLDER_NAME ) {
                filePath = fmt::format("{}{}/{}", getResourceDir_win(), LOG_FOLDER_NAME, resID.getOptionalDir());
                assertDir_log();
            }
            else {
                const auto dirToAssert = fmt::format("{}{}/{}", getResourceDir_win(), USERDATA_FOLDER_NAME, resID.getPackage());
                filePath = fmt::format("{}/{}", dirToAssert, resID.getOptionalDir());
                assertDir_userdata();
                assertDir(dirToAssert.c_str());
            }

            getListFolFile_win(filePath, result);
        }
#elif defined(__ANDROID__)
        {

        }
#endif

        return result;
    }

}