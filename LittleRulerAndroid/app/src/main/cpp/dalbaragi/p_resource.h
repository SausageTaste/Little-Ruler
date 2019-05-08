#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "u_loadinfo.h"
#include "p_meshStatic.h"
#include "p_texture.h"
#include "u_pool.h"
#include "p_uniloc.h"


namespace dal {

	struct Model;
	class Package;
	class ResourceMaster;


	class TextureHandle2 {

	private:
		struct Pimpl;
		Pimpl* pimpl = nullptr;

	public:
		TextureHandle2(void);
		TextureHandle2(const char* const texID, Texture* const texture);
		TextureHandle2(const TextureHandle2& other);
		TextureHandle2(TextureHandle2&& other) noexcept;
		TextureHandle2& operator=(const TextureHandle2& other);
		TextureHandle2& operator=(TextureHandle2&& other) noexcept;
		~TextureHandle2(void);

		bool isReady(void) const;
		void sendUniform(const GLint uniloc_sampler, const GLint uniloc_hasTex, const unsigned int index) const;
		void destroyTexture(void);

	};


	class Material2 {

	public:
		float m_shininess = 32.0f;
		float m_specularStrength = 1.0f;
		glm::vec3 m_diffuseColor{ 1.0f, 1.0f, 1.0f };

	private:
		glm::vec2 m_texScale{ 1.0f, 1.0f };
		TextureHandle2 m_diffuseMap;

	public:
		// If paremeter value is 0, old value remains.
		void setTexScale(float x, float y);
		void setDiffuseMap(TextureHandle2 tex);

		void sendUniform(const UnilocGeneral& uniloc) const;

	};


	class ModelHandle {

	private:
		struct Pimpl;
		Pimpl* pimpl = nullptr;

	public:
		ModelHandle(void);
		ModelHandle(const char* const modelID, Model* const model);
		ModelHandle(const ModelHandle& other);
		ModelHandle(ModelHandle&& other) noexcept;
		ModelHandle& operator=(const ModelHandle& other);
		ModelHandle& operator=(ModelHandle&& other) noexcept;
		~ModelHandle(void);

		bool isReady(void) const;

		void renderGeneral(const UnilocGeneral& uniloc, const std::list<Actor>& actors) const;
		void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<Actor>& actors) const;

		void destroyModel(void);
		Model* replace(Model* model);
		std::string replace(const std::string& model);

	};


	class Package {

	private:
		std::string m_name;
		std::unordered_map<std::string, ModelHandle> m_models;
		std::unordered_map<std::string, TextureHandle2> m_textures;

	public:
		void setName(const char* const packageName);
		void setName(const std::string& packageName);

		ModelHandle orderModel(const ResourceFilePath& resPath, ResourceMaster* const resMas);
		ModelHandle buildModel(const loadedinfo::ModelDefined& info);
		TextureHandle2 orderDiffuseMap(const char* const texID);

		void clear(void);

	private:
		TextureHandle2 buildDiffuseMap(const char* const texID, const loadedinfo::ImageFileData& info);

	};


	class ResourceMaster : public ITaskDoneListener {

		//////// Attribs ////////

	private:
		std::unordered_map<std::string, Package> m_packages;

		//////// Methods ////////

	public:
		virtual ~ResourceMaster(void) override;

		virtual void notifyTask(ITask* const task) override;

		ModelHandle orderModel(const char* const packageName_dir_modelID);

		ModelHandle buildModel(const loadedinfo::ModelDefined& info, const char* const packageName);

		static TextureHandle2 getDepthMap(const unsigned int width, const unsigned int height);
		static TextureHandle2 getMaskMap(const uint8_t* const buf, const unsigned int width, const unsigned int height);

	private:
		Package& orderPackage(const std::string& packName);

	};

}