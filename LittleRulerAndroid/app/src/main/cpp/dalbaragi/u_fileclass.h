#pragma once

#include <vector>
#include <string>


namespace dal {

	
	namespace filec {

		struct ImageFileData {
			std::vector<uint8_t> m_buf;
			size_t m_width = 0, m_height = 0, m_pixSize = 0;
		};

		bool initFilesystem(void* mgr);
		bool isFilesystemReady(void);

		bool getAsset_text(const char* const path, std::string* buf);
		bool getResource_image(const char* const path, ImageFileData& data);

		bool getResource_buffer(const char* const path, std::vector<uint8_t>& buffer);

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