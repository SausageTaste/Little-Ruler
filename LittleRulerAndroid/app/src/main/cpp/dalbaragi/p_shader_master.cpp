#include "p_shader_master.h"

#include "u_fileclass.h"


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

}


// Shader Master
namespace dal {

	ShaderMaster::ShaderMaster(void)
	:	m_general("shader_general"),
		m_depthmap("shader_fscreen"),
		m_fscreen("shader_depthmap"),
		m_overlay("shader_overlay")
	{
		// Compile shaders general
		{
			std::string vertSrc, fragSrc;
			futil::getRes_text("glsl/general_v.glsl", vertSrc);
			futil::getRes_text("glsl/general_f.glsl", fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_general.attachShader(verShader);
			this->m_general.attachShader(fragShader);
			this->m_general.link();
			this->m_generalUniloc.init(this->m_general.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		// Compile shaders fill screen
		{
			std::string vertSrc, fragSrc;
			futil::getRes_text("glsl/fillscreen_v.glsl", vertSrc);
			futil::getRes_text("glsl/fillscreen_f.glsl", fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_fscreen.attachShader(verShader);
			this->m_fscreen.attachShader(fragShader);
			this->m_fscreen.link();
			this->m_fscreenUniloc.init(this->m_fscreen.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		// Compile shaders depth map
		{
			std::string vertSrc, fragSrc;
			futil::getRes_text("glsl/depth_v.glsl", vertSrc);
			futil::getRes_text("glsl/depth_f.glsl", fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_depthmap.attachShader(verShader);
			this->m_depthmap.attachShader(fragShader);
			this->m_depthmap.link();
			this->m_depthmapUniloc.init(this->m_depthmap.get());

			glDeleteShader(verShader);
			glDeleteShader(fragShader);
		}

		/* Compile shaders overlay */ {
			std::string vertSrc, fragSrc;
			futil::getRes_text("glsl/overlay_v.glsl", vertSrc);
			futil::getRes_text("glsl/overlay_f.glsl", fragSrc);

			auto verShader = compileShader(ShaderType::VERTEX, vertSrc.c_str());
			auto fragShader = compileShader(ShaderType::FRAGMENT, fragSrc.c_str());

			this->m_overlay.attachShader(verShader);
			this->m_overlay.attachShader(fragShader);
			this->m_overlay.link();
			this->m_overlayUniloc.init(this->m_overlay.get());

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

}