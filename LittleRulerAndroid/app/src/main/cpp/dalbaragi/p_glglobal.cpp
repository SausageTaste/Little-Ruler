#include "p_glglobal.h"


namespace dal {
	namespace GLSwitch {

		void setOnlyOnce(void) {
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		}

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
}