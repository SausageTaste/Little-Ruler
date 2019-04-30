#include "p_texture.h"

#include <string>
#include <cassert>


#include "s_logger_god.h"
#include "s_threader.h"
#include "u_fileclass.h"


using namespace std::string_literals;


namespace dal {

	Texture::~Texture(void) {
		if (mTexID != 0) {
			this->deleteTex();
		}
	}

	void Texture::init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		mWidth = width;
		mHeight = height;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		this->genTexture("init_diffueMap");

		glBindTexture(GL_TEXTURE_2D, mTexID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::init_diffueMap3(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		mWidth = width;
		mHeight = height;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		this->genTexture("init_diffueMap");

		glBindTexture(GL_TEXTURE_2D, mTexID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::init_depthMap(const unsigned int width, const unsigned int height) {
		mWidth = width;
		mHeight = height;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);

		this->genTexture("init_depthMap");

		glBindTexture(GL_TEXTURE_2D, mTexID); {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
		} glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		this->genTexture("init_maskMap");
		mWidth = width;
		mHeight = height;

		glBindTexture(GL_TEXTURE_2D, mTexID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void Texture::deleteTex(void) {
		glDeleteTextures(1, &this->mTexID);
		this->mTexID = 0;
	}

	void Texture::sendUniform(const GLint uniloc_sampler, const unsigned int index) const {
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_2D, this->mTexID);
		glUniform1i(uniloc_sampler, index);
	}

	bool Texture::isInitiated(void) const {
		return this->mTexID != 0;
	}

	// Getters

	GLuint Texture::getTexID(void) {
		return mTexID;
	}

	unsigned int Texture::getWidth(void) const {
		return mWidth;
	}

	unsigned int Texture::getHeight(void) const {
		return mHeight;
	}

	void Texture::genTexture(const char* const str4Log) {
		glGenTextures(1, &mTexID);
		if (mTexID == 0) {
			dal::LoggerGod::getinst().putFatal("Failed to init dal::Texture::init_depthMap::"s + str4Log);
			throw -1;
		}
	}

}


namespace dal {

	TextureHandle::TextureHandle(void) : m_tex(nullptr) {

	}

	TextureHandle::~TextureHandle(void) {

	}

	void TextureHandle::sendUniform(const GLint uniloc_sampler, const GLint uniloc_hasTex, const unsigned int index) const {
		if (m_tex != nullptr && m_tex->isInitiated()) {
			glUniform1i(uniloc_hasTex, 1);
			this->m_tex->sendUniform(uniloc_sampler, index);
		}
		else {
			glUniform1i(uniloc_hasTex, 0);
		}
	}

	GLuint TextureHandle::getTexID(void) {
		if (m_tex == nullptr) {
			return 0;
		}
		else {
			return m_tex->getTexID();
		}
	}

}


namespace dal {

	TexLoadTask::TexLoadTask(const char* const texName)
	:	iTask("TexLoadTask"),
		m_texName(texName),
		m_width(0), m_height(0), m_pixSize(0),
		m_success(false)
	{

	}

	void TexLoadTask::start(void) {
		const auto path = "texture/"s + m_texName;
		auto res = dal::file::readImageFile(path.c_str(), &m_buf, &m_width, &m_height, &m_pixSize);
		m_success = res;
	}

}


namespace dal {

	TextureMaster::TextureMaster(void) {
		std::vector<uint8_t> buf;
		int w, h, pixSize;
		file::readImageFile("texture/grass1.png", &buf, &w, &h, &pixSize);
		m_nullDiffuse = std::make_shared<dal::TextureHandle>();
		m_nullDiffuse->m_tex = new dal::Texture();
		m_nullDiffuse->m_tex->init_diffueMap(buf.data(), w, h);

		m_nullTex = std::make_shared<dal::TextureHandle>();
	}

	TextureMaster::~TextureMaster(void) {

	}

	void TextureMaster::update(void) {
		if (!m_returnedTasks.empty()) {
			auto task = this->m_returnedTasks.back();
			this->m_returnedTasks.pop_back();

			this->precessReturnedTask(task);
			delete task;
		}
	}

	void TextureMaster::notify(iTask* const task) {
		if (g_sentTasks_texLoad.find(task) != g_sentTasks_texLoad.end()) {
			g_sentTasks_texLoad.erase(task);

			auto loaded = reinterpret_cast<TexLoadTask*>(task);
			if (!loaded->m_success) {
				LoggerGod::getinst().putFatal("Failed to load Texture: "s + loaded->m_texName);
				throw - 1;
			}
			else {
				m_returnedTasks.push_back(loaded);
			}
		}
		else {
			LoggerGod::getinst().putFatal("Not registered task revieved in TextureMaster::notify.");
			throw -1;
		}
	}

	TextureHandle_ptr TextureMaster::getNull_diffuseMap(void) {
		return m_nullTex;
	}

	TextureHandle_ptr TextureMaster::getNull_maskMap(void) {
		return m_nullTex;
	}

	TextureHandle_ptr TextureMaster::request_diffuseMap(const char* const texName) {
		auto res = m_diffuseMaps.find(texName);
		if (res == m_diffuseMaps.end()) {
			auto texIter = this->m_diffuseMaps.emplace( texName, new dal::TextureHandle() );
			auto& newTexHandlePtr = texIter.first->second;

			/* Make task */ {
				auto task = new TexLoadTask(texName);
				g_sentTasks_texLoad.insert(task);
				assert(g_sentTasks_texLoad.find((void*)task) != g_sentTasks_texLoad.end());  // Just because I dont have confidence.

				TaskGod::getinst().orderTask(task, this);
			}
			
			return newTexHandlePtr;
		}
		else {
			return res->second;
		}
	}

	TextureHandle_ptr TextureMaster::request_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height) {
		auto handle = std::make_shared<dal::TextureHandle>();
		handle->m_tex = new dal::Texture();
		handle->m_tex->init_maskMap(image, width, height);
		return handle;
	}

	TextureHandle_ptr TextureMaster::request_depthMap(const unsigned int width, const unsigned int height) {
		auto handle = std::make_shared<dal::TextureHandle>();
		handle->m_tex = new dal::Texture();
		handle->m_tex->init_depthMap(width, height);
		return handle;
	}

	// Static

	TextureHandle_ptr TextureMaster::getNullTex(void) {
		static auto nullTex = std::make_shared<dal::TextureHandle>();
		return nullTex;
	}

	// Private

	void TextureMaster::precessReturnedTask(TexLoadTask* const task) {
		auto loadedTex = new dal::Texture();
		if (task->m_pixSize == 3) {
			loadedTex->init_diffueMap3(task->m_buf.data(), task->m_width, task->m_height);
		}
		else if (task->m_pixSize == 4) {
			loadedTex->init_diffueMap(task->m_buf.data(), task->m_width, task->m_height);
		}
		else {
			LoggerGod::getinst().putError(
				"Unknown pixel size: "s + task->m_texName + ", "s + std::to_string(task->m_pixSize)
			);
			delete loadedTex;
			return;
		}

		for (auto& handle : m_diffuseMaps) {
			const auto& name = handle.first;
			if (name == task->m_texName) {
				handle.second->m_tex = loadedTex;
			}
		}
	}

}