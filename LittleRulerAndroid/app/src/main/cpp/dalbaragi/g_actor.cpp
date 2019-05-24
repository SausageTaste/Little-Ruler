#include "g_actor.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

	glm::mat4 Camera::makeViewMat(void) const {
		auto viewMat = glm::mat4(1.0f);

		viewMat = glm::rotate(viewMat, this->m_viewDirec.y, glm::vec3(-1.0f, 0.0f, 0.0f));
		viewMat = glm::rotate(viewMat, this->m_viewDirec.x, glm::vec3(0.0f, 1.0f, 0.0f));
		viewMat = glm::translate(viewMat, glm::vec3(-this->m_pos.x, -this->m_pos.y, -this->m_pos.z));

		return viewMat;
	}


	glm::vec3 Camera::getPos(void) const {
		return this->m_pos;
	}

	void Camera::setPos(const float x, const float y, const float z) {
		this->m_pos.x = x;
		this->m_pos.y = y;
		this->m_pos.z = z;
	}

	void Camera::setPos(const glm::vec3& pos) {
		this->m_pos = pos;
	}

	void Camera::addPos(const float x, const float y, const float z) {
		this->m_pos.x += x;
		this->m_pos.y += y;
		this->m_pos.z += z;
	}

	void Camera::addPos(const glm::vec3& pos) {
		this->m_pos += pos;
	}


	glm::vec2 Camera::getViewPlane(void) const {
		return this->m_viewDirec;
	}

	void Camera::setViewPlane(const float x, const float y) {
		this->m_viewDirec.x = x;
		this->m_viewDirec.y = y;
	}


	void Camera::addViewPlane(const float x, const float y) {
		this->m_viewDirec.x += x;
		this->m_viewDirec.y += y;

		this->clampViewDir();
	}

	void Camera::clampViewDir(void) {
		constexpr auto plus90Degree = glm::radians(90.0f);
		constexpr auto minus90Degree = glm::radians(-90.0f);

		if (this->m_viewDirec.y > plus90Degree) {
			this->m_viewDirec.y = plus90Degree;
		}
		else if (this->m_viewDirec.y < minus90Degree) {
			this->m_viewDirec.y = minus90Degree;
		}
	}

}


namespace dal {
	
	glm::mat4 ActorInfo::getViewMat(void) const {
		//auto scaleMat = glm::scale(glm::mat4{ 1.0f }, { rescale, rescale , rescale });
		auto translateMat = glm::translate(glm::mat4{ 1.0f }, this->pos);
		return translateMat * glm::mat4_cast(myQuat); // *scaleMat;
	}

	void ActorInfo::rotate(const float v, const glm::vec3& selector) {
		this->myQuat = glm::angleAxis(v, selector) * this->myQuat;
		this->myQuat = glm::normalize(this->myQuat);
	}

}