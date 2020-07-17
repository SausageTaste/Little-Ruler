#include "d_collider.h"

#include <tuple>

#include <u_math.h>
#include <d_debugview.h>


// Global constants
namespace {

    /*
    const glm::vec3 AABB_NORMALS[6] = {
        { -1.f,  0.f,  0.f },
        {  0.f, -1.f,  0.f },
        {  0.f,  0.f, -1.f },
        {  1.f,  0.f,  0.f },
        {  0.f,  1.f,  0.f },
        {  0.f,  0.f,  1.f }
    };
    */

}


// Util functions
namespace {

    size_t minValueIndex(const float* const arr, const size_t arrSize) {
        if ( 0 == arrSize ) {
            assert(false && "Are you mad?");
            return 0;
        }
        else if ( 1 == arrSize ) {
            return 0;
        }
        else {
            float minValue = arr[0];
            size_t minIndex = 0;

            for ( size_t i = 1; i < arrSize; i++ ) {
                if ( arr[i] < minValue ) {
                    minValue = arr[i];
                    minIndex = i;
                }
            }

            return minIndex;
        }
    }

    dal::CollisionResolveInfo calcResolveInfo_withMinMax(
        const glm::vec3& one1, const glm::vec3& one2,
        const glm::vec3& other1, const glm::vec3& other2,
        const float thisFactor, const float otherFactor
    ) {
        const auto xOne = one2.x - other1.x;
        const auto xTwo = one1.x - other2.x;
        const auto xDistance = abs(xOne) < abs(xTwo) ? xOne : xTwo;

        const auto yOne = one2.y - other1.y;
        const auto yTwo = one1.y - other2.y;
        const auto yDistance = abs(yOne) < abs(yTwo) ? yOne : yTwo;

        const auto zOne = one2.z - other1.z;
        const auto zTwo = one1.z - other2.z;
        const auto zDistance = abs(zOne) < abs(zTwo) ? zOne : zTwo;

        const auto xForThis = -xDistance * thisFactor;
        const auto yForThis = -yDistance * thisFactor;
        const auto zForThis = -zDistance * thisFactor;

        const auto xForOther = xDistance * otherFactor;
        const auto yForOther = yDistance * otherFactor;
        const auto zForOther = zDistance * otherFactor;

        const float selector[3] = { std::abs(xForThis), std::abs(yForThis), std::abs(zForThis) };
        switch ( minValueIndex(selector, 3) ) {

        case 0:
            return dal::CollisionResolveInfo{ { xForThis, 0.0f, 0.0f }, { xForOther, 0.0f, 0.0f }, true };
        case 1:
            return dal::CollisionResolveInfo{ { 0.0f, yForThis, 0.0f }, { 0.0f, yForOther, 0.0f }, true };
        case 2:
            return dal::CollisionResolveInfo{ { 0.0f, 0.0f, zForThis }, { 0.0f, 0.0f, zForOther }, true };
        default:
            assert(false && "This can't happen!");
            return dal::CollisionResolveInfo{};

        }
    }

}


// Collider resolver
namespace {

    class ColliderResolver {

    private:
        template <typename T>
        class ColFuncTable {

        private:
            static constexpr size_t NUM_COLLIDERS = static_cast<size_t>(dal::ColliderType::eoe);
            T m_array[NUM_COLLIDERS][NUM_COLLIDERS] = { 0 };

        public:
            auto get(const dal::ColliderType one, const dal::ColliderType two) const -> T {
                return this->m_array[this->indexof(one)][this->indexof(two)];
            }

            void set(const dal::ColliderType one, const dal::ColliderType two, T ptr) {
                this->m_array[this->indexof(one)][this->indexof(two)] = ptr;
            }

        private:
            static size_t indexof(const dal::ColliderType e) {
                return static_cast<unsigned int>(e);
            }

        };

    private:
        using checkColFunc_t = bool (*)(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo);
        using calcResolveFunc_t = dal::CollisionResolveInfo(*)(
            const dal::ICollider& one, const dal::PhysicalProperty& physicsOne, const dal::Transform& transOne,
            const dal::ICollider& two, const dal::PhysicalProperty& physicsTwo, const dal::Transform& transTwo
            );

    private:
        ColFuncTable<checkColFunc_t> m_checkCol;
        ColFuncTable<calcResolveFunc_t> m_calcResolve;

    public:
        ColliderResolver(void) {
            // Collision check functions
            {
                this->m_checkCol.set(dal::ColliderType::sphere, dal::ColliderType::sphere, nullptr);
                this->m_checkCol.set(dal::ColliderType::sphere, dal::ColliderType::aabb, this->checkCol_sphere_aabb);
                this->m_checkCol.set(dal::ColliderType::sphere, dal::ColliderType::triangle_soup, nullptr);

                this->m_checkCol.set(dal::ColliderType::aabb, dal::ColliderType::sphere, this->checkCol_aabb_sphere);
                this->m_checkCol.set(dal::ColliderType::aabb, dal::ColliderType::aabb, this->checkCol_aabb_aabb);
                this->m_checkCol.set(dal::ColliderType::aabb, dal::ColliderType::triangle_soup, this->checkCol_aabb_trisoup);

                this->m_checkCol.set(dal::ColliderType::triangle_soup, dal::ColliderType::sphere, nullptr);
                this->m_checkCol.set(dal::ColliderType::triangle_soup, dal::ColliderType::aabb, nullptr);
                this->m_checkCol.set(dal::ColliderType::triangle_soup, dal::ColliderType::triangle_soup, nullptr);
            }

            // Collision resolve function
            {
                this->m_calcResolve.set(dal::ColliderType::aabb, dal::ColliderType::aabb, this->calcResolve_aabb_aabb);
                this->m_calcResolve.set(dal::ColliderType::aabb, dal::ColliderType::triangle_soup, this->calcResolve_aabb_soup);
            }
        }

        bool checkCollision(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) const {
            const auto pFunc = this->m_checkCol.get(one.getColType(), two.getColType());
            if ( nullptr == pFunc ) {
                return false;
            }
            else {
                return pFunc(one, two, transOne, transTwo);
            }
        }

        dal::CollisionResolveInfo calcResolveInfoABS(const dal::ICollider& one, const dal::PhysicalProperty& physicsOne, const dal::Transform& transOne,
            const dal::ICollider& two, const dal::PhysicalProperty& physicsTwo, const dal::Transform& transTwo) const
        {
            const auto oneType = one.getColType();
            const auto twoType = two.getColType();
            const auto func = this->m_calcResolve.get(oneType, twoType);
            if ( nullptr == func ) {
                return dal::CollisionResolveInfo{};
            }
            else {
                return func(one, physicsOne, transOne, two, physicsTwo, transTwo);
            }
        }

    private:
        static bool checkCol_sphere_aabb(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& sphere = reinterpret_cast<const dal::ColSphere&>(one).transform(transOne.getPos(), transOne.getScale());
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(two).transform(transTwo.getPos(), transTwo.getScale());
            return dal::isIntersecting(sphere, aabb);
        }

        static bool checkCol_aabb_sphere(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(one).transform(transOne.getPos(), transOne.getScale());
            const auto& sphere = reinterpret_cast<const dal::ColSphere&>(two).transform(transTwo.getPos(), transTwo.getScale());
            return dal::isIntersecting(sphere, aabb);
        }

        static bool checkCol_aabb_aabb(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& oneAABB = reinterpret_cast<const dal::ColAABB&>(one).transform(transOne.getPos(), transOne.getScale());
            const auto& twoAABB = reinterpret_cast<const dal::ColAABB&>(two).transform(transTwo.getPos(), transTwo.getScale());
            return dal::isIntersecting(oneAABB, twoAABB);
        }

        static bool checkCol_aabb_trisoup(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(one).transform(transOne.getPos(), transOne.getScale());
            const auto& soup = reinterpret_cast<const dal::ColTriangleSoup&>(two);
            return dal::isIntersecting(aabb, soup, transTwo.getMat());
        }

    private:
        static dal::CollisionResolveInfo calcResolve_aabb_aabb(const dal::ICollider& one, const dal::PhysicalProperty& physicsOne, const dal::Transform& transOne,
            const dal::ICollider& two, const dal::PhysicalProperty& physicsTwo, const dal::Transform& transTwo)
        {
            const auto& oneAABB = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& twoAABB = reinterpret_cast<const dal::ColAABB&>(two);
            return dal::calcResolveInfo(oneAABB, physicsOne, transOne, twoAABB, physicsTwo, transTwo);
        }

        static dal::CollisionResolveInfo calcResolve_aabb_soup(const dal::ICollider& one, const dal::PhysicalProperty& physicsOne, const dal::Transform& transOne,
            const dal::ICollider& two, const dal::PhysicalProperty& physicsTwo, const dal::Transform& transTwo)
        {
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& soup = reinterpret_cast<const dal::ColTriangleSoup&>(two);
            return dal::calcResolveInfo(aabb, physicsOne, transOne, soup, physicsTwo, transTwo);
        }

    } g_colResolver;

}


// ICollider
namespace dal {

    ICollider::ICollider(const ColliderType type)
        : m_type(type)
    {
        assert(ColliderType::eoe != this->m_type);
    }

    ColliderType ICollider::getColType(void) const noexcept {
        return this->m_type;
    }

}


// checkCollision funcs
namespace dal {

    bool checkCollisionAbs(const ICollider& one, const ICollider& two, const Transform& transOne, const Transform& transTwo) {
        return g_colResolver.checkCollision(one, two, transOne, transTwo);
    }

    bool checkCollisionAbs(const Segment& seg, const ICollider& col, const Transform& transCol) {
        const auto colType = col.getColType();
        switch ( colType ) {

        case dal::ColliderType::sphere:
        {
            const auto newSphere = reinterpret_cast<const ColSphere&>(col).transform(transCol.getPos(), transCol.getScale());
            return isIntersecting(seg, newSphere);
        }
        case dal::ColliderType::aabb:
        {
            const auto newAABB = reinterpret_cast<const ColAABB&>(col).transform(transCol.getPos(), transCol.getScale());
            return isIntersecting(seg, newAABB);
        }
        case dal::ColliderType::triangle_soup:
            assert(false && "Not implemented.");
            return false;
        default:
            assert(false && "Unkown collider type code");
            return false;

        }
    }

}


#include <iostream>

// calcResolveInfo funcs
namespace dal {

    CollisionResolveInfo calcResolveInfoABS(
        const ICollider& one, const PhysicalProperty& physicsOne, const Transform& transOne,
        const ICollider& two, const PhysicalProperty& physicsTwo, const Transform& transTwo
    ) {
        return g_colResolver.calcResolveInfoABS(one, physicsOne, transOne, two, physicsTwo, transTwo);
    }


    /*
    CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other, const PhysicalProperty& physicsOne, const PhysicalProperty& physicsTwo) {
        const auto sumOfMassInv = physicsOne.getMassInv() + physicsTwo.getMassInv();
        if ( sumOfMassInv == 0.0f ) {
            return CollisionResolveInfo{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        }

        const auto thisFactor = physicsOne.getMassInv() / sumOfMassInv;
        const auto otherFactor = physicsTwo.getMassInv() / sumOfMassInv;

        const auto one1 = one.getPoint000();
        const auto one2 = one.getPoint111();
        const auto other1 = other.getPoint000();
        const auto other2 = other.getPoint111();

        return calcResolveInfo_withMinMax(one1, one2, other1, other2, thisFactor, otherFactor);
    }
    */

    CollisionResolveInfo calcResolveInfo(const AABB& one, const PhysicalProperty& physicsOne, const Transform& transOne,
        const AABB& two, const PhysicalProperty& physicsTwo, const Transform& transTwo)
    {
        const auto sumOfMassInv = physicsOne.getMassInv() + physicsTwo.getMassInv();
        if ( sumOfMassInv == 0.0f ) {
            return CollisionResolveInfo{};
        }

        const auto newOne = one.transform(transOne.getPos(), transOne.getScale());
        const auto newTwo = two.transform(transTwo.getPos(), transTwo.getScale());

        if ( !dal::isIntersecting(newOne, newTwo) ) {
            return CollisionResolveInfo{};
        }

        const auto thisFactor = physicsOne.getMassInv() / sumOfMassInv;
        const auto otherFactor = physicsTwo.getMassInv() / sumOfMassInv;

        const auto one1 = transOne.getScale() * one.min() + transOne.getPos();
        const auto one2 = transOne.getScale() * one.max() + transOne.getPos();
        const auto other1 = transTwo.getScale() * two.min() + transTwo.getPos();
        const auto other2 = transTwo.getScale() * two.max() + transTwo.getPos();

        return calcResolveInfo_withMinMax(one1, one2, other1, other2, thisFactor, otherFactor);
    }

    CollisionResolveInfo calcResolveInfo(const AABB& aabb, const PhysicalProperty& physicsOne, const Transform& transOne,
        const dal::ColTriangleSoup& soup, const PhysicalProperty& physicsTwo, const Transform& transTwo)
    {
        const auto newAABB = aabb.transform(transOne.getPos(), transOne.getScale());
        const auto boxVertices = newAABB.makePoints();

        float maxDist = 0;
        glm::vec3 resolveDirec{ 0 };

        for ( auto& tri : soup ) {
            const auto newTri = tri.transform(transTwo.getMat());
            if ( dal::isIntersecting(newTri, newAABB, boxVertices) ) {
                dal::DebugViewGod::inst().addTriangle(
                    newTri.point0(), newTri.point1(), newTri.point2(), glm::vec4{ 1, 0.3, 0.3, 0.2 }
                );

                const auto [dist, direc] = dal::calcIntersectingDepth(newAABB, newTri.plane());
                if ( dist > maxDist ) {
                    maxDist = dist;
                    resolveDirec = direc;
                }
            }
            else {
                dal::DebugViewGod::inst().addTriangle(
                    newTri.point0(), newTri.point1(), newTri.point2(), glm::vec4{ 0.3, 0, 0, 0.2 }
                );
            }
        }

        CollisionResolveInfo result{};
        result.m_this = -resolveDirec * maxDist;
        result.m_valid = true;

        return result;
    }

}


// calcCollisionInfo funcs
namespace dal {

    std::optional<RayCastingResult> calcCollisionInfoAbs(const Segment& ray, const ICollider& col, const Transform& transCol) {
        const auto colType = col.getColType();
        switch ( colType ) {

        case dal::ColliderType::sphere:
        {
            auto newSphere = reinterpret_cast<const ColSphere&>(col).transform(transCol.getPos(), transCol.getScale());
            assert(false && "Not implemented");
        }
        case dal::ColliderType::aabb:
        {
            auto newAABB = reinterpret_cast<const ColAABB&>(col).transform(transCol.getPos(), transCol.getScale());
            return dal::findIntersection(ray, newAABB);
        }
        case dal::ColliderType::triangle_soup:
        {
            auto newSoup = reinterpret_cast<const ColTriangleSoup&>(col);
            return dal::calcCollisionInfo(ray, newSoup, transCol);
        }
        default:
            assert(false && "Unkown collider type code");
            return std::nullopt;

        }
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const ColTriangleSoup triSoup, const Transform& transTriSoup) {
        std::optional<RayCastingResult> result{ std::nullopt };
        float leastDistance = std::numeric_limits<float>::max();

        for ( const auto& tri : triSoup ) {
            const auto newTri = tri.transform(transTriSoup.getMat());
            const auto info = dal::findIntersection(ray, newTri, triSoup.isFaceCullSet());
            if ( info ) {
                if ( info->m_distance < leastDistance ) {
                    leastDistance = info->m_distance;
                    result = info;
                }
            }
        }

        return result;
    }

}
