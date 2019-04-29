#include "m_collision2d.h"


namespace dal {

	void AABB_2D::setPoints(const glm::vec2& p1, const glm::vec2& p2) {
		this->p1 = p1;
		this->p2 = p2;
		this->validateOrder();
	}

	bool AABB_2D::isInside(const glm::vec2& p) const {
		if (p.x < p1.x) return false;
		if (p.x > p2.x) return false;
		if (p.y < p1.y) return false;
		if (p.y > p2.y) return false;

		return true;
	}

	void AABB_2D::validateOrder(void) {
		if (p1.x > p2.x) {
			std::swap(p1.x, p2.x);
		}
		if (p1.y > p2.y) {
			std::swap(p1.y, p2.y);
		}
	}

}