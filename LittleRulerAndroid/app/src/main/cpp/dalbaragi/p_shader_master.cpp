#include "p_shader_master.h"

#include <array>
#include <unordered_map>

#include <fmt/format.h>

#include "u_fileclass.h"
#include "s_logger_god.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

	void setFor_generalRender(void) {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void setFor_fillingScreen(void) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void setFor_shadowmap(void) {
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		//glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(4.0f, 100.0f);

	}

	void setFor_overlay(void) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void setFor_water(void) {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

}


namespace {

	GLuint compileShader2(dal::ShaderType type, const char* const src) {
		// Returns 0 on error
		GLuint shaderID = 0;

		switch ( type ) {
		case dal::ShaderType::VERTEX:
			shaderID = glCreateShader(GL_VERTEX_SHADER);
			break;
		case dal::ShaderType::FRAGMENT:
			shaderID = glCreateShader(GL_FRAGMENT_SHADER);
			break;
		}

		if ( shaderID == 0 ) {
			dalAbort("Failed to create shader object.");
		}

		glShaderSource(shaderID, 1, &src, NULL);
		glCompileShader(shaderID);

		GLint vShaderCompiled = GL_FALSE;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &vShaderCompiled);
		if ( vShaderCompiled != GL_TRUE ) {
			GLsizei length = 0;
			char log[1024];
			glGetShaderInfoLog(shaderID, 1024, &length, log);
			dalAbort("Shader primitive compile error. Error message from OpenGL is\n"s + log + "\nshader source code is\n" + src + '\n');
		}

		return shaderID;
	}

}



namespace {

	class ShaderLoader {

	private:
		enum class ShaderType { vert, frag };
		enum class Precision { nodef, high, mediump };
		enum class Defined { parse_fail, ignore_this, include };

	private:
		std::unordered_map<std::string, std::string> m_soures;

	public:
		ShaderLoader(void) {
			constexpr std::array<const char*, 10> k_fileNames = {
				"depth.vert",
				"depth.frag",
				"fillscreen.vert",
				"fillscreen.frag",
				"general.frag",
				"general.vert",
				"overlay.vert",
				"overlay.frag",
				"water.vert",
				"water.frag",
			};

			for ( const auto fileName : k_fileNames ) {
				this->orderShaderSrc(fileName);
			}

			for ( auto& spair : m_soures ) {
				const auto shaderType = determineShaderType(spair.first);

				switch ( shaderType ) {
				case ShaderType::vert:
					spair.second = makeHeader(Precision::nodef) + spair.second;
					break;
				case ShaderType::frag:
#if defined(_WIN32)
					spair.second = makeHeader(Precision::nodef) + spair.second;
#elif defined(__ANDROID__)
					spair.second = makeHeader(Precision::mediump) + spair.second;
#endif
					break;
				}
			}
		}

		const char* const operator[](const std::string& key) {
			const auto iter = this->m_soures.find(key);
			if ( this->m_soures.end() == iter ) dalAbort("\'{}\' not exist in ShaderLoader."_format(key));
			return iter->second.c_str();
		}

	private:
		std::string preprocess(std::string src) {
			size_t lastTail = 0;
			while ( true ) {
				const auto head = src.find("#", lastTail);
				if ( std::string::npos == head ) break;
				auto tail = src.find("\n", head);
				if ( std::string::npos == tail ) tail = src.size() - 1;

				std::vector<std::string> args;
				const auto resDef = this->parseDefine(src.substr(head, tail - head), args);
				if ( Defined::include == resDef ) {
					const auto& toInclude = this->orderShaderSrc(args[0]);
					src = src.substr(0, head) + toInclude + src.substr(tail, src.size() - tail);
					lastTail = head + toInclude.size() + 1;
				}
				else if ( Defined::ignore_this ==resDef ) {
					lastTail = tail + 1;
					continue;
				}
				else if ( Defined::parse_fail == resDef ) {
					dalAbort("Error during preprocessing");
				}
			}

			return src;
		}

		const std::string& orderShaderSrc(const std::string& fileName) {
			auto found = this->m_soures.find(fileName);
			if ( this->m_soures.end() == found ) {
				auto file = dal::resopen("asset::glsl/"s + fileName, dal::FileMode::read);
				if ( nullptr == file ) dalAbort("Failed to load shader source file: "s + fileName);

				auto iter = this->m_soures.emplace(fileName, "");
				if ( false == iter.second ) {
					dalAbort("Failed to add element into ShaderLoader::m_soures.");
				}
				auto& buffer = iter.first->second;
				if ( !file->readText(buffer) ) dalAbort("Failed to read shader source file: "s + fileName);

				buffer = this->preprocess(buffer);
				return buffer;
			}
			else {
				return found->second;
			}
		}

		// Static

		static Defined parseDefine(const std::string& text, std::vector<std::string>& results) {
			{
				const auto head = text.find("include");
				if ( std::string::npos != head ) {
					const auto argHead = text.find('<', head);
					const auto argTail = text.find('>', argHead);
					if ( std::string::npos ==  argHead || std::string::npos ==argTail ) {
						dalError("Error during parsing #include: "s + text);
						return Defined::parse_fail;
					}

					const auto fileToInclude = text.substr(argHead + 1, argTail - argHead - 1);
					results.clear();
					results.emplace_back(fileToInclude);

					return Defined::include;
				}
			}

			{
				const auto head = text.find("#ifdef GL_ES");
				if ( std::string::npos != head ) return Defined::ignore_this;
			}
			{
				const auto head = text.find("#endif");
				if ( std::string::npos != head ) return Defined::ignore_this;
			}
			{
				const auto head = text.find("#else");
				if ( std::string::npos != head ) return Defined::ignore_this;
			}
			{
				const auto head = text.find("#ifndef GL_ES");
				if ( std::string::npos != head ) return Defined::ignore_this;
			}

			dalError("Unknown define in shader: "s + text);
			return Defined::parse_fail;
		}

		static ShaderType determineShaderType(const std::string& fileName) {
			if ( std::string::npos != fileName.find(".vert") ) {
				return ShaderType::vert;
			}
			else if ( std::string::npos != fileName.find(".frag") ) {
				return ShaderType::frag;
			}
			else {
				dalAbort("Can't determine shader type for: "s + fileName);
			}
		}

		static std::string makeHeader(const Precision precision) {
#if defined(_WIN32)
			std::string fileHeader = "#version 330 core\n";
#elif defined(__ANDROID__)
			std::string fileHeader = "#version 300 es\n";
#endif
			switch ( precision ) {
			case Precision::nodef:
				break;
			case Precision::high:
				fileHeader += "precision highp float;\n";
				break;
			case Precision::mediump:
				fileHeader += "precision mediump float;\n";
				break;
			}

			return fileHeader;
		}

	};

}


// Shader Master
namespace dal {

	ShaderMaster::ShaderMaster(void)
	:	m_general("shader_general"),
		m_depthmap("shader_fscreen"),
		m_fscreen("shader_depthmap"),
		m_overlay("shader_overlay"),
		m_waterry("shader_water")
	{
		ShaderLoader loader;

		{
			auto verShader = compileShader2(ShaderType::VERTEX, loader["general.vert"]);
			auto fragShader = compileShader2(ShaderType::FRAGMENT, loader["general.frag"]);

			this->m_general.attachShader(verShader);
			this->m_general.attachShader(fragShader);
			this->m_general.link();
			this->m_generalUniloc.init(this->m_general.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		{
			auto verShader = compileShader2(ShaderType::VERTEX, loader["fillscreen.vert"]);
			auto fragShader = compileShader2(ShaderType::FRAGMENT, loader["fillscreen.frag"]);

			this->m_fscreen.attachShader(verShader);
			this->m_fscreen.attachShader(fragShader);
			this->m_fscreen.link();
			this->m_fscreenUniloc.init(this->m_fscreen.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		{
			auto verShader = compileShader2(ShaderType::VERTEX, loader["depth.vert"]);
			auto fragShader = compileShader2(ShaderType::FRAGMENT, loader["depth.frag"]);

			this->m_depthmap.attachShader(verShader);
			this->m_depthmap.attachShader(fragShader);
			this->m_depthmap.link();
			this->m_depthmapUniloc.init(this->m_depthmap.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		{
			auto verShader = compileShader2(ShaderType::VERTEX, loader["overlay.vert"]);
			auto fragShader = compileShader2(ShaderType::FRAGMENT, loader["overlay.frag"]);

			this->m_overlay.attachShader(verShader);
			this->m_overlay.attachShader(fragShader);
			this->m_overlay.link();
			this->m_overlayUniloc.init(this->m_overlay.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		{
			auto verShader = compileShader2(ShaderType::VERTEX, loader["water.vert"]);
			auto fragShader = compileShader2(ShaderType::FRAGMENT, loader["water.frag"]);

			this->m_waterry.attachShader(verShader);
			this->m_waterry.attachShader(fragShader);
			this->m_waterry.link();
			this->m_waterryUniloc.init(this->m_waterry.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}
	}

	void ShaderMaster::useGeneral(void) const {
		setFor_generalRender();
		this->m_general.use();
	}

	void ShaderMaster::useDepthMp(void) const {
		setFor_shadowmap();
		this->m_depthmap.use();
	}

	void ShaderMaster::useFScreen(void) const {
		setFor_fillingScreen();
		this->m_fscreen.use();
	}

	void ShaderMaster::useOverlay(void) const {
		setFor_overlay();
		this->m_overlay.use();
	}

	void ShaderMaster::useWaterry(void) const {
		setFor_water();
		this->m_waterry.use();
	}

	const UnilocGeneral& ShaderMaster::getGeneral(void) const {
		return this->m_generalUniloc;
	}

	const UnilocDepthmp& ShaderMaster::getDepthMp(void) const {
		return this->m_depthmapUniloc;
	}

	const UnilocFScreen& ShaderMaster::getFScreen(void) const {
		return this->m_fscreenUniloc;
	}

	const UnilocOverlay& ShaderMaster::getOverlay(void) const {
		return this->m_overlayUniloc;
	}

	const UnilocWaterry& ShaderMaster::getWaterry(void) const {
		return this->m_waterryUniloc;
	}

}