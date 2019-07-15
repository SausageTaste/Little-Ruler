#pragma once

#include <vector>
#include <string>
#include <memory>

#include "u_loadinfo.h"


namespace dal {

    class ResourceID {

    private:
        std::string m_package, m_dir, m_bareName, m_ext;

    public:
        ResourceID(void) = default;
        ResourceID(const char* const resourceID);
        ResourceID(const std::string& resourceID);
        ResourceID(const std::string& package, const std::string& optionalDir, const std::string& bareName, const std::string& ext);

        const std::string& getPackage(void) const;
        const std::string& getOptionalDir(void) const;
        const std::string& getBareName(void) const;
        const std::string& getExt(void) const;

        std::string makeIDStr(void) const;
        std::string makeFileName(void) const;
        std::string makeFilePath(void) const;

        void setPackage(const std::string& t);
        void setPackageIfEmpty(const std::string& t);
        void setOptionalDir(const std::string& t);

    };

    bool resolveRes(dal::ResourceID& result);

    bool initFilesystem(void* mgr, const char* const sdcardPath);
    bool isFilesystemReady(void);

}


namespace dal::futil {

    bool getRes_text(const ResourceID& resID, std::string& buffer);
    bool getRes_image(const ResourceID& resID, loadedinfo::ImageFileData& data);
    bool getRes_buffer(const ResourceID& resID, std::vector<uint8_t>& buffer);

}


namespace dal {

    enum class FileMode { read, write, append, bread, bwrite, bappend };
    enum class Whence { beg, cur, end };

    FileMode mapFileMode(const char* const str);


    class IResourceStream {

    public:
        IResourceStream(void) = default;
        virtual ~IResourceStream(void) = default;

    private:
        IResourceStream(const IResourceStream&) = delete;
        IResourceStream(IResourceStream&&) = delete;
        IResourceStream& operator=(const IResourceStream&) = delete;
        IResourceStream& operator=(IResourceStream&&) = delete;

    public:
        virtual bool open(const char* const path, const FileMode mode) = 0;
        virtual void close(void) = 0;

        virtual size_t read(uint8_t* const buf, const size_t bufSize) = 0;
        virtual bool readText(std::string& buffer) = 0;
        virtual bool write(const uint8_t* const buf, const size_t bufSize) = 0;
        virtual bool write(const char* const str) = 0;
        virtual size_t getSize(void) = 0;

        virtual bool isOpen(void) = 0;
        virtual bool seek(const size_t offset, const Whence whence = Whence::beg) = 0;
        virtual size_t tell(void) = 0;

    };


    std::unique_ptr<IResourceStream> resopen(ResourceID resID, const FileMode mode);

}