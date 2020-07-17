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

    class ColTriangleSoup : public ICollider, public dal::TriangleSoup {

    public:
        ColTriangleSoup(void)
            : ICollider(ColliderType::triangle_soup)
        {

        }

    };

}


// checkCollision funcs
namespace dal {

    bool checkCollisionAbs(const ICollider& one, const ICollider& two, const Transform& transOne, const Transform& transTwo);

    bool checkCollisionAbs(const Segment& seg, const ICollider& col, const Transform& transCol);

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

    CollisionResolveInfo calcResolveInfo(const AABB& aabb, const PhysicalProperty& physicsOne, const Transform& transOne,
        const dal::ColTriangleSoup& soup, const PhysicalProperty& physicsTwo, const Transform& transTwo);

}


// Resolve AABB
namespace dal {

    static glm::vec3 resolveAABB_abs(const dal::MovingAABBInfo& aabb, const dal::ICollider& other, const dal::Transform& trans);

}


// calcCollisionInfo funcs
namespace dal {

    std::optional<RayCastingResult> calcCollisionInfoAbs(const Segment& ray, const ICollider& col, const Transform& transCol);

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const ColTriangleSoup triSoup, const Transform& transTriSoup);

}
