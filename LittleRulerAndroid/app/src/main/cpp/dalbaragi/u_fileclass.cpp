#ifdef _WIN32
#pragma warning(disable:4996)
// To disable fopen deprecated error which is caused by tga.h
#endif

#include "u_fileclass.h"

#include <string>
#include <vector>
#include <memory>
#include <iostream>
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

	auto& g_logger = dal::LoggerGod::getinst();

	const std::string PACKAGE_NAME_ASSET{ "asset" };
	const std::string RESOURCE_FOLDER_NAME{ "Resource" };

	constexpr unsigned int g_assetDirCount = 3;
	const char* const g_assetDirs[g_assetDirCount] = {
		"maps/",
		"models/",
		"texture/"
	};

#if defined(_WIN32)

	size_t getListFolFile_win(std::string pattern, std::vector<std::string>& con) {
		con.clear();

		if (pattern.back() != '/') pattern.push_back('/');
		pattern.push_back('*');

		WIN32_FIND_DATA data;
		HANDLE hFind;
		if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
			do {
				const auto fileName = std::string{ data.cFileName };
				if (fileName == "."s) continue;
				if (".."s == fileName) continue;

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

	const std::string& getResourceDir_win(void) {
		static std::string path;

		if (path == "") {
			std::vector<std::string> folders;
			std::string pattern("./");

			while (getListFolFile_win(pattern.c_str(), folders) > 0) {
				const auto found = [&folders](void) -> bool {
					for (auto& item : folders) {
						if (item == RESOURCE_FOLDER_NAME) return true;
					}
					return false;
				}();

				if (found) {
					path = pattern += RESOURCE_FOLDER_NAME + '/';
					break;
				}

				pattern.append("../");
			}
		}

		return path;
}

	bool isFile_win(const std::string path) {
		if (FILE* file = fopen(path.c_str(), "r")) {
			fclose(file);
			return true;
		}
		else {
			return false;
		}
	}

	bool findMatching_win(std::string & result, std::string path, const std::string & criteria) {
		if (!path.empty() && path.back() != '/') path.push_back('/');

		std::vector<std::string> dirs;
		getListFolFile_win(path, dirs);
		for (const auto& one : dirs) {
			auto newPath = path + one;
			if (isFile_win(newPath)) {
				if (one == criteria) {
					result = path + one;
					return true;
				}
			}
			else {
				if (findMatching_win(result, newPath, criteria)) {
					return true;
				}
			}
		}

		return false;
	};

#elif defined(__ANDROID__)

	AAssetManager* gAssetMgr = nullptr;

	size_t getFileList_android(std::string path, std::vector<std::string>& dirs) {
		dirs.clear();
		if (!path.empty() && path.back() == '/') path.pop_back();

		AAssetDir* assetDir = AAssetManager_openDir(gAssetMgr, path.c_str());
		while (true) {
			auto filename = AAssetDir_getNextFileName(assetDir);
			if (filename == nullptr) break;
			dirs.emplace_back(filename);
		}
		AAssetDir_close(assetDir);

		return dirs.size();
	}

#endif

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
	
}


// Resource ID
namespace dal {

	ResourceID::ResourceID(const std::string& resourceID) {
		const auto& pathStr = resourceID;

		const auto packagePos = pathStr.find("::");
		size_t excludePos = 0;
		if (0 == packagePos) {
			this->m_package = "";
			excludePos = 2;
		}
		else if (std::string::npos == packagePos) {
			this->m_package = "";
		}
		else {
			this->m_package = pathStr.substr(0, packagePos);
			excludePos = packagePos + 2;
		}

		const auto dirPos = pathStr.rfind("/");
		if (std::string::npos == dirPos) {
			this->m_dir = "";
		}
		else {
			this->m_dir = pathStr.substr(excludePos, dirPos + 1 - excludePos);
			excludePos = dirPos + 1;
		}

		const auto extPos = pathStr.rfind(".");
		if (std::string::npos == extPos) {
			this->m_dir += pathStr.substr(excludePos, pathStr.size() - excludePos);
			this->m_bareName = "";
			this->m_ext = "";
		}
		else {
			this->m_bareName = pathStr.substr(excludePos, extPos - excludePos);
			this->m_ext = pathStr.substr(extPos, pathStr.size() - extPos);
		}

		if (!this->m_dir.empty() && this->m_dir.back() != '/') this->m_dir.push_back('/');
	}

	ResourceID::ResourceID(const char* const resourceID) : ResourceID(std::string{ resourceID }) {

	}

	ResourceID::ResourceID(const std::string & package, const std::string & optionalDir, const std::string & bareName, const std::string & ext)
		: m_package(package),
		m_dir(optionalDir),
		m_bareName(bareName),
		m_ext(ext)
	{

	}

	const std::string& ResourceID::getPackage(void) const {
		return this->m_package;
	}

	const std::string& ResourceID::getOptionalDir(void) const {
		return this->m_dir;
	}

	const std::string& ResourceID::getBareName(void) const {
		return this->m_bareName;
	}

	const std::string& ResourceID::getExt(void) const {
		return this->m_ext;
	}

	std::string ResourceID::makeIDStr(void) const {
		return this->m_package + "::" + this->m_dir + this->m_bareName + this->m_ext;
	}

	std::string ResourceID::makeFileName(void) const {
		return this->m_bareName + this->m_ext;
	}

	std::string ResourceID::makeFilePath(void) const {
		return this->m_dir + this->m_bareName + this->m_ext;
	}


	void ResourceID::setOptionalDir(const std::string t) {
		this->m_dir = t;
	}

}


namespace dal {
	namespace filec {

		bool resolveRes(dal::ResourceID& result) {
			const auto fileName = result.makeFileName();

			if (PACKAGE_NAME_ASSET != result.getPackage()) {
				g_logger.putError("Cannot resolve " + result.getPackage() + "::" + fileName + ", only asset is supported yes.");
				return false;
			}
			if (result.getPackage().empty()) {
				g_logger.putError("Cannot resolve " + fileName + " without package defined.");
				return false;
			}

#if defined(_WIN32)
			const auto& path = getResourceDir_win() + result.getPackage() + '/';
			std::string resultStr;
			if (findMatching_win(resultStr, path, fileName)) {
				result.setOptionalDir(resultStr.substr(path.size(), resultStr.find(fileName) - path.size()));
				g_logger.putInfo("Resource resolved: " + result.makeIDStr());
				return true;
			}
			else {
				return false;
			}
#elif defined(__ANDROID__)

#endif
			return false;
		}

		bool initFilesystem(void* mgr) {

#if defined(_WIN32)
			return true;
#elif defined(__ANDROID__)
			if (mgr == nullptr) {
				return false;
			}
			else {
				gAssetMgr = reinterpret_cast<AAssetManager*>(mgr);
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
			if (!file.open( path)) return false;

			auto bufSize = file.getFileSize();
			auto buf = std::unique_ptr<uint8_t>(new uint8_t[bufSize + 1]);
			if (!file.read(buf.get(), bufSize)) return false;
			buf.get()[bufSize] = '\0';

			bufStr->clear();
			*bufStr = reinterpret_cast<char*>(buf.get());

			return true;
		}

		bool getResource_image(ResourceID path, loadedinfo::ImageFileData& data) {
			if (PACKAGE_NAME_ASSET != path.getPackage()) throw - 1;

			if (path.getOptionalDir().empty()) {
				resolveRes(path);
			}
			auto newPath = path.makeFilePath();

			if (path.getExt() == ".tga"s) {
				const auto res = readFileAsTGA(newPath.c_str(), &data.m_buf, &data.m_width, &data.m_height, &data.m_pixSize);
				assert(data.m_pixSize == 4 || data.m_pixSize == 3);
				return res;
			}
			else if (path.getExt() == ".png"s) {
				const auto res = readFileAsPNG(newPath.c_str(), &data.m_buf, &data.m_width, &data.m_height);
				const size_t calcSize = size_t(data.m_width) * size_t(data.m_height) * size_t(4);
				assert(data.m_buf.size() == calcSize);
				data.m_pixSize = 4;
				return res;
			}
			else {
				LoggerGod::getinst().putError("Not supported image file type: "s + path.makeIDStr());
				return false;
			}
		}

		bool getResource_buffer(ResourceID path, std::vector<uint8_t>& buffer) {
			if (PACKAGE_NAME_ASSET != path.getPackage()) throw - 1;

			if (path.getOptionalDir().empty()) {
				resolveRes(path);
			}

			const auto filePath = path.makeFilePath();

			AssetFileStream file;
			if (!file.open(filePath.c_str())) { throw - 1;  return false; }

			buffer.resize(file.getFileSize());
			const auto reeadRes = file.read(buffer.data(), buffer.size());
			if (!reeadRes) { throw - 1; return false; }

			return true;
		}

	}
}


// Asset File Stream
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
		m_path = path;
		if (m_path.find(getResourceDir_win()))
		m_path = getResourceDir_win() + "asset/" + path;

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
				"File not read completely, only "s + std::to_string(this->pimpl->mIFile.gcount()) + " could be read:"s + m_path
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