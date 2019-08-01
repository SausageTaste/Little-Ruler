#pragma once

#include <glm/glm.hpp>


namespace dal {

    struct CollisionResolveInfo {
        glm::vec3 m_this, m_other;
    };

}


namespace dal {

    class AxisAlignedBoundingBox {

    private:
        glm::vec3 m_p1, m_p2;
        float m_massInv = 1.0f;

    public:
        AxisAlignedBoundingBox(void) = default;
        AxisAlignedBoundingBox(const glm::vec3& p1, const glm::vec3& p2, const float massInv);

        void set(const glm::vec3& p1, const glm::vec3& p2);
        void add(const glm::vec3& offset);

        friend bool checkCollision(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);
        friend CollisionResolveInfo calcResolveInfo(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);

    private:
        void validateOrder(void);

    };

}


namespace dal {

    bool checkCollision(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);

    CollisionResolveInfo calcResolveInfo(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);

}