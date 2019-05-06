#ifdef _WIN32
#pragma warning(disable:4996)
// To disable fopen deprecated error which is caused by tga.h
#endif

#include "u_fileclass.h"

#include <string>
#include <vector>
#include <memory>
#include <cassert>

#include <lodepng.h>
#include <tga.h>

#include "s_logger_god.h"
#include "u_autofree.h"

#if defined(_WIN32)
#include <fstream>
#include <windows.h>
#elif defined(__ANDROID__)
#include <android/asset_manager.h>
#include <zconf.h> // Just for SEEK_SET

#endif


using namespace std::string_literals;


namespace {

	const std::string PACKAGE_ASSET{ "asset" };

}


namespace {

#if defined(_WIN32)
	int getDirList(const char* const path, std::vector<std::string>& con) {
		con.clear();

		std::string pattern{ path };
		if (pattern.back() != '/') pattern.push_back('/');
		pattern.push_back('*');

		WIN32_FIND_DATA data;
		HANDLE hFind;
		if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
			do {
				con.push_back(data.cFileName);
				if (con.back() == "System Volume Information"s) {
					con.clear();
					return 0;
				}
			} while (FindNextFile(hFind, &data) != 0);
			FindClose(hFind);
		}

		return con.size();
	}

	const std::string& getResourceDir(void) {
		static std::string path;

		if (path.empty()) {
			std::vector<std::string> folders;
			std::string pattern("./");

			while (getDirList(pattern.c_str(), folders) > 0) {
				const auto found = [&folders](void) -> bool {
					for (auto& item : folders) {
						if (item == "Resources"s) return true;
					}
					return false;
				}();

				if (found) {
					path = pattern += "Resources/assets/";
					break;
				}

				pattern.append("../");
			}
		}

		return path;
	}
#elif defined(__ANDROID__)
	AAssetManager* gAssetMgr = nullptr;
#endif

}




namespace {

	struct ResourcePath {
		std::string m_package, m_additionalDir, m_name, m_ext;
	};

	bool parseResPath(const char* const path, ResourcePath& result) {
		std::string pathStr{ path };

		const auto packagePos = pathStr.find("::");
		size_t excludePos = 0;
		if      (0 == packagePos) {
			result.m_package = "::";
			excludePos = 2;
		}
		else if (std::string::npos == packagePos) {
			result.m_package = "";
		}
		else {
			result.m_package = pathStr.substr(0, packagePos);
			excludePos = packagePos + 2;
		}

		const auto dirPos = pathStr.rfind("/") + 1;
		if (std::string::npos == dirPos) {
			result.m_additionalDir = "";
		}
		else {
			result.m_additionalDir = pathStr.substr(excludePos, dirPos - excludePos);
			excludePos = dirPos;
		}

		const auto extPos = pathStr.rfind(".");
		if (std::string::npos == extPos) {
			result.m_name = pathStr.substr(excludePos, pathStr.size() - excludePos);
			result.m_ext = "";
		}
		else {
			result.m_name = pathStr.substr(excludePos, extPos - excludePos);
			result.m_ext = pathStr.substr(extPos + 1, pathStr.size() - extPos - 1);
		}

		return true;
	}

}


namespace {

	bool readFileAsPNG(const char* const path, std::vector<uint8_t>* output, size_t* const width, size_t* const height) {
		std::vector<uint8_t> buf;
		
		/* Fill buffer */ {
			dal::AssetFileStream file;
			file.open(path);
			auto bufSize = file.getFileSize();

			buf.reserve(bufSize);
			buf.resize(bufSize);
			file.read(buf.data(), buf.capacity());
		}

		unsigned int w, h;
		output->clear();

		auto error = lodepng::decode(*output, w, h, buf);
		if (error) {
			auto errMsg = "PNG decode error: "s + lodepng_error_text(error);
			dal::LoggerGod::getinst().putFatal(errMsg);
			return false;
		}

		*width = w;
		*height = h;

		return true;
	}

	bool readFileAsTGA(const char* const path, std::vector<uint8_t>* output, size_t* const width, size_t* const height, size_t* const pixSize) {
		dal::AssetFileStream file;
		file.open(path);
		const auto bufSize = file.getFileSize();
		auto buf = std::unique_ptr<uint8_t>{ new uint8_t[bufSize] };
		const auto res = file.read(buf.get(), bufSize);
		if (res == 0) {
			return false;
		}

		int w, h, p;
		dal::AutoFree<uint8_t> result{ tga_load_memory(buf.get(), bufSize, &w, &h, &p) };
		*width = static_cast<size_t>(w);
		*height = static_cast<size_t>(h);
		*pixSize = static_cast<size_t>(p);
		if (result.isNull()) {
			dal::LoggerGod::getinst().putFatal("Failed to parse tga file: "s + path);
			return false;
		}
		const auto resArrSize = (*width) * (*height) * (*pixSize);

		output->clear();
		output->insert(output->begin(), *result, *result + resArrSize);
		
		return true;
	}

	void getFileList(const char* const path, std::vector<std::string>* results) {

#if defined(_WIN32)
		dal::LoggerGod::getinst().putFatal("Not implemented.");
#elif defined(__ANDROID__)
		// Check error
		if (!dal::file::isFilesystemReady()) {
			dal::LoggerGod::getinst().putError("Filesystem is not initialized");
		}

		// Do
		AAssetDir* assetDir = AAssetManager_openDir(gAssetMgr, path);
		while (true) {
			auto fileName = AAssetDir_getNextFileName(assetDir);
			if (fileName == nullptr) break;
			results->push_back(fileName);
		}
		AAssetDir_close(assetDir);
#endif

	}

}


namespace dal {
	namespace filec {

		bool initFilesystem(void* mgr) {

#if defined(_WIN32)
			return true;
#elif defined(__ANDROID__)
			if (mgr == nullptr) {
				return false;
			}
			else {
				gAssetMgr = (AAssetManager*)mgr;
				return true;
			}
#endif

		}

		bool isFilesystemReady(void) {
#if defined(_WIN32)
			return true;
#elif defined(__ANDROID__)
			return gAssetMgr != nullptr;
#endif
		}

		bool getAsset_text(const char* const path, std::string* bufStr) {
			AssetFileStream file;
			if (file.open(path) != true) {
				return false;
			}

			auto bufSize = file.getFileSize();
			auto buf = std::unique_ptr<uint8_t>(new uint8_t[bufSize + 1]);
			file.read(buf.get(), bufSize);
			buf.get()[bufSize] = '\0';

			bufStr->clear();
			*bufStr = reinterpret_cast<char*>(buf.get());
			return true;
		}

		bool getResource_image(const char* const path, ImageFileData& data) {
			const auto len = strlen(path);
			assert(path[len - 4] == '.');

			std::string paramExt{ path + len - 3 };
			if (paramExt == "tga"s) {
				const auto res = readFileAsTGA(path, &data.m_buf, &data.m_width, &data.m_height, &data.m_pixSize);
				assert(data.m_pixSize == 4 || data.m_pixSize == 3);
				return res;
			}
			else if (paramExt == "png"s) {
				const auto res = readFileAsPNG(path, &data.m_buf, &data.m_width, &data.m_height);
				const size_t calcSize = size_t(data.m_width) * size_t(data.m_height) * size_t(4);
				assert(data.m_buf.size() == calcSize);
				data.m_pixSize = 4;
				return res;
			}
			else {
				LoggerGod::getinst().putError("Not supported image file type: "s + path);
				return false;
			}
		}

		bool getResource_buffer(const char* const path, std::vector<uint8_t>& buffer) {
			ResourcePath resPath; parseResPath(path, resPath);
			if (PACKAGE_ASSET != resPath.m_package) throw - 1;

			const auto filePath = resPath.m_additionalDir + resPath.m_name + '.' + resPath.m_ext;
			AssetFileStream file;
			const auto openRes = file.open(filePath.c_str());
			if (!openRes) return false;

			buffer.resize(file.getFileSize());
			const auto reeadRes = file.read(buffer.data(), buffer.size());
			if (!reeadRes) return false;

			return true;
		}

	}
}




namespace dal {

	struct AssetFileStream::InFileclassPimpl {
#if defined(_WIN32)
		std::ifstream mIFile;
#elif defined(__ANDROID__)
		AAsset* mOpenedAsset = nullptr;
#endif
	};


	AssetFileStream::AssetFileStream(void)
		: pimpl(new InFileclassPimpl()),
		m_fileContentsSize(0),
		m_opened(false)
	{

	}

	AssetFileStream::AssetFileStream(const char* const path)
		: AssetFileStream()
	{
		const auto res = this->open(path);
		if (!res)  throw - 1;
	}

	AssetFileStream::~AssetFileStream(void) {
		if (m_opened) this->close();

		delete this->pimpl;
	}

	bool AssetFileStream::open(const char* const path) {

#if defined(_WIN32)
		m_path = getResourceDir() + path;

		this->pimpl->mIFile.open(m_path.c_str(), std::ios::binary);
		if (!this->pimpl->mIFile) {
			LoggerGod::getinst().putError("Failed to open file: "s + m_path);
			m_opened = false;
			return false;
		}

		this->pimpl->mIFile.seekg(0, std::ios::end);
		m_fileContentsSize = static_cast<size_t>(this->pimpl->mIFile.tellg());
		this->pimpl->mIFile.seekg(0, std::ios::beg);
#elif defined(__ANDROID__)
		m_path = path;

		this->pimpl->mOpenedAsset = AAssetManager_open(gAssetMgr, path, AASSET_MODE_UNKNOWN);
		if (this->pimpl->mOpenedAsset == nullptr) {
			LoggerGod::getinst().putError("Failed to open file: "s + path);
			m_opened = false;
			return false;
		}

		this->m_fileContentsSize = (unsigned int)AAsset_getLength64(this->pimpl->mOpenedAsset);
		if (this->m_fileContentsSize <= 0) {
			LoggerGod::getinst().putWarn("File contents' length is 0: "s + path);
		}
#endif

		m_opened = true;
		return true;
	}

	void AssetFileStream::close(void) {

#if defined(_WIN32)
		this->pimpl->mIFile.close();
#elif defined(__ANDROID__)
		AAsset_close(this->pimpl->mOpenedAsset);
#endif

		this->m_fileContentsSize = 0;
		m_opened = false;
	}

	size_t AssetFileStream::read(uint8_t * const buf, const size_t bufSize) {
		auto sizeToRead = bufSize < m_fileContentsSize ? bufSize : m_fileContentsSize;

#if defined(_WIN32)
		this->pimpl->mIFile.read(reinterpret_cast<char*>(buf), sizeToRead);

		if (!this->pimpl->mIFile) {
			LoggerGod::getinst().putError(
				"File not read completely, only "s + std::to_string(this->pimpl->mIFile.gcount()) + "could be read:"s + m_path
			);
			return 0;
		}
#elif defined(__ANDROID__)
		if (AAsset_read(this->pimpl->mOpenedAsset, buf, sizeToRead) < 0) {
			LoggerGod::getinst().putError("Failed to read file:"s + m_path);
			return 0;
		}
#endif

		return sizeToRead;
	}

	size_t AssetFileStream::getFileSize(void) const {
		return this->m_fileContentsSize;
	}

	bool AssetFileStream::seek(const size_t offset, const Whence whence) {

#if defined(_WIN32)
		switch (whence) {
		case Whence::beg:
			pimpl->mIFile.seekg(offset, std::ios_base::beg);
			break;
		case Whence::cur:
			pimpl->mIFile.seekg(offset, std::ios_base::cur);
			break;
		case Whence::end:
			pimpl->mIFile.seekg(offset, std::ios_base::end);
			break;
		}

		return !pimpl->mIFile.fail();
#elif defined(__ANDROID__)
		decltype(SEEK_SET) cwhence;

		switch (whence) {
		case Whence::beg:
			cwhence = SEEK_SET;
			break;
		case Whence::cur:
			cwhence = SEEK_CUR;
			break;
		case Whence::end:
			cwhence = SEEK_END;
			break;
		}

		return AAsset_seek(pimpl->mOpenedAsset, static_cast<off_t>(offset), cwhence) != -1;
#endif

	}

	size_t AssetFileStream::tell(void) const {

#if defined(_WIN32)
		return static_cast<size_t>(this->pimpl->mIFile.tellg());
#elif defined(__ANDROID__)
		const auto curPos = AAsset_getRemainingLength(pimpl->mOpenedAsset);
		return m_fileContentsSize - static_cast<size_t>(curPos);
#endif

	}

	size_t AssetFileStream::write(const uint8_t * const buf, const size_t bufSize) {
		LoggerGod::getinst().putFatal("Not implemented: AssetFileStream::write");
		throw - 1;
	}

}

