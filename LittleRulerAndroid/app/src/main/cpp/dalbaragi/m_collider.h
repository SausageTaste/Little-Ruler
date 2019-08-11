#pragma once

#include <array>

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

        glm::vec3 getPoint000(void) const {
            return this->m_p1;
        }
        glm::vec3 getPoint111(void) const {
            return this->m_p2;
        }

        // The order is
        // 000, 001, 010, 011, 100, 101, 110, 111
        // Each digit means x, y, z, 0 means lower value on the axis, 1 means higher.
        std::array<glm::vec3, 8> getAllPoints(void) const;

        void set(const glm::vec3& p1, const glm::vec3& p2);
        void add(const glm::vec3& offset);

        friend bool checkCollision(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);
        friend CollisionResolveInfo calcResolveInfo(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);

    private:
        void validateOrder(void);

    };


    class Plane {

    private:
        glm::vec4 m_coeff;

    public:
        Plane(void);

        void set(const float a, const float b, const float c, const float d) {
            this->m_coeff.x = a;
            this->m_coeff.y = b;
            this->m_coeff.z = c;
            this->m_coeff.w = d;
        }

        glm::vec3 getNormal(void) const {
            return glm::vec3{ this->m_coeff.x, this->m_coeff.y, this->m_coeff.z };
        }

        float getSignedDist(const glm::vec3 v) const {
            const auto numerator = glm::dot(this->m_coeff, glm::vec4{v, 1.0f});
            const auto denominatorInv = glm::inversesqrt(this->m_coeff.x * this->m_coeff.x);
            return numerator * denominatorInv;
        }
        bool isInFront(const glm::vec3 v) const {
            return 0.0f < glm::dot(this->m_coeff, glm::vec4{v, 1.0f});
        }

    };

}


namespace dal {

    bool checkCollision(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);

    bool checkCollision(const AxisAlignedBoundingBox& aabb, const Plane& plane);

    CollisionResolveInfo calcResolveInfo(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other);

}