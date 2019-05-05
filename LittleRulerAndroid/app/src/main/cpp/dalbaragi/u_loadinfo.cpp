#include "u_loadinfo.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

	glm::mat4 Actor::getViewMat(void) const {
		//auto scaleMat = glm::scale(glm::mat4{ 1.0f }, { rescale, rescale , rescale });
		auto translateMat = glm::translate(glm::mat4{ 1.0f }, this->pos);
		return translateMat * glm::mat4_cast(myQuat); // *scaleMat;
	}

	void Actor::rotate(const float v, const glm::vec3& selector) {
		this->myQuat = glm::angleAxis(v, selector) * this->myQuat;
		this->myQuat = glm::normalize(this->myQuat);
	}

}