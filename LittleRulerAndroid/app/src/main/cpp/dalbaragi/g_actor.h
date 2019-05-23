#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

	class Camera {
	
	private:
		glm::vec2 m_viewDirec;
		glm::vec3 m_pos;

	public:
		glm::mat4 makeViewMat(void) const;

		glm::vec3 getPos(void) const;
		glm::vec2 getViewPlaneVec(void) const;

		void addPos(const float x, const float y, const float z);
		void addPos(const glm::vec3& pos);
		void addViewPlane(const float x, const float y);

	private:
		void clampViewDir(void);
	
	};

	
	struct ActorInfo {
		std::string m_actorID;
		glm::vec3 pos;
		glm::quat myQuat;
		bool m_static = true;

		glm::mat4 getViewMat(void) const;
		void rotate(const float v, const glm::vec3& selector);
	};

}