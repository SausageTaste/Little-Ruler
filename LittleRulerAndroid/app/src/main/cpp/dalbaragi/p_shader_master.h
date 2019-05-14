#pragma once

#include "p_shader.h"
#include "p_uniloc.h"


namespace dal {
	
	class ShaderMaster {

	private:
		ShaderProgram m_general;
		UnilocGeneral m_generalUniloc;

		ShaderProgram m_depthmap;
		UnilocDepthmp m_depthmapUniloc;

		ShaderProgram m_fscreen;
		UnilocFScreen m_fscreenUniloc;

		ShaderProgram m_overlay;
		UnilocOverlay m_overlayUniloc;

	public:
		ShaderMaster(void);

		void useGeneral(void) const;
		void useDepthMp(void) const;
		void useFScreen(void) const;
		void useOverlay(void) const;

		const UnilocGeneral& getGeneral(void) const;
		const UnilocDepthmp& getDepthMp(void) const;
		const UnilocFScreen& getFScreen(void) const;
		const UnilocOverlay& getOverlay(void) const;

	};

}