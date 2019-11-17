#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>


// Path
namespace dal {

    struct ResPathInfo {
        std::string m_package, m_intermPath, m_finalPath;
        bool m_isResolveMode = false;
    };

    ResPathInfo parseResPath(const std::string& resPath);
    std::optional<ResPathInfo> resolvePath(const std::string& package, const std::string& dir, const std::string& fname);

    std::string findExtension(const std::string& path);

    bool assertUserdataFolder(void);
    bool assertLogFolder(void);

}


// Primitives
namespace dal {

    std::vector<std::string> listdir(const char* const resPath);
    std::vector<std::string> listfile(const char* const resPath);
    std::vector<std::string> listfolder(const char* const resPath);

    bool isdir(const char* const resPath);
    bool isfile(const char* const resPath);
    bool isfolder(const char* const resPath);

}


// File open
namespace dal {

    enum class FileMode2 { read = 0, write, append, bread, bwrite, bappend };  // Order shouldn't change.
    enum class Whence2 { beg, cur, end };

    class IFileStream {

    public:
        IFileStream(void) = default;
        virtual ~IFileStream(void) = default;

    public:
        IFileStream(const IFileStream&) = delete;
        IFileStream(IFileStream&&) = delete;
        IFileStream& operator=(const IFileStream&) = delete;
        IFileStream& operator=(IFileStream&&) = delete;

    public:
        virtual bool open(const char* const path, const FileMode2 mode) = 0;
        virtual void close(void) = 0;

        virtual size_t read(uint8_t* const buf, const size_t bufSize) = 0;
        virtual bool readText(std::string& buffer) = 0;

        virtual bool write(const uint8_t* const buf, const size_t bufSize) = 0;
        virtual bool write(const char* const str) = 0;
        virtual bool write(const std::string& str) = 0;

        virtual size_t getSize(void) = 0;
        virtual bool isOpen(void) = 0;
        virtual bool seek(const size_t offset, const Whence2 whence = Whence2::beg) = 0;
        virtual size_t tell(void) = 0;

    };

    
    std::unique_ptr<IFileStream> fileopen(const char* const resPath, const FileMode2 mode);

}
