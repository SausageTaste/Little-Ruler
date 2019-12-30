#include "d_geometrymath.h"

#include <limits>


namespace {

    glm::vec3 calcNormalCCW(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        return glm::cross(p1 - p0, p2 - p0);
    }

    // Param p must be on the plane.
    bool isPointInsideTri(const glm::vec3& p, const dal::Triangle& tri) {
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


// Segment
namespace dal {

    Segment::Segment(const glm::vec3& pos, const glm::vec3& move) 
        : m_pos(pos)
        , m_rel(move)
    {

    }

    const glm::vec3& Segment::pos(void) const {
        return this->m_pos;
    }

    const glm::vec3& Segment::rel(void) const {
        return this->m_rel;
    }

    float Segment::length(void) const {
        return glm::length(this->rel());
    }

    float Segment::lengthSqr(void) const {
        return glm::dot(this->rel(), this->rel());
    }

    glm::vec3 Segment::findNearestPointOnSeg(const glm::vec3& p) const {
        // From https://answers.unity.com/questions/62644/distance-between-a-ray-and-a-point.html

        constexpr auto EPSILON = 1E-06f;

        const auto rhs = p - this->pos();
        const auto magnitude = this->length();
        const auto lhs = magnitude > EPSILON ? (this->rel() / magnitude) : this->rel();
        const auto num2 = glm::clamp<float>(glm::dot(lhs, rhs), 0, magnitude);

        return this->m_pos + (lhs * num2);
    }

    void Segment::setPos(const glm::vec3& pos) {
        this->m_pos = pos;
    }

    void Segment::setRel(const glm::vec3& rel) {
        this->m_rel = rel;
    }

    void Segment::set(const glm::vec3& pos, const glm::vec3& rel) {
        this->m_pos = pos;
        this->m_rel = rel;
    }

}


// Plane
namespace dal {

    Plane::Plane(const glm::vec3& normal, const glm::vec3& point)
        : m_normal(glm::normalize(normal))
        , m_point(point)
    {

    }

    Plane::Plane(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
        : Plane(calcNormalCCW(p0, p1, p2), p0)
    {

    }

    const glm::vec3& Plane::normal(void) const {
        return this->m_normal;
    }

    glm::vec4 Plane::planeEquation(void) const {
        const auto d = -glm::dot(this->m_normal, this->m_point);
        return glm::vec4{ this->normal(), d };
    }

    float Plane::calcSignedDist(const glm::vec3& p) const {
        return glm::dot(this->planeEquation(), glm::vec4{ p, 1 });
    }

    bool Plane::isInFront(const glm::vec3& p) const {
        return 0.f < this->calcSignedDist(p);
    }

    void Plane::set(const glm::vec3& normal, const glm::vec3& point) {
        this->m_normal = glm::normalize(normal);
        this->m_point = point;
    }

    void Plane::set(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        this->set(calcNormalCCW(p0, p1, p2), p0);
    }

}


// Triangle
namespace dal {

    Triangle::Triangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        this->m_points[0] = p0;
        this->m_points[1] = p1;
        this->m_points[2] = p2;
    }

    glm::vec3 Triangle::normal(void) const {
        return glm::normalize(calcNormalCCW(this->m_points[0], this->m_points[1], this->m_points[2]));
    }

    Plane Triangle::plane(void) const {
        return Plane{ this->m_points[0], this->m_points[1], this->m_points[2] };
    }

    float Triangle::area(void) const {
        const auto a = glm::distance(this->m_points[1], this->m_points[0]);
        const auto b = glm::distance(this->m_points[2], this->m_points[1]);
        const auto c = glm::distance(this->m_points[0], this->m_points[2]);

        const auto s = (a + b + c) * 0.5f;
        return sqrt(s * (s - a) * (s - b) * (s - c));
    }

}


// Sphere
namespace dal {

    Sphere::Sphere(const glm::vec3& center, const float radius)
        : m_center(center)
        , m_radius(radius)
    {

    }

    float Sphere::volume(void) const {
        const double r = this->radius();
        return r * r * r * static_cast<double>(DAL_PI) * 4.0 / 3.0;
    }

    float Sphere::calcSignedDist(const glm::vec3& p) const {
        return glm::distance(this->center(), p) - this->radius();
    }

    bool Sphere::isInside(const glm::vec3& p) const {
        const auto dist = p - this->m_center;
        return glm::dot(dist, dist) < (this->radius() * this->radius());
    }

    void Sphere::upscaleToInclude(const glm::vec3& p) {
        const auto dist = glm::distance(this->center(), p);
        if ( dist > this->m_radius ) {
            this->m_radius = dist;
        }
    }

}


// AABB
namespace dal {

    AABB::AABB(const glm::vec3& p0, const glm::vec3& p1) {
        this->set(p0, p1);
    }

    float AABB::volume(void) const {
        return (this->m_max.x - this->m_min.x) * (this->m_max.y - this->m_min.y) * (this->m_max.z - this->m_min.z);
    }

    bool AABB::isInside(const glm::vec3& p) const {
        for ( int i = 0; i < 3; ++i ) {
            if ( p[i] <= this->m_min[i] ) {
                return false;
            }
            else if ( p[i] >= this->m_max[i] ) {
                return false;
            }
        }

        return true;
    }

    void AABB::set(const glm::vec3& p0, const glm::vec3& p1) {
        for ( int i = 0; i < 3; ++i ) {
            const auto value0 = p0[i];
            const auto value1 = p1[i];

            if ( value0 < value1 ) {
                this->m_min[i] = value0;
                this->m_max[i] = value1;
            }
            else {
                this->m_min[i] = value1;
                this->m_max[i] = value0;
            }
        }
    }

    void AABB::upscaleToInclude(const glm::vec3& p) {
        for ( int i = 0; i < 3; ++i ) {
            if ( p[i] < this->m_min[i] ) {
                this->m_min[i] = p[i];
            }
            else if ( p[i] > this->m_max[i] ) {
                this->m_max[i] = p[i];
            }
        }
    }

}


// Intersection check
namespace dal {

    bool isIntersecting(const Segment& seg, const Plane& plane) {
        const auto pointA = seg.pos();
        const auto pointB = pointA + seg.rel();

        const auto distA = plane.calcSignedDist(pointA);
        const auto distB = plane.calcSignedDist(pointB);

        return (distA * distB) <= 0.0f;
    }

    bool isIntersecting(const Segment& seg, const Triangle& tri) {
        const auto plane = tri.plane();
        const auto planeCol = findIntersection(seg, plane);

        if ( !planeCol ) {
            return false;
        }
        else {
            const auto collisionPoint = seg.pos() + glm::normalize(seg.rel()) * planeCol->m_distance;
            return isPointInsideTri(collisionPoint, tri);
        }
    }

    bool isIntersecting(const Segment& seg, const Sphere& sphere) {
        if ( sphere.isInside(seg.pos()) ) {
            return true;
        }
        else if ( sphere.isInside(seg.endpoint()) ) {
            return true;
        }
        else {
            const auto nearestPoint = seg.findNearestPointOnSeg(sphere.center());
            return glm::distance(nearestPoint, sphere.center()) <= sphere.radius();
        }
    }

    bool isIntersecting(const Segment& seg, const AABB& aabb) {
        // From https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

        const glm::vec3 bounds[2] = { aabb.min(), aabb.max() };
        const auto orig = seg.pos();
        const auto invdir = 1.f / seg.rel();
        const int sign[3] = {
            invdir.x < 0,
            invdir.y < 0,
            invdir.z < 0
        };

        auto tmin = (bounds[sign[0]].x - orig.x) * invdir.x;
        auto tmax = (bounds[1 - sign[0]].x - orig.x) * invdir.x;

        const auto tymin = (bounds[sign[1]].y - orig.y) * invdir.y;
        const auto tymax = (bounds[1 - sign[1]].y - orig.y) * invdir.y;

        if ( (tmin > tymax) || (tymin > tmax) ) {
            return false;
        }
        if ( tymin > tmin ) {
            tmin = tymin;
        }
        if ( tymax < tmax ) {
            tmax = tymax;
        }

        const auto tzmin = (bounds[sign[2]].z - orig.z) * invdir.z;
        const auto tzmax = (bounds[1 - sign[2]].z - orig.z) * invdir.z;

        if ( (tmin > tzmax) || (tzmin > tmax) ) {
            return false;
        }
        if ( tzmin > tmin ) {
            tmin = tzmin;
        }
        if ( tzmax < tmax ) {
            tmax = tzmax;
        }

        return true;
    }

}


// Calc intersection info
namespace dal {

    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Plane& plane) {
        const auto pointA = seg.pos();
        const auto pointB = pointA + seg.rel();

        const auto distA = plane.calcSignedDist(pointA);
        const auto distB = plane.calcSignedDist(pointB);

        if ( (distA * distB) > 0.0f ) {
            return std::nullopt;
        }

        const auto absDistA = std::abs(distA);
        const auto distance = seg.length() * absDistA / (absDistA + std::abs(distB));

        return SegIntersecInfo{ distA > distB, distance };
    }

    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Triangle& tri) {
        const auto plane = tri.plane();
        const auto planeCol = findIntersection(seg, plane);

        if ( planeCol ) {
            const auto collisionPoint = seg.pos() + glm::normalize(seg.rel()) * planeCol->m_distance;
            if ( isPointInsideTri(collisionPoint, tri) ) {
                return planeCol;
            }
        }

        return std::nullopt;
    }

    // Not tested yet.
    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const AABB& aabb) {
        Plane planes[6];
        {
            planes[0].set(glm::vec3{ -1, 0, 0 }, aabb.min());
            planes[1].set(glm::vec3{ 0, -1, 0 }, aabb.min());
            planes[2].set(glm::vec3{ 0, 0, -1 }, aabb.min());

            planes[3].set(glm::vec3{ 1, 0, 0 }, aabb.max());
            planes[4].set(glm::vec3{ 0, 1, 0 }, aabb.max());
            planes[5].set(glm::vec3{ 0, 0, 1 }, aabb.max());
        }

        std::optional<SegIntersecInfo> result{ std::nullopt };
        float distance = std::numeric_limits<float>::max();

        for ( int i = 0; i < 6; ++i ) {
            const auto col = findIntersection(seg, planes[i]);
            if ( col && col->m_distance < distance ) {
                result = col;
            }
        }

        return result;
    }

}
