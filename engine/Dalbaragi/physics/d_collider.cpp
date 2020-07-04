#include "d_collider.h"

#include <tuple>

#include <u_math.h>


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

    inline void makeTrianglesFromRect(const glm::vec3& p1, const glm::vec3& p2,
        const glm::vec3& p3, const glm::vec3& p4, dal::Triangle& tri1, dal::Triangle& tri2)
    {
        tri1 = dal::Triangle{ p1, p2, p3 };
        tri2 = dal::Triangle{ p1, p3, p4 };
    }

    std::array<dal::Triangle, 12> makeTriangles(std::array<glm::vec3, 8> ps) {
        std::array<dal::Triangle, 12> result;

        makeTrianglesFromRect(ps[3], ps[1], ps[5], ps[7], result[0], result[1]);
        makeTrianglesFromRect(ps[7], ps[5], ps[4], ps[6], result[2], result[3]);
        makeTrianglesFromRect(ps[6], ps[4], ps[0], ps[2], result[4], result[5]);
        makeTrianglesFromRect(ps[2], ps[0], ps[1], ps[3], result[6], result[7]);
        makeTrianglesFromRect(ps[2], ps[3], ps[7], ps[6], result[8], result[9]);
        makeTrianglesFromRect(ps[4], ps[5], ps[1], ps[0], result[10], result[11]);

        return result;
    }

    std::array<dal::Triangle, 12> makeTriangles(const dal::AABB& aabb, const dal::Transform& transAABB) {
        std::array<dal::Triangle, 12> result;

        const auto ps = aabb.getAllPoints([&transAABB](auto vec) { return transAABB.getScale() * vec + transAABB.getPos(); });

        makeTrianglesFromRect(ps[3], ps[1], ps[5], ps[7], result[0], result[1]);
        makeTrianglesFromRect(ps[7], ps[5], ps[4], ps[6], result[2], result[3]);
        makeTrianglesFromRect(ps[6], ps[4], ps[0], ps[2], result[4], result[5]);
        makeTrianglesFromRect(ps[2], ps[0], ps[1], ps[3], result[6], result[7]);
        makeTrianglesFromRect(ps[2], ps[3], ps[7], ps[6], result[8], result[9]);
        makeTrianglesFromRect(ps[4], ps[5], ps[1], ps[0], result[10], result[11]);

        return result;
    }

    // Param p must be on the plane.
    bool isPointInsideTriangle(const glm::vec3& p, const dal::Triangle& tri) {
        const auto edge1 = tri.point<1>() - tri.point<0>();
        const auto edge2 = tri.point<2>() - tri.point<1>();

        const auto toPoint1 = p - tri.point<0>();
        const auto toPoint2 = p - tri.point<1>();

        const auto crossed1 = glm::cross(edge1, toPoint1);
        const auto crossed2 = glm::cross(edge2, toPoint2);

        const auto dotted1 = glm::dot(crossed1, crossed2);

        if ( dotted1 < 0.f ) {
            return false;
        }

        const auto edge3 = tri.point<0>() - tri.point<2>();
        const auto toPoint3 = p - tri.point<2>();
        const auto crossed3 = glm::cross(edge3, toPoint3);
        const auto dotted2 = glm::dot(crossed1, crossed3);

        if ( dotted2 < 0.f ) {
            return false;
        }

        return true;
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
                this->m_checkCol.set(dal::ColliderType::sphere, dal::ColliderType::aabb, checkCol_sphere_aabb);
                this->m_checkCol.set(dal::ColliderType::sphere, dal::ColliderType::triangle_soup, nullptr);

                this->m_checkCol.set(dal::ColliderType::aabb, dal::ColliderType::sphere, checkCol_aabb_sphere);
                this->m_checkCol.set(dal::ColliderType::aabb, dal::ColliderType::aabb, checkCol_aabb_aabb);
                this->m_checkCol.set(dal::ColliderType::aabb, dal::ColliderType::triangle_soup, checkCol_aabb_trisoup);

                this->m_checkCol.set(dal::ColliderType::triangle_soup, dal::ColliderType::sphere, nullptr);
                this->m_checkCol.set(dal::ColliderType::triangle_soup, dal::ColliderType::aabb, nullptr);
                this->m_checkCol.set(dal::ColliderType::triangle_soup, dal::ColliderType::triangle_soup, nullptr);
            }

            // Collision resolve function
            {
                this->m_calcResolve.set(dal::ColliderType::aabb, dal::ColliderType::aabb, calcResolve_aabb_aabb);
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
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& soup = reinterpret_cast<const dal::ColTriangleSoup&>(two);
            return dal::checkCollision(aabb, soup, transOne, transTwo);
        }

    private:
        static dal::CollisionResolveInfo calcResolve_aabb_aabb(const dal::ICollider& one, const dal::PhysicalProperty& physicsOne, const dal::Transform& transOne,
            const dal::ICollider& two, const dal::PhysicalProperty& physicsTwo, const dal::Transform& transTwo)
        {
            const auto& oneAABB = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& twoAABB = reinterpret_cast<const dal::ColAABB&>(two);
            return dal::calcResolveInfo(oneAABB, physicsOne, transOne, twoAABB, physicsTwo, transTwo);
        }

    } g_colResolver;

}


// Collision fucntions
namespace {

    bool checkCollisionWithMinMax(const glm::vec3& one1, const glm::vec3& one2, const glm::vec3& other1, const glm::vec3& other2) {
        if ( one2.x < other1.x ) return false;
        else if ( one1.x > other2.x ) return false;
        else if ( one2.y < other1.y ) return false;
        else if ( one1.y > other2.y ) return false;
        else if ( one2.z < other1.z ) return false;
        else if ( one1.z > other2.z ) return false;
        else return true;
    }

    bool checkCollision_withAllPoints(const std::array<glm::vec3, 8>& points, const dal::Plane& plane) {
        const auto firstOne = plane.isInFront(points[0]);

        for ( size_t i = 1; i < points.size(); ++i ) {
            const auto thisOne = plane.isInFront(points[i]);
            if ( firstOne != thisOne ) {
                return true;
            }
        }

        return false;
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

        }
    }

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
        {
            assert(false && "Not implemented.");
        }
        default:
            assert(false && "Unkown collider type code");

        }
    }

    bool checkCollision(const AABB& aabb, const ColTriangleSoup triSoup, const Transform& transAABB, const Transform& transTriSoup) {
        for ( auto& tri : triSoup ) {
            const auto newTri = tri.transform(transTriSoup.getMat());
            const auto newBox = aabb.transform(transAABB.getPos(), transAABB.getScale());
            if ( dal::isIntersecting(newTri, newBox) ) {
                return true;
            }
        }
        return false;
    }

}


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

}


// calcCollisionInfo funcs
namespace dal {

    std::optional<RayCastingResult> calcCollisionInfoAbs(const Segment& ray, const ICollider& col, const Transform& transCol) {
        const auto colType = col.getColType();
        switch ( colType ) {

        case dal::ColliderType::sphere:
        {
            auto newSphere = reinterpret_cast<const ColSphere&>(col);
            return calcCollisionInfo(ray, newSphere, transCol);
        }
        case dal::ColliderType::aabb:
        {
            auto newAABB = reinterpret_cast<const ColAABB&>(col);
            return calcCollisionInfo(ray, newAABB, transCol);
        }
        case dal::ColliderType::triangle_soup:
        {
            auto newSoup = reinterpret_cast<const ColTriangleSoup&>(col);
            return calcCollisionInfo(ray, newSoup, transCol);
        }
        default:
            assert(false && "Unkown collider type code");

        }
    }


    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const Plane& plane) {
        const auto pointA = ray.pos();
        const auto pointB = pointA + ray.rel();

        const auto distA = plane.calcSignedDist(pointA);
        const auto distB = plane.calcSignedDist(pointB);

        if ( (distA * distB) > 0.0f ) {
            return std::nullopt;
        }

        const auto absDistA = std::abs(distA);
        const auto distance = ray.length() * absDistA / (absDistA + std::abs(distB));

        return RayCastingResult{ distance, distA > distB };
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const Triangle& tri, const bool ignoreFromBack) {
        const auto plane = tri.plane();
        const auto planeCol = calcCollisionInfo(ray, plane);
        if ( !planeCol ) {
            return std::nullopt;
        }
        else if ( ignoreFromBack && !planeCol->m_isFromFront ) {
            return std::nullopt;
        }

        if ( isIntersecting(ray, tri) ) {
            return planeCol;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const Sphere& sphere, const Transform& transSphere) {
        assert(false && "Not implemented.");
        return std::nullopt;
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const AABB& aabb) {
        if ( aabb.volume() == 0.f ) {
            return std::nullopt;
        }

        const auto triangles = makeTriangles(aabb.getAllPoints());
        for ( auto& tri : triangles ) {
            const auto triCol = calcCollisionInfo(ray, tri);
            if ( triCol ) {
                return triCol;
            }
        }

        return std::nullopt;
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const AABB& aabb, const Transform& transAABB) {
        if ( aabb.volume() == 0.0f ) {
            return std::nullopt;
        }

        const auto triangles = makeTriangles(aabb.getAllPoints(transAABB.getPos(), transAABB.getScale()));
        for ( auto& tri : triangles ) {
            const auto triCol = calcCollisionInfo(ray, tri);
            if ( triCol ) {
                return triCol;
            }
        }

        return std::nullopt;
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Segment& ray, const ColTriangleSoup triSoup, const Transform& transTriSoup) {
        std::optional<RayCastingResult> result{ std::nullopt };
        float leastDistance = std::numeric_limits<float>::max();

        for ( const auto& tri : triSoup ) {
            const auto newTri = tri.transform(transTriSoup.getMat());
            const auto info = dal::calcCollisionInfo(ray, newTri, triSoup.isFaceCullSet());
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
