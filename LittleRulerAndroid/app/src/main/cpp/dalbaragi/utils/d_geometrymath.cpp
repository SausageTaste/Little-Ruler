#include "d_geometrymath.h"

#include <math.h>


namespace {

    glm::vec3 calcNormalCCW(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        return glm::cross(p1 - p0, p2 - p0);
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
        return r * r * r * dal::PI * 4.0 / 3.0;
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
