#pragma once

#include <array>
#include <optional>

#include <glm/glm.hpp>


#define DAL_PI 3.141592653589793238462643383279502884L


namespace dal {

    class Segment {

    private:
        glm::vec3 m_pos{ 0 }, m_rel{ 0 };

    public:
        Segment(void) = default;
        Segment(const glm::vec3& pos, const glm::vec3& move);

        const glm::vec3& pos(void) const;
        const glm::vec3& rel(void) const;
        glm::vec3 endpoint(void) const {
            return this->pos() + this->rel();
        }
        float length(void) const;
        float lengthSqr(void) const;

        void setPos(const glm::vec3& pos);
        void setRel(const glm::vec3& rel);
        void set(const glm::vec3& pos, const glm::vec3& rel);

        glm::vec3 findNearestPointOnSeg(const glm::vec3& p) const;

    };


    class Plane {

    private:
        glm::vec3 m_normal{ 0, 1, 0 }, m_point{ 0 };

    public:
        Plane(void) = default;
        Plane(const glm::vec3& normal, const glm::vec3& point);
        Plane(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);

        const glm::vec3& normal(void) const;
        glm::vec4 planeEquation(void) const;

        float calcSignedDist(const glm::vec3& p) const;
        bool isInFront(const glm::vec3& v) const;

        void set(const glm::vec3& normal, const glm::vec3& point);
        void set(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);

    };


    class Triangle {

    private:
        std::array<glm::vec3, 3> m_points = { {{0.f, 0.f, 0.f}} };

    public:
        Triangle(void) = default;
        Triangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);

        template <unsigned I>
        const glm::vec3& point(void) const {
            static_assert(I < 3);
            return this->m_points[I];
        }
        glm::vec3 normal(void) const;
        Plane plane(void) const;
        float area(void) const;

    };


    class Sphere {

    private:
        glm::vec3 m_center{ 0 };
        float m_radius = 0;

    public:
        Sphere(void) = default;
        Sphere(const glm::vec3& center, const float radius);

        const glm::vec3& center(void) const {
            return this->m_center;
        }
        float radius(void) const {
            return this->m_radius;
        }
        float volume(void) const;

        float calcSignedDist(const glm::vec3& p) const;
        bool isInside(const glm::vec3& p) const;

        void setCenter(const glm::vec3& center) {
            this->m_center = center;
        }
        void setRadius(const float radius) {
            this->m_radius = radius;
        }

        void upscaleToInclude(const glm::vec3& p);

    };


    class AABB {

    private:
        glm::vec3 m_min{ 0 }, m_max{ 0 };

    public:
        AABB(void) = default;
        AABB(const glm::vec3& p0, const glm::vec3& p1);

        const glm::vec3& min(void) const {
            return this->m_min;
        }
        const glm::vec3& max(void) const {
            return this->m_max;
        }
        float volume(void) const;

        bool isInside(const glm::vec3& p) const;

        void set(const glm::vec3& p0, const glm::vec3& p1);

        void upscaleToInclude(const glm::vec3& p);

    };

}


// Intersection check
namespace dal {

    bool isIntersecting(const Segment& seg, const Plane& plane);
    bool isIntersecting(const Segment& seg, const Triangle& tri);
    bool isIntersecting(const Segment& seg, const Sphere& sphere);
    bool isIntersecting(const Segment& seg, const AABB& aabb);

}


// Calc intersection info
namespace dal {

    struct SegIntersecInfo {
        float m_distance = 0;
        bool m_isFromFront = false;
    };

    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Plane& plane);
    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Triangle& tri);
    //std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Sphere& sphere);
    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const AABB& aabb);

}
