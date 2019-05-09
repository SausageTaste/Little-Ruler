#pragma once

#include <vector>
#include <string>

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

	};


	namespace filec {

		bool initFilesystem(void* mgr);
		bool isFilesystemReady(void);

		bool getAsset_text(const char* const path, std::string* buf);

		bool getResource_image(const ResourceID& path, loadedinfo::ImageFileData& data);
		bool getResource_buffer(const ResourceID& path, std::vector<uint8_t>& buffer);

	}
}


namespace dal {

	class AssetFileStream {

	public:
		enum class Whence { beg, cur, end };

	private:
		struct InFileclassPimpl;
		InFileclassPimpl* pimpl;

		size_t m_fileContentsSize;
		bool m_opened;
		std::string m_path;

	public:
		AssetFileStream(void);
		AssetFileStream(const char* const path);
		~AssetFileStream(void);

		bool open(const char* const path);
		void close(void);
		size_t read(uint8_t* const buf, const size_t bufSize);

		size_t getFileSize(void) const;

		bool seek(const size_t offset, const Whence whence = Whence::beg);
		size_t tell(void) const;
		size_t write(const uint8_t* const buf, const size_t bufSize);

	};

}