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
#endif


using namespace std::string_literals;


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
					dal::LoggerGod::getinst().putInfo("Resource path: "s + pattern);
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


using namespace std::string_literals;


namespace {

	bool readFileAsPNG(
		const char* const path, std::vector<uint8_t>* output,
		int* const width, int* const height
	) {
		std::vector<uint8_t> buf;
		
		/* Fill buffer */ {
			dal::AssetFileIn file;
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

	bool readFileAsTGA(
		const char* const path, std::vector<uint8_t>* output,
		int* const width, int* const height, int* const pixSize
	) {
		dal::AssetFileIn file;
		file.open(path);
		const auto bufSize = file.getFileSize();
		auto buf = std::unique_ptr<uint8_t>{ new uint8_t[bufSize] };
		const auto res = file.read(buf.get(), bufSize);
		if (!res) {
			return false;
		}

		dal::AutoFree<uint8_t> result{ tga_load_memory(buf.get(), bufSize, width, height, pixSize) };
		if (result.isNull()) {
			dal::LoggerGod::getinst().putFatal("Failed to parse tga file: "s + path);
			return false;
		}
		const auto resArrSize = (*width) * (*height) * (*pixSize);

		output->clear();
		output->insert(output->begin(), *result, *result + resArrSize);
		
		return true;
	}

}


namespace dal {

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

	bool readFileAsStr(const char* const path, std::string* bufStr) {
		AssetFileIn file;
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

	bool readImageFile(const char* const path, std::vector<uint8_t>* const output, int* const width, int* const height, int* const pixSize) {
		const auto len = strlen(path);
		assert(path[len - 4] == '.');

		std::string paramExt{ path + len - 3 };
		if (paramExt == "tga"s) {
			const auto res = readFileAsTGA(path, output, width, height, pixSize);
			assert(*pixSize == 4 || *pixSize == 3);
			return res;
		}
		else if (paramExt == "png"s) {
			const auto res = readFileAsPNG(path, output, width, height);
			const size_t calcSize = size_t(*width) * size_t(*height) * size_t(4);
			assert(output->size() == calcSize);
			*pixSize = 4;
			return res;
		}
		else {
			LoggerGod::getinst().putError("Not supported image file type: "s + path);
			return false;
		}
	}

	void getFileList(const char* const path, std::vector<std::string>* results) {

#if defined(_WIN32)
		LoggerGod::getinst().putFatal("Not implemented.");
#elif defined(__ANDROID__)
		// Check error
		if (!isFilesystemReady()) {
			LoggerGod::getinst().putError("Filesystem is not initialized");
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

	struct InFileclassPimpl {
#if defined(_WIN32)
		std::ifstream mIFile;
#elif defined(__ANDROID__)
		AAsset* mOpenedAsset = nullptr;
#endif
	};


	AssetFileIn::AssetFileIn(void)
	:	pimpl(new InFileclassPimpl()),
		m_fileContentsSize(0),
		m_opened(false)
	{

	}

	AssetFileIn::AssetFileIn(const char* const path)
		: AssetFileIn()
	{
		const auto res = this->open(path);
		if (!res)  throw -1;
	}

	AssetFileIn::~AssetFileIn(void) {
		if (m_opened) this->close();

		delete this->pimpl;
	}

	bool AssetFileIn::open(const char* const path) {

#if defined(_WIN32)
		m_path = getResourceDir() + path;
		LoggerGod::getinst().putInfo(m_path);

		this->pimpl->mIFile.open(m_path.c_str(), std::ios::binary);
		if (!this->pimpl->mIFile){
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

	void AssetFileIn::close(void) {

#if defined(_WIN32)
		this->pimpl->mIFile.close();
#elif defined(__ANDROID__)
		AAsset_close(this->pimpl->mOpenedAsset);
#endif

		this->m_fileContentsSize = 0;
		m_opened = false;
	}

	bool AssetFileIn::read(uint8_t* const buf, size_t bufSize) {
		auto sizeToRead = bufSize < m_fileContentsSize ? bufSize : m_fileContentsSize;

#if defined(_WIN32)
		this->pimpl->mIFile.read(reinterpret_cast<char*>(buf), sizeToRead);

		if (!this->pimpl->mIFile) {
			LoggerGod::getinst().putError(
				"File not read completely, only "s + std::to_string(this->pimpl->mIFile.gcount()) + "could be read:"s + m_path
			);
			return false;
		}
#elif defined(__ANDROID__)
		if (AAsset_read(this->pimpl->mOpenedAsset, buf, sizeToRead) < 0) {
			LoggerGod::getinst().putError("Failed to read file:"s + m_path);
			return false;
		}
#endif

		return true;
	}

	size_t AssetFileIn::getFileSize(void) const {
		return this->m_fileContentsSize;
	}

}