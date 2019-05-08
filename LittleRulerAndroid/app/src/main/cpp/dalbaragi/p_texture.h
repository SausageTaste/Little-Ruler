#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <unordered_set>

#include "p_dalopengl.h"
#include "s_threader.h"
#include "u_fileclass.h"


namespace dal {

	class TextureMaster;


	class Texture {

		//////// Attribs ////////

	private:
		GLuint m_texID = 0;
		unsigned int mWidth = 0, mHeight = 0;

		//////// Methods ////////

	public:
		void init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
		void init_diffueMap3(const uint8_t* const image, const unsigned int width, const unsigned int height);
		void init_depthMap(const unsigned int width, const unsigned int height);
		void init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
		void deleteTex(void);

		void sendUniform(const GLint uniloc_sampler, const unsigned int index) const;
		bool isInitiated(void) const;

		// Getters

		GLuint getTexID(void);
		unsigned int getWidth(void) const;
		unsigned int getHeight(void) const;

	private:
		void genTexture(const char* const str4Log);

	};


	class TextureHandle {

		//////// Definitions ////////

	public:
		friend class dal::TextureMaster;

		//////// Attribs ////////

	private:
		Texture* m_tex;

		//////// Methods ////////

	public:
		TextureHandle(void);
		~TextureHandle(void);
		void sendUniform(const GLint uniloc_sampler, const GLint uniloc_hasTex, const unsigned int index) const;

		GLuint getTexID(void);

	};


	class TexLoadTask : public dal::ITask {

		//////// Attribs ////////

	public:
		std::string m_texName;
		loadedinfo::ImageFileData m_data;
		bool m_success;

		//////// Methods ////////

		TexLoadTask(const char* const texName);

		virtual void start(void) override;

	};


	using TextureHandle_ptr = std::shared_ptr<dal::TextureHandle>;


	class TextureMaster : public ITaskDoneListener {

	private:
		
		//////// Attribs ////////

	private:
		std::unordered_map<std::string, TextureHandle_ptr> m_diffuseMaps;

		std::unordered_set<void*> g_sentTasks_texLoad;
		std::vector<TexLoadTask*> m_returnedTasks;

		TextureHandle_ptr m_nullDiffuse;
		TextureHandle_ptr m_nullMask;
		TextureHandle_ptr m_nullTex;

		//////// Methods ////////

	public:
		TextureMaster(void);
		virtual ~TextureMaster(void) override;
		TextureMaster(TextureMaster&) = delete;
		TextureMaster& operator=(TextureMaster&) = delete;

		void update(void);

		virtual void notifyTask(ITask* const task) override;

		TextureHandle_ptr getNull_diffuseMap(void);
		TextureHandle_ptr getNull_maskMap(void);

		TextureHandle_ptr request_diffuseMap(const char* const texName);
		TextureHandle_ptr request_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
		TextureHandle_ptr request_depthMap(const unsigned int width, const unsigned int height);

		static TextureHandle_ptr getNullTex(void);

	private:
		void precessReturnedTask(TexLoadTask* const task);

	};

}