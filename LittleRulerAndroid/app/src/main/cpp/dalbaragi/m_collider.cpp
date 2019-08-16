#include "m_collider.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    const glm::vec3 AABB_NORMALS[6] = {
        { -1.f,  0.f,  0.f },
        {  0.f, -1.f,  0.f },
        {  0.f,  0.f, -1.f },
        {  1.f,  0.f,  0.f },
        {  0.f,  1.f,  0.f },
        {  0.f,  0.f,  1.f }
    };

}


namespace {

    size_t minValueIndex(const float* const arr, const size_t arrSize) {
        if ( 0 == arrSize ) {
            dalAbort("Are you mad?");
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

}


namespace dal {

    AABB::AABB(const glm::vec3& p1, const glm::vec3& p2, const float massInv)
        : m_p1(p1), m_p2(p2)
        , m_massInv(massInv)
    {
        this->validateOrder();
    }

    std::array<glm::vec3, 8> AABB::getAllPoints(void) const {
        std::array<glm::vec3, 8> result;

        {
            const auto p000 = this->getPoint000();
            const auto p111 = this->getPoint111();

            result[0] = p000;  // 000
            result[1] = glm::vec3{ p000.x, p000.y, p111.z };  // 001
            result[2] = glm::vec3{ p000.x, p111.y, p000.z };  // 010
            result[3] = glm::vec3{ p000.x, p111.y, p111.z };  // 011
            result[4] = glm::vec3{ p111.x, p000.y, p000.z };  // 100
            result[5] = glm::vec3{ p111.x, p000.y, p111.z };  // 101
            result[6] = glm::vec3{ p111.x, p111.y, p000.z };  // 110
            result[7] = p111;  // 111
        }

        return result;
    }

    void AABB::set(const glm::vec3& p1, const glm::vec3& p2) {
        this->m_p1 = p1;
        this->m_p2 = p2;

        this->validateOrder();
    }

    void AABB::add(const glm::vec3& offset) {
        this->m_p1 += offset;
        this->m_p2 += offset;
    }

    void AABB::scale(const float mag) {
        this->m_p1 *= mag;
        this->m_p2 *= mag;
    }

    // Private

    void AABB::validateOrder(void) {
        if ( this->m_p1.x > this->m_p2.x ) {
            std::swap(this->m_p1.x, this->m_p2.x);
        }
        if ( this->m_p1.y > this->m_p2.y ) {
            std::swap(this->m_p1.y, this->m_p2.y);
        }
        if ( this->m_p1.z > this->m_p2.z ) {
            std::swap(this->m_p1.z, this->m_p2.z);
        }
    }

}


namespace dal {

    Plane::Plane(void)
        : m_coeff(0.0f, 1.0f, 0.0f, 0.0f)
    {

    }

    Plane::Plane(const glm::vec3& normal, const glm::vec3& point)
        : m_coeff(normal.x, normal.y, normal.z, -glm::dot(normal, point))
    {

    }

    Plane::Plane(const float a, const float b, const float c, const float d)
        : m_coeff(a, b, c, d)
    {

    }

}


namespace dal {

    Ray::Ray(void)
        : m_pos(0.f, 0.f, 0.f)
        , m_rel(0.f, 1.f, 0.f)
        , m_len(1.f)
    {

    }

    Ray::Ray(const glm::vec3& pos, const glm::vec3& rel)
        : m_pos(pos)
        , m_rel(rel)
        , m_len(glm::length(rel))
    {
        dalAssert(this->m_len > 0.0f);
    }

    void Ray::setRel(const glm::vec3& v) {
        this->m_rel = v;
        this->m_len = glm::length(this->m_rel);
        dalAssert(this->m_len > 0.0f);
    }

}


namespace dal {

    bool checkCollision(const AABB& one, const AABB& other) {
        if ( one.m_p2.x < other.m_p1.x ) return false;
        else if ( one.m_p1.x > other.m_p2.x ) return false;
        else if ( one.m_p2.y < other.m_p1.y ) return false;
        else if ( one.m_p1.y > other.m_p2.y ) return false;
        else if ( one.m_p2.z < other.m_p1.z ) return false;
        else if ( one.m_p1.z > other.m_p2.z ) return false;
        else return true;
    }

    bool checkCollision(const AABB& aabb, const Plane& plane) {
        const auto points = aabb.getAllPoints();

        const auto firstOne = plane.isInFront(points[0]);

        for ( size_t i = 1; i < points.size(); ++i ) {
            const auto thisOne = plane.isInFront(points[i]);
            if ( firstOne != thisOne ) {
                return true;
            }
        }

        return false;
    }

    bool checkCollision(const Ray& ray, const Plane& plane) {
        const auto pointA = ray.getStartPos();
        const auto pointB = pointA + ray.getRel();

        const auto distA = plane.getSignedDist(pointA);
        const auto distB = plane.getSignedDist(pointB);

        return (distA * distB) <= 0.0f;
    }

    /*
    bool checkCollision(const Ray& ray, const AABB& aabb) {
        for ( int i = 0; i < 3; ++i ) {
            Plane plane{ AABB_NORMALS[i], aabb.getPoint000() };
            if ( checkCollision(ray, plane) ) {
                return true;
            }
        }
        for ( int i = 3; i < 6; ++i ) {
            Plane plane{ AABB_NORMALS[i], aabb.getPoint111() };
            if ( checkCollision(ray, plane) ) {
                return true;
            }
        }

        return false;
    }
    */

    bool checkCollision(const Ray& ray, const AABB& aabb) {
        // From https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

        const glm::vec3 bounds[2] = { aabb.getPoint000(), aabb.getPoint111() };
        const auto orig = ray.getStartPos();
        const auto invdir = 1.f / ray.getRel();
        const int sign[3] = {
            invdir.x < 0,
            invdir.y < 0,
            invdir.z < 0
        };

        float tmin, tmax, tymin, tymax, tzmin, tzmax;

        tmin = (bounds[sign[0]].x - orig.x) * invdir.x;
        tmax = (bounds[1 - sign[0]].x - orig.x) * invdir.x;
        tymin = (bounds[sign[1]].y - orig.y) * invdir.y;
        tymax = (bounds[1 - sign[1]].y - orig.y) * invdir.y;

        if ( (tmin > tymax) || (tymin > tmax) )
            return false;
        if ( tymin > tmin )
            tmin = tymin;
        if ( tymax < tmax )
            tmax = tymax;

        tzmin = (bounds[sign[2]].z - orig.z) * invdir.z;
        tzmax = (bounds[1 - sign[2]].z - orig.z) * invdir.z;

        if ( (tmin > tzmax) || (tzmin > tmax) )
            return false;
        if ( tzmin > tmin )
            tmin = tzmin;
        if ( tzmax < tmax )
            tmax = tzmax;

        return true;
    }


    CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other,
        const float oneMassInv, const float otherMassInv)
    {
        const auto sumOfMassInv = oneMassInv + otherMassInv;
        if ( sumOfMassInv == 0.0f ) {
            return CollisionResolveInfo{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        }

        const auto thisFactor = oneMassInv / sumOfMassInv;
        const auto otherFactor = otherMassInv / sumOfMassInv;

        const auto xOne = one.m_p2.x - other.m_p1.x;
        const auto xTwo = one.m_p1.x - other.m_p2.x;
        const auto xDistance = abs(xOne) < abs(xTwo) ? xOne : xTwo;

        const auto yOne = one.m_p2.y - other.m_p1.y;
        const auto yTwo = one.m_p1.y - other.m_p2.y;
        const auto yDistance = abs(yOne) < abs(yTwo) ? yOne : yTwo;

        const auto zOne = one.m_p2.z - other.m_p1.z;
        const auto zTwo = one.m_p1.z - other.m_p2.z;
        const auto zDistance = abs(zOne) < abs(zTwo) ? zOne : zTwo;

        const auto xForThis = -xDistance * thisFactor;
        const auto yForThis = -yDistance * thisFactor;
        const auto zForThis = -zDistance * thisFactor;

        const auto xForOther = xDistance * otherFactor;
        const auto yForOther = yDistance * otherFactor;
        const auto zForOther = zDistance * otherFactor;

        const float selector[3] = { std::abs(xForThis), std::abs(yForThis), std::abs(zForThis) };
        const auto index = minValueIndex(selector, 3);
        switch ( index ) {

        case 0:
            return CollisionResolveInfo{ { xForThis, 0.0f, 0.0f }, { xForOther, 0.0f, 0.0f } };
        case 1:
            return CollisionResolveInfo{ { 0.0f, yForThis, 0.0f }, { 0.0f, yForOther, 0.0f } };
        case 2:
            return CollisionResolveInfo{ { 0.0f, 0.0f, zForThis }, { 0.0f, 0.0f, zForOther } };
        default:
            dalAbort("This can't happen!");

        }
    }


    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const Plane& plane) {
        const auto pointA = ray.getStartPos();
        const auto pointB = pointA + ray.getRel();

        const auto distA = plane.getSignedDist(pointA);
        const auto distB = plane.getSignedDist(pointB);

        if ( (distA * distB) > 0.0f ) {
            return std::nullopt;
        }

        const auto absDistA = std::abs(distA);
        const auto distance = ray.getLength() * absDistA / (absDistA + std::abs(distB));

        return RayCastingResult{ distA > distB, distance };
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const AABB& aabb) {
        RayCastingResult result;
        bool found = false;

        auto innerFunc = [&](const Plane& plane) -> void {
            const auto info = calcCollisionInfo(ray, plane);
            if ( info ) {
                if ( found ) {
                    if ( info->m_distance < result.m_distance ) {
                        result = *info;
                        found = true;
                    }
                }
                else {
                    result = *info;
                    found = true;
                }
            }
        };

        for ( int i = 0; i < 3; ++i ) {
            Plane plane{ AABB_NORMALS[i], aabb.getPoint000() };
            innerFunc(plane);
        }
        for ( int i = 3; i < 6; ++i ) {
            Plane plane{ AABB_NORMALS[i], aabb.getPoint111() };
            innerFunc(plane);
        }

        if ( found ) {
            return result;
        }
        else {
            return std::nullopt;
        }
    }

}