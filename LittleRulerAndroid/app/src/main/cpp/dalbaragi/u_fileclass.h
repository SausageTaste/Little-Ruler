#pragma once

#include <vector>
#include <string>


namespace dal {

	bool initFilesystem(void* mgr);
	bool isFilesystemReady(void);

	bool readFileAsStr(const char* const path, std::string* buf);
	bool readImageFile(const char* const path, std::vector<uint8_t>* const output, int* const width, int* const height, int* const pixSize);


	struct InFileclassPimpl;


	class AssetFileIn {

	private:
		InFileclassPimpl* pimpl;
		size_t m_fileContentsSize;
		bool m_opened;
		std::string m_path;

	public:
		AssetFileIn(void);
		AssetFileIn(const char* const path);
		~AssetFileIn(void);

		bool open(const char* const path);
		void close(void);
		bool read(uint8_t* const buf, size_t bufSize);

		size_t getFileSize(void) const;

	};

}