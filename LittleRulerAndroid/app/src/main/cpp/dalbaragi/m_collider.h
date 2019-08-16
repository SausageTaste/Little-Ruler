#pragma once

#include <array>
#include <optional>

#include <glm/glm.hpp>


namespace dal {

    struct CollisionResolveInfo {
        glm::vec3 m_this, m_other;
    };

    struct RayCastingResult {
        bool m_isFromFront;
        float m_distance;
    };

}


namespace dal {

    class AABB {

    private:
        glm::vec3 m_p1, m_p2;
        float m_massInv = 1.0f;

    public:
        AABB(void) = default;
        AABB(const glm::vec3& p1, const glm::vec3& p2, const float massInv);

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
        void scale(const float mag);

        friend bool checkCollision(const AABB& one, const AABB& other);
        friend CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other,
            const float oneMassInv, const float otherMassInv);

        float calcArea(void) const;

    private:
        void validateOrder(void);
        
    };


    class Plane {

    private:
        glm::vec4 m_coeff;

    public:
        Plane(void);
        Plane(const glm::vec3& normal, const glm::vec3& point);
        Plane(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
        Plane(const float a, const float b, const float c, const float d);

        glm::vec3 getNormal(void) const {
            return glm::vec3{ this->m_coeff.x, this->m_coeff.y, this->m_coeff.z };
        }

        float getSignedDist(const glm::vec3 v) const {
            const auto numerator = glm::dot(this->m_coeff, glm::vec4{v, 1.0f});
            const auto denominatorInv = glm::inversesqrt(
                this->m_coeff.x * this->m_coeff.x + this->m_coeff.y * this->m_coeff.y  + this->m_coeff.z * this->m_coeff.z
            );
            return numerator * denominatorInv;
        }
        bool isInFront(const glm::vec3 v) const {
            return 0.0f < glm::dot(this->m_coeff, glm::vec4{v, 1.0f});
        }

    };


    class Ray {

    private:
        glm::vec3 m_pos, m_rel;
        float m_len;

    public:
        Ray(void);
        Ray(const glm::vec3& pos, const glm::vec3& rel);

        const glm::vec3& getStartPos(void) const {
            return this->m_pos;
        }
        const glm::vec3& getRel(void) const {
            return this->m_rel;
        }
        float getLength(void) const {
            return this->m_len;
        }
        void setStartPos(const glm::vec3& v) {
            this->m_pos = v;
        }
        void setRel(const glm::vec3& v);

    };


    class Triangle {

    private:
        glm::vec3 m_points[3];

    public:
        Triangle(void) = default;
        Triangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

        const glm::vec3& getPoint1(void) const {
            return this->m_points[0];
        }
        const glm::vec3& getPoint2(void) const {
            return this->m_points[1];
        }
        const glm::vec3& getPoint3(void) const {
            return this->m_points[2];
        }

    private:
        float calcArea(void) const;

    };

}


namespace dal {

    bool checkCollision(const AABB& one, const AABB& other);
    bool checkCollision(const AABB& aabb, const Plane& plane);
    bool checkCollision(const Ray& ray, const Plane& plane);
    bool checkCollision(const Ray& ray, const AABB& aabb);
    bool checkCollision(const Ray& ray, const Triangle& tri);

    CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other,
        const float oneMassInv, const float otherMassInv);

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const Plane& plane);
    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const AABB& aabb);

}