#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "u_loadinfo.h"
#include "p_meshStatic.h"
#include "u_pool.h"
#include "p_uniloc.h"
#include "s_threader.h"
#include "u_fileclass.h"


namespace dal {

	struct Model;
	class Package;
	class Texture;
	class ResourceMaster;


	class TextureHandle2 {

	private:
		struct Pimpl;
		Pimpl* pimpl = nullptr;

	public:
		TextureHandle2(void);
		TextureHandle2(const std::string& texID, Texture* const texture);
		TextureHandle2(const TextureHandle2& other);
		TextureHandle2(TextureHandle2&& other) noexcept;
		TextureHandle2& operator=(const TextureHandle2& other);
		TextureHandle2& operator=(TextureHandle2&& other) noexcept;
		~TextureHandle2(void);

		bool isReady(void) const;
		unsigned int getRefCount(void) const;

		void sendUniform(const GLint uniloc_sampler, const GLint uniloc_hasTex, const unsigned int index) const;
		GLuint getTex(void);
		Texture* replace(Texture* const tex);
		void destroyTexture(void);

	};


	class Material {

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
		ModelHandle(const std::string& modelID, Model* const model);
		ModelHandle(const ModelHandle& other);
		ModelHandle(ModelHandle&& other) noexcept;
		ModelHandle& operator=(const ModelHandle& other);
		ModelHandle& operator=(ModelHandle&& other) noexcept;
		~ModelHandle(void);

		bool operator==(const ModelHandle& other) const;

		bool isReady(void) const;
		unsigned int getRefCount(void) const;

		void renderGeneral(const UnilocGeneral& uniloc, const std::list<Actor>& actors) const;
		void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<Actor>& actors) const;

		void destroyModel(void);
		Model* replace(Model* model);
		std::string replace(const std::string& model);

	};


	class Package {

	public:
		struct ResourceReport {
			std::string m_packageName;
			std::vector<std::pair<std::string, unsigned int>> m_models;
			std::vector<std::pair<std::string, unsigned int>> m_textures;

			void print(void) const;
		};

	private:
		std::string m_name;
		std::unordered_map<std::string, ModelHandle> m_models;
		std::unordered_map<std::string, TextureHandle2> m_textures;

	public:
		void setName(const char* const packageName);
		void setName(const std::string& packageName);

		ModelHandle orderModel(const ResourceID& resPath, ResourceMaster* const resMas);
		ModelHandle buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas);
		TextureHandle2 orderDiffuseMap(const ResourceID& texID, ResourceMaster* const resMas);

		void getResReport(ResourceReport& report) const;

		void clear(void);

	private:
		TextureHandle2 buildDiffuseMap(const ResourceID& texID, const loadedinfo::ImageFileData& info);

	};


	class ResourceMaster : public ITaskDoneListener {

		//////// Attribs ////////

	private:
		std::unordered_map<std::string, Package> m_packages;

		//////// Methods ////////

	public:
		virtual ~ResourceMaster(void) override;

		virtual void notifyTask(ITask* const task) override;

		ModelHandle orderModel(const ResourceID& resID);

		ModelHandle buildModel(const loadedinfo::ModelDefined& info, const char* const packageName);

		static TextureHandle2 getDepthMap(const unsigned int width, const unsigned int height);
		static TextureHandle2 getMaskMap(const uint8_t* const buf, const unsigned int width, const unsigned int height);

		size_t getResReports(std::vector<Package::ResourceReport>& reports) const;

	private:
		Package& orderPackage(const std::string& packName);

	};

}