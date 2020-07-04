#pragma once

#include <array>
#include <vector>
#include <variant>
#include <optional>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <d_geometrymath.h>
#include <d_transform.h>


// Structs
namespace dal {

    struct CollisionResolveInfo {
        glm::vec3 m_this, m_other;
        bool m_valid = false;
    };

    struct RayCastingResult {
        bool m_isFromFront = false;
        float m_distance = 0.0f;
    };


    class PhysicalProperty {

    private:
        float m_massInv = 0.0f;

    public:
        void setMassInv(const float v) noexcept {
            this->m_massInv = v;
        }
        float getMassInv(void) const noexcept {
            return this->m_massInv;
        }

    };

}


// For polymorphism
namespace dal {

    enum class ColliderType {
        sphere = 0, aabb, triangle_soup,
        eoe
    };


    class ICollider {

    private:
        ColliderType m_type;

    protected:
        ICollider(const ColliderType type);

    public:
        virtual ~ICollider(void) = default;
        ColliderType getColType(void) const noexcept;

    };

}


// Polymorphic colliders
namespace dal {

    class ColSphere : public Sphere, public ICollider {

    public:
        ColSphere(void)
            : ICollider(ColliderType::sphere)
        {

        }
        ColSphere(const glm::vec3& center, const float radius)
            : Sphere(center, radius)
            , ICollider(ColliderType::sphere)
        {

        }
        ColSphere(const Sphere& sphere)
            : Sphere(sphere)
            , ICollider(ColliderType::sphere)
        {

        }

    };


    class ColAABB : public AABB, public ICollider {

    public:
        ColAABB(void)
            : ICollider(ColliderType::aabb)
        {

        }
        ColAABB(const glm::vec3& p1, const glm::vec3& p2)
            : AABB(p1, p2)
            , ICollider(ColliderType::aabb)
        {

        }
        ColAABB(const AABB& aabb)
            : AABB(aabb)
            , ICollider(ColliderType::aabb)
        {

        }

    };

}


// Complex colliders
namespace dal {

    class ColTriangleSoup : public ICollider {

    private:
        std::vector<Triangle> m_triangles;
        bool m_faceCull = true;

    public:
        ColTriangleSoup(void)
            : ICollider(ColliderType::triangle_soup)
        {

        }

        const Triangle& operator[](const size_t index) const {
            return this->m_triangles[index];
        }
        const Triangle& at(const size_t index) const {
            return this->m_triangles.at(index);
        }
        size_t getSize(void) const {
            return this->m_triangles.size();
        }
        bool isFaceCullSet(void) const {
            return this->m_faceCull;
        }

        auto begin(void) const {
            return this->m_triangles.begin();
        }
        auto end(void) const {
            return this->m_triangles.end();
        }

        void addTriangle(const Triangle& tri) {
            this->m_triangles.push_back(tri);
        }
        Triangle& emplaceTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
            return this->m_triangles.emplace_back(p1, p2, p3);
        }
        void reserve(const size_t size) {
            this->m_triangles.reserve(size);
        }
        void resize(const size_t size) {
            this->m_triangles.resize(size);
        }
        auto data(void) {
            static_assert(sizeof(float) * 9 == sizeof(Triangle));
            return this->m_triangles.data();
        }

    };

}


// checkCollision funcs
namespace dal {

    bool checkCollisionAbs(const ICollider& one, const ICollider& two, const Transform& transOne, const Transform& transTwo);
    bool checkCollisionAbs(const Segment& ray, const ICollider& col, const Transform& transCol);

    bool checkCollision(const Segment& ray, const Plane& plane);
    bool checkCollision(const Segment& ray, const Sphere& sphere);
    bool checkCollision(const Segment& ray, const Sphere& sphere, const Transform& transSphere);
    bool checkCollision(const Segment& ray, const Triangle& tri);
    bool checkCollision(const Segment& ray, const AABB& aabb);
    bool checkCollision(const Segment& ray, const AABB& aabb, const Transform& transAABB);

    bool checkCollision(const Plane& plane, const Sphere& sphere);
    bool checkCollision(const Plane& plane, const Sphere& sphere, const Transform& transSphere);
    bool checkCollision(const Plane& plane, const AABB& aabb);
    bool checkCollision(const Plane& plane, const AABB& aabb, const Transform& transAABB);

    bool checkCollision(const Triangle& tri, const AABB& aabb, const Transform& transTri, const Transform& transAABB);

    bool checkCollision(const Sphere& sphere, const AABB& aabb, const Transform& transSphere, const Transform& transAABB);

    bool checkCollision(const AABB& one, const AABB& other);
    bool checkCollision(const AABB& one, const AABB& two, const Transform& transOne, const Transform& transTwo);
    bool checkCollision(const AABB& aabb, const ColTriangleSoup triSoup, const Transform& transAABB, const Transform& transTriSoup);

}


// calcResolveInfo funcs
namespace dal {

    CollisionResolveInfo calcResolveInfoABS(
        const ICollider& one, const PhysicalProperty& physicsOne, const Transform& transOne,
        const ICollider& two, const PhysicalProperty& physicsTwo, const Transform& transTwo
    );

    //CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other, const PhysicalProperty& physicsOne, const PhysicalProperty& physicsTwo);
    CollisionResolveInfo calcResolveInfo(
        const AABB& one, const PhysicalProperty& physicsOne, const Transform& transOne,
        const AABB& two, const PhysicalProperty& physicsTwo, const Transform& transTwo
    );

}


// calcCollisionInfo funcs
namespace dal {

    std::optional<RayCastingResult> calcCollisionInfoAbs(const Segment& ray, const ICollider& col, const Transform& transCol);

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const Triangle& tri, const bool ignoreFromBack = false);
    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const Sphere& sphere, const Transform& transSphere);
    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const Plane& plane);
    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const AABB& aabb);
    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const AABB& aabb, const Transform& transAABB);
    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const ColTriangleSoup triSoup, const Transform& transTriSoup);

}
