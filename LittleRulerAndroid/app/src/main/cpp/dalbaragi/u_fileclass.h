#pragma once

#include <vector>
#include <string>


namespace dal {

	namespace file {

		bool initFilesystem(void* mgr);
		bool isFilesystemReady(void);

		bool readFileAsStr(const char* const path, std::string* buf);
		bool readImageFile(const char* const path, std::vector<uint8_t>* const output, int* const width, int* const height, int* const pixSize);

	}


	struct InFileclassPimpl;


	class AssetFileStream {

	public:
		enum class Whence { beg, cur, end };

	private:
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