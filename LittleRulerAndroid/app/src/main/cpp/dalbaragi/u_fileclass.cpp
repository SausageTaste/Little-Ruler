#ifdef _WIN32
#pragma warning(disable:4996)
// To disable fopen deprecated error which is caused by tga.h
#endif

#include "u_fileclass.h"

#include <string>
#include <vector>
#include <memory>
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
#endif


using namespace std::string_literals;
using namespace fmt::literals;


// Translation unit level globals
namespace {

	auto& g_logger = dal::LoggerGod::getinst();

	const std::string PACKAGE_NAME_ASSET{ "asset" };
	const std::string RESOURCE_FOLDER_NAME{ "Resource" };

	const std::string USERDATA_FOLDER_NAME{ "userdata" };
	const std::string LOG_FOLDER_NAME{ "log" };

	struct DirNode {
		std::string m_name;
		std::vector<DirNode> m_subfolders;
	};

	DirNode g_assetFolders = {
		PACKAGE_NAME_ASSET, {
			{ "font", {}},
			{ "glsl", {}},
			{ "map", {}},
			{ "model", {}},
			{ "texture", {
					{"dva", {}},
					{"mess", {}}
				}
			}
		}
	};

}


// Windows
namespace {

#ifdef _WIN32

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

#endif

}


// Android
namespace {

#ifdef __ANDROID__

	AAssetManager* gAssetMgr = nullptr;

	std::string g_storagePath;

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
	bool findMatchingAsset(std::string& result, const DirNode& node, const std::string&  accumPath, const std::string & criteria) {
		for (auto& folNode : node.m_subfolders) {
			std::string newPath;
			if (accumPath.empty()) {
				newPath = folNode.m_name;
			}
			else {
				newPath = accumPath + '/' + folNode.m_name;
			}

			std::vector<std::string> dirs;
			getFileList_android(newPath, dirs);
			for (auto& fileName : dirs) {
				if (criteria == fileName) {
					result = newPath;
					return true;
				}
			}

			if (findMatchingAsset(result, folNode, newPath, criteria)) return true;
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
		if (isdir(path)) return;

#if defined(_WIN32)
		const auto res = _mkdir(path);
#elif defined(__ANDROID__)
		const auto res = mkdir(path, 0);
#endif
		if (0 != res) {
			switch (errno) {
				case EEXIST:
					dalWarn("Checked isdir but dir already exists upon _mkdir for userdata.");
					break;
				case ENOENT:
					dalAbort("Invalid path name in assertDir_userdata: "s + path);
				case EROFS:
					dalAbort("Parent folder is read only: "s + path);
				default:
					dalAbort("Unknown errno for _mkdir in assertDir_userdata: "s + std::to_string(errno));
			}
		}
		else {
			dalInfo("Folder created: userdata");
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

			switch (mode) {
			case dal::FileMode::read:
				this->m_file.open(path, std::ios::in); break;
			case dal::FileMode::write:
				this->m_file.open(path, std::ios::out); break;
			case dal::FileMode::append:
				this->m_file.open(path, std::ios::out | std::ios::app); break;
			case dal::FileMode::bread:
				this->m_file.open(path, std::ios::in | std::ios::binary); break;
			case dal::FileMode::bwrite:
				this->m_file.open(path, std::ios::out | std::ios::binary); break;
			case dal::FileMode::bappend:
				this->m_file.open(path, std::ios::out | std::ios::app | std::ios::binary); break;
			}

			if (this->isOpen()) {
				return true;
			}
			else {
				g_logger.putError("Failed STDFileStream::open for: "s + path, __LINE__, __func__, __FILE__);
				return false;
			}
		}

		virtual void close(void) override {
			if (this->isOpen()) this->m_file.close();
		}

		virtual size_t read(uint8_t* const buf, const size_t bufSize) override {
			const auto remaining = this->getSize() - this->tell();
			const auto sizeToRead = bufSize < remaining ? bufSize : remaining;
			if (sizeToRead <= 0) return 0;

			this->m_file.read(reinterpret_cast<char*>(buf), sizeToRead);
			const auto readSize = this->m_file.gcount();

			if (!this->m_file) {
				std::string errMsg;
				const auto errFlags = this->m_file.rdstate();

				if ( (errFlags & std::fstream::eofbit) != 0 )
					errMsg += "eof, ";
				if ( (errFlags & std::fstream::failbit) != 0 )
					errMsg += "fail, ";
				if ( (errFlags & std::fstream::badbit) != 0 )
					errMsg += "bad, ";

				if ( readSize < sizeToRead )
					errMsg += "{} out of {} has been read"_format(readSize, sizeToRead);

				dalError("File not read completely, here are errors: " + errMsg);
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
			switch (whence) {

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

			switch (mode) {
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
			if (nullptr == this->m_asset) {
				dalError("Failed AssetSteam::open for: "s + path);
				return false;
			}

			this->m_fileSize = static_cast<size_t>(AAsset_getLength64(this->m_asset));
			if (this->m_fileSize <= 0) {
				dalWarn("File contents' length is 0 for: "s + path);
			}

			return true;
		}

		virtual void close(void) override {
			if (this->isOpen()) AAsset_close(this->m_asset);
			this->m_asset = nullptr;
			this->m_fileSize = 0;
		}

		virtual size_t read(uint8_t* const buf, const size_t bufSize) override {
			// Android asset manager implicitly read beyond file range WTF!!!
			const auto remaining = this->m_fileSize - this->tell();
			auto sizeToRead = bufSize < remaining ? bufSize : remaining;
			if (sizeToRead <= 0) return 0;

			const auto readBytes = AAsset_read(this->m_asset, buf, sizeToRead);
			if (readBytes < 0) {
				dalError("Failed to read asset.");
				return 0;
			}
			else if (0 == readBytes) {
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

		virtual size_t getSize(void) override {
			return this->m_fileSize;
		}

		virtual bool isOpen(void) override {
			return nullptr != this->m_asset;
		}

		virtual bool seek(const size_t offset, const dal::Whence whence = dal::Whence::beg) override {
			decltype(SEEK_SET) cwhence;

			switch (whence) {
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

	bool parseImagePNG(dal::loadedinfo::ImageFileData& data, const std::vector<uint8_t>& dataBuffer) {
		unsigned int w, h;
		auto error = lodepng::decode(data.m_buf, w, h, dataBuffer);
		if (error) {
			dalError("PNG decode error: "s + lodepng_error_text(error));
			return false;
		}

		data.m_width = static_cast<size_t>(w);
		data.m_height = static_cast<size_t>(h);
		data.m_pixSize = 4;

		// Assert that pixel size is 4.
		assert(data.m_width * data.m_height * data.m_pixSize == data.m_buf.size());

		return true;
	}

	bool parseImageTGA(dal::loadedinfo::ImageFileData& data, uint8_t* const buf, const size_t bufSize) {
		int w, h, p;
		std::unique_ptr<uint8_t, decltype(free)*> result{ tga_load_memory(buf, static_cast<int>(bufSize), &w, &h, &p), free };
		if (nullptr == result) {
			dalError("Failed to parse tga file.");
			return false;
		}

		data.m_width = static_cast<size_t>(w);
		data.m_height = static_cast<size_t>(h);
		data.m_pixSize = static_cast<size_t>(p);

		const auto resArrSize = data.m_width * data.m_height * data.m_pixSize;
		data.m_buf.clear();
		data.m_buf.insert(data.m_buf.begin(), result.get(), result.get() + resArrSize);

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

	void ResourceID::setPackage(const std::string& t) {
		this->m_package = t;
	}

	void ResourceID::setOptionalDir(const std::string& t) {
		this->m_dir = t;
		if (!this->m_dir.empty() && '/' != this->m_dir.back()) {
			this->m_dir.push_back('/');
		}
	}

}


// filec
namespace dal {

	namespace futil {

		bool getRes_text(const ResourceID& resID, std::string& buffer) {
			auto file = resopen(resID, FileMode::read);
			if (nullptr == file) return false;

			return file->readText(buffer);
		}

		bool getRes_image(const ResourceID& resID, loadedinfo::ImageFileData& data) {
			const std::unordered_set<std::string> supportedFormats{ ".tga", ".png" };
			if ( supportedFormats.find( resID.getExt() ) == supportedFormats.end() ) {
				dalError( "Not supported image file format: "s + resID.makeIDStr() );
				return false;
			}

			std::vector<uint8_t> fileBuffer;
			auto file = resopen( resID, FileMode::bread );
			if ( nullptr == file ) return false;

			const auto fileSize = file->getSize();
			fileBuffer.resize( fileSize );
			if ( !file->read( fileBuffer.data(), fileBuffer.size() ) ) return false;

			if (".tga"s == resID.getExt()) {
				const auto res = parseImageTGA( data, fileBuffer.data(), fileBuffer.size() );
				if ( !res ) dalError( "Error while parsing tga image: "s + resID.makeIDStr() );
				return res;
			}
			else if (".png"s == resID.getExt()) {
				const auto res = parseImagePNG( data, fileBuffer );
				if ( !res ) dalError( "Error while parsing png image: "s + resID.makeIDStr() );
				return res;
			}
			else {
				dalError( "Early support test for image has failed: "s + resID.makeIDStr() );
				return false;
			}
		}

		bool getRes_buffer(const ResourceID& resID, std::vector<uint8_t>& buffer) {
			auto file = resopen( resID, FileMode::bread );
			if ( nullptr == file ) return false;

			const auto fileSize = file->getSize();
			buffer.resize(fileSize);
			return file->read( buffer.data(), buffer.size() ) == fileSize;
		}

	}


	bool resolveRes(ResourceID& result) {
		const auto fileName = result.makeFileName();

		if (result.getPackage().empty()) {
			dalError("Cannot resolve " + fileName + " without package defined.");
			return false;
		}

#if defined(_WIN32)
		std::string path;
		if ( PACKAGE_NAME_ASSET == result.getPackage() ) {
			path = getResourceDir_win() + result.getPackage() + '/';
		}
		else {
			path = getResourceDir_win() + USERDATA_FOLDER_NAME + '/' + result.getPackage() + '/';
		}

		std::string resultStr;
		if (findMatching_win(resultStr, path, fileName)) {
			result.setOptionalDir(resultStr.substr(path.size(), resultStr.find(fileName) - path.size()));
			dalInfo("Resource resolved: " + result.makeIDStr(), __LINE__, __func__, __FILE__);
			return true;
		}
		else {
			dalError("Resource resolve failed: " + result.makeIDStr(), __LINE__, __func__, __FILE__);
			return false;
		}
#elif defined(__ANDROID__)
		if (PACKAGE_NAME_ASSET == result.getPackage()) {
			std::string foundStr;
			if (findMatchingAsset(foundStr, g_assetFolders, "", result.makeFileName())) {
				result.setOptionalDir(foundStr);
				dalInfo("Resource resolved: " + result.makeIDStr());
				return true;
			}
			else {
				dalError("Resource resolve failed: " + result.makeIDStr());
				return false;
			}
		}
		else {
			dalError("Cannot resolve " + result.getPackage() + "::" + fileName + ", only asset is supported yet.");
			return false;
		}
#endif

	}

	bool initFilesystem(void* mgr, const char* const sdcardPath) {

#ifdef __ANDROID__
		if (mgr == nullptr)  return false;
		gAssetMgr = reinterpret_cast<AAssetManager*>(mgr);

		if (nullptr == sdcardPath) return false;
		g_storagePath = sdcardPath;
		dalInfo("Storage path set: "s + g_storagePath);
#endif

		return true;
	}

	bool isFilesystemReady(void) {
#ifdef __ANDROID__
		if (gAssetMgr == nullptr) return false;
		if (g_storagePath.empty()) return false;
#endif
		return true;
	}

		
	FileMode mapFileMode(const char* const str) {
		// "wb", "w", "wt", "rb", "r", "rt".

		std::unordered_map<std::string, dal::FileMode> map{
			{ "wb", dal::FileMode::bwrite },
			{ "w", dal::FileMode::write },
			{ "wt", dal::FileMode::write },
			{ "rb", dal::FileMode::bread },
			{ "r", dal::FileMode::read },
			{ "rt", dal::FileMode::read },
			{ "ab", dal::FileMode::bappend },
			{ "a", dal::FileMode::append },
			{ "at", dal::FileMode::append },
		};

		const auto iter = map.find(str);
		if (map.end() != iter) {
			return iter->second;
		}
		else {
			dalAbort("Unkown file mode str: "s + str);
		}
	}

	std::unique_ptr<IResourceStream> resopen(ResourceID resID, const FileMode mode) {
		if ( resID.getPackage().empty() ) {
			dalError("Caanot open resource without package specified: "s + resID.makeIDStr());
			return std::unique_ptr<IResourceStream>{ nullptr };
		}

		if (FileMode::read != mode && FileMode::bread != mode) goto finishResolve;
		if (!resID.getOptionalDir().empty()) goto finishResolve;

		if (!resolveRes(resID)) {
			dalError("Failed to resolve '{}' in fopen."_format(resID.makeIDStr()));
			return std::unique_ptr<IResourceStream>{ nullptr };
		}

	finishResolve:

#if defined(_WIN32)
		std::string filePath;
		if (PACKAGE_NAME_ASSET == resID.getPackage()) {
			filePath = getResourceDir_win() + PACKAGE_NAME_ASSET + '/' + resID.makeFilePath();
		}
		else if (LOG_FOLDER_NAME == resID.getPackage()) {
			filePath = getResourceDir_win() + LOG_FOLDER_NAME + '/' + resID.makeFilePath();
			assertDir_log();
		}
		else {
			filePath = getResourceDir_win() + USERDATA_FOLDER_NAME + '/' + resID.getPackage() + '/' + resID.makeFilePath();
			assertDir_userdata();
			assertDir((getResourceDir_win() + USERDATA_FOLDER_NAME + '/' + resID.getPackage()).c_str());
		}

		std::unique_ptr<IResourceStream> file{ new STDFileStream };
		if (false == file->open(filePath.c_str(), mode)) {
			dalError("Failed to open file: "s + filePath);
			return std::unique_ptr<IResourceStream>{ nullptr };
		}

		return file;
#elif defined(__ANDROID__)
		std::string filePath;
		std::unique_ptr<IResourceStream> file;

		if (PACKAGE_NAME_ASSET == resID.getPackage()) {
			filePath = resID.makeFilePath();
			file.reset(new AssetSteam);
		}
		else if (LOG_FOLDER_NAME == resID.getPackage()) {
			filePath = g_storagePath + LOG_FOLDER_NAME + '/' + resID.makeFilePath();
			assertDir_log();
			file.reset(new STDFileStream);
		}
		else {
			filePath = g_storagePath + USERDATA_FOLDER_NAME + '/' + resID.getPackage() + '/' + resID.makeFilePath();
			assertDir_userdata();
			assertDir((g_storagePath + USERDATA_FOLDER_NAME + '/' + resID.getPackage()).c_str());
			file.reset(new STDFileStream);
		}

		if (!file->open(filePath.c_str(), mode)) {
			dalError("Failed to open file: "s + resID.makeIDStr());
			return std::unique_ptr<IResourceStream>{ nullptr };
		}

		return file;
#endif
		
	}

}