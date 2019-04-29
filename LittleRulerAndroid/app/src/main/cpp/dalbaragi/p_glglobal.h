#pragma once

#include "p_dalopengl.h"


namespace dal {
	namespace GLSwitch {

		void setOnlyOnce(void);

		void setFor_generalRender(void);
		void setFor_fillingScreen(void);
		void setFor_shadowmap(void);
		void setFor_overlay(void);

	}
}