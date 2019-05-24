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
	class ResourceMaster;

	
	class Texture {

		//////// Attribs ////////

	private:
		GLuint m_texID = 0;

		//////// Methods ////////

	private:
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

	public:
		Texture(void) = default;
		Texture(const GLuint id);

		Texture(Texture&& other) noexcept;
		Texture& operator=(Texture&& other) noexcept;

		~Texture(void);

		void init_diffueMap(const uint8_t* const image, const unsigned int width, const unsigned int height);
		void init_diffueMap3(const uint8_t* const image, const unsigned int width, const unsigned int height);
		void init_depthMap(const unsigned int width, const unsigned int height);
		void init_maskMap(const uint8_t* const image, const unsigned int width, const unsigned int height);

		void deleteTex(void);
		void sendUniform(const GLint uniloc_sampler, const GLint uniloc_has, const unsigned int index) const;

		bool isReady(void) const;

		// Getters

		GLuint get(void);

	private:
		void genTexture(const char* const str4Log);

	};


	class Material {

	public:
		float m_shininess = 32.0f;
		float m_specularStrength = 1.0f;
		glm::vec3 m_diffuseColor{ 1.0f, 1.0f, 1.0f };

	private:
		glm::vec2 m_texScale{ 1.0f, 1.0f };
		Texture* m_diffuseMap = nullptr;

	public:
		// If paremeter value is 0, old value remains.
		void setTexScale(float x, float y);
		void setDiffuseMap(Texture* const tex);

		void sendUniform(const UnilocGeneral& uniloc) const;
		void sendUniform(const UnilocWaterry& uniloc) const;

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

		void renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const;
		void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const;

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
			std::string getStr(void) const;
		};

	private:
		struct TextureManageInfo {
			Texture* m_tex = nullptr;
			int64_t m_refCount = 0;
		};

	private:
		std::string m_name;
		std::unordered_map<std::string, ModelHandle> m_models;
		std::unordered_map<std::string, TextureManageInfo> m_textures;

	public:
		void setName(const char* const packageName);
		void setName(const std::string& packageName);

		ModelHandle orderModel(const ResourceID& resPath, ResourceMaster* const resMas);
		ModelHandle buildModel(const loadedinfo::ModelDefined& info, ResourceMaster* const resMas);
		Texture* orderDiffuseMap(const ResourceID& texID, ResourceMaster* const resMas);

		void getResReport(ResourceReport& report) const;

		void clear(void);

	private:
		Texture* buildDiffuseMap(const ResourceID& texID, const loadedinfo::ImageFileData& info);

	};


	class ResourceMaster : public ITaskDoneListener {

		//////// Attribs ////////

	private:
		std::unordered_map<std::string, Package> m_packages;

		//////// Methods ////////

	public:
		virtual ~ResourceMaster(void) override;

		virtual void notifyTask(std::unique_ptr<ITask> task) override;

		ModelHandle orderModel(const ResourceID& resID);

		ModelHandle buildModel(const loadedinfo::ModelDefined& info, const char* const packageName);

		static Texture* getUniqueTexture(void);
		static void dumpUniqueTexture(Texture* const tex);

		size_t getResReports(std::vector<Package::ResourceReport>& reports) const;

	private:
		Package& orderPackage(const std::string& packName);

	};

}