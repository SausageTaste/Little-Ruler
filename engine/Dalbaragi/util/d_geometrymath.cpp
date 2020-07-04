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


// Copied from web
namespace {

    // From https://stackoverflow.com/questions/4578967/cube-sphere-intersection-test
    bool doesCubeIntersectSphere(const glm::vec3 C1, const glm::vec3 C2, const glm::vec3 S, const float R) {
        const auto squared = [](const float v) -> float {
            return v * v;
        };

        float dist_squared = R * R;
        /* assume C1 and C2 are element-wise sorted, if not, do that now */
        if ( S.x < C1.x )
            dist_squared -= squared(S.x - C1.x);
        else if ( S.x > C2.x )
            dist_squared -= squared(S.x - C2.x);
        if ( S.y < C1.y )
            dist_squared -= squared(S.y - C1.y);
        else if ( S.y > C2.y )
            dist_squared -= squared(S.y - C2.y);
        if ( S.z < C1.z )
            dist_squared -= squared(S.z - C1.z);
        else if ( S.z > C2.z )
            dist_squared -= squared(S.z - C2.z);
        return dist_squared > 0.f;
    }


    // Frome http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox3.txt
    namespace fileadmin {

        template <typename T>
        inline std::pair<T, T> findMinMax(const T x0, const T x1, const T x2) {
            T min, max;
            min = max = x0;

            if ( x1 < min ) min = x1;
            if ( x1 > max ) max = x1;
            if ( x2 < min ) min = x2;
            if ( x2 > max ) max = x2;

            return { min, max };
        }

        bool planeBoxOverlap(const glm::vec3 normal, const glm::vec3 vert, const glm::vec3 maxbox) {
            glm::vec3 vmin, vmax;

            for ( int i = 0; i < 3; i++ ) {
                auto v = vert[i];
                if ( normal[i] > 0.0f ) {
                    vmin[i] = -maxbox[i] - v;
                    vmax[i] = maxbox[i] - v;
                }
                else {
                    vmin[i] = maxbox[i] - v;
                    vmax[i] = -maxbox[i] - v;
                }
            }

            if ( glm::dot(normal, vmin) > 0.0f )
                return false;
            else if ( glm::dot(normal, vmax) >= 0.0f )
                return true;
            else
                return false;
        }

        bool triBoxOverlap(const glm::vec3 boxcenter, const glm::vec3 boxhalfsize, const std::array<glm::vec3, 3>& triverts) {
            /*    use separating axis theorem to test overlap between triangle and box */
            /*    need to test for overlap in these directions: */
            /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
            /*       we do not even need to test these) */
            /*    2) normal of the triangle */
            /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
            /*       this gives 3x3=9 more tests */

            //   float axis[3];
            float min, max;		// -NJMP- "d" local variable removed

            /* This is the fastest branch on Sun */

            /* move everything so that the boxcenter is in (0,0,0) */
            const auto v0 = triverts[0] - boxcenter;
            const auto v1 = triverts[1] - boxcenter;
            const auto v2 = triverts[2] - boxcenter;

            /* compute triangle edges */
            auto e0 = v1 - v0;      /* tri edge 0 */
            auto e1 = v2 - v1;      /* tri edge 1 */
            auto e2 = v0 - v2;      /* tri edge 2 */

            /* Bullet 3:  */
            /*  test the 9 tests first (this was faster) */

            auto fex = fabsf(e0.x);
            auto fey = fabsf(e0.y);
            auto fez = fabsf(e0.z);

            constexpr int X = 0;
            constexpr int Y = 1;
            constexpr int Z = 2;

            auto AXISTEST_X01 = [&](const float a, const float b, const float fa, const float fb) {
                const auto p0 = a * v0.y - b * v0.z;
                const auto p2 = a * v2.y - b * v2.z;
                if ( p0 < p2 ) {
                    min = p0;
                    max = p2;
                }
                else {
                    min = p2;
                    max = p0;
                }

                const auto rad = fa * boxhalfsize.y + fb * boxhalfsize.z;

                if ( min > rad || max < -rad )
                    return 0;
                else
                    return 1;
            };

            auto AXISTEST_X2 = [&](const float a, const float b, const float fa, const float fb) {
                const auto p0 = a * v0[Y] - b * v0[Z];
                const auto p1 = a * v1[Y] - b * v1[Z];
                if ( p0 < p1 ) { min = p0; max = p1; }
                else { min = p1; max = p0; }
                const auto rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];
                if ( min > rad || max < -rad ) return 0;
                else return 1;
            };

            auto AXISTEST_Y1 = [&](const float a, const float b, const float fa, const float fb) {
                const auto p0 = -a * v0[X] + b * v0[Z];
                const auto p1 = -a * v1[X] + b * v1[Z];
                if ( p0 < p1 ) { min = p0; max = p1; }
                else { min = p1; max = p0; }
                const auto rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];
                if ( min > rad || max < -rad ) return 0;
                else return 1;
            };

            auto AXISTEST_Y02 = [&](const float a, const float b, const float fa, const float fb) {
                const auto p0 = -a * v0[X] + b * v0[Z];
                const auto p2 = -a * v2[X] + b * v2[Z];
                if ( p0 < p2 ) { min = p0; max = p2; }
                else { min = p2; max = p0; }
                const auto rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];
                if ( min > rad || max < -rad ) return 0;
                else return 1;
            };

            auto AXISTEST_Z12 = [&](const float a, const float b, const float fa, const float fb) {
                const auto p1 = a * v1[X] - b * v1[Y];
                const auto p2 = a * v2[X] - b * v2[Y];
                if ( p2 < p1 ) { min = p2; max = p1; }
                else { min = p1; max = p2; }
                const auto rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];
                if ( min > rad || max < -rad ) return 0;
                else return 1;
            };

            auto AXISTEST_Z0 = [&](const float a, const float b, const float fa, const float fb) {
                const auto p0 = a * v0[X] - b * v0[Y];
                const auto p1 = a * v1[X] - b * v1[Y];
                if ( p0 < p1 ) { min = p0; max = p1; }
                else { min = p1; max = p0; }
                const auto rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];
                if ( min > rad || max < -rad ) return 0;
                else return 1;
            };

            AXISTEST_X01(e0.z, e0.y, fez, fey);
            AXISTEST_Y02(e0.z, e0.x, fez, fex);
            AXISTEST_Z12(e0.y, e0.x, fey, fex);

            fex = fabsf(e1[X]);
            fey = fabsf(e1[Y]);
            fez = fabsf(e1[Z]);

            AXISTEST_X01(e1[Z], e1[Y], fez, fey);
            AXISTEST_Y02(e1[Z], e1[X], fez, fex);
            AXISTEST_Z0(e1[Y], e1[X], fey, fex);

            fex = fabsf(e2[X]);
            fey = fabsf(e2[Y]);
            fez = fabsf(e2[Z]);

            AXISTEST_X2(e2[Z], e2[Y], fez, fey);
            AXISTEST_Y1(e2[Z], e2[X], fez, fex);
            AXISTEST_Z12(e2[Y], e2[X], fey, fex);

            /* Bullet 1: */
            /*  first test overlap in the {x,y,z}-directions */
            /*  find min, max of the triangle each direction, and test for overlap in */
            /*  that direction -- this is equivalent to testing a minimal AABB around */
            /*  the triangle against the AABB */

            /* test in X-direction */
            std::tie(min, max) = findMinMax(v0[X], v1[X], v2[X]);
            if ( min > boxhalfsize[X] || max < -boxhalfsize[X] ) return 0;

            /* test in Y-direction */
            std::tie(min, max) = findMinMax(v0[Y], v1[Y], v2[Y]);
            if ( min > boxhalfsize[Y] || max < -boxhalfsize[Y] ) return 0;

            /* test in Z-direction */
            std::tie(min, max) = findMinMax(v0[Z], v1[Z], v2[Z]);
            if ( min > boxhalfsize[Z] || max < -boxhalfsize[Z] ) return 0;

            /* Bullet 2: */
            /*  test if the box intersects the plane of the triangle */
            /*  compute plane equation of triangle: normal*x+d=0 */
            const auto normal = glm::cross(e0, e1);
            // -NJMP- (line removed here)
            if ( !planeBoxOverlap(normal, v0, boxhalfsize) )
                return 0;

            return 1;   /* box and triangle overlaps */
        }

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

    glm::vec3 Segment::findNearestPointOnSeg(const glm::vec3& p) const {
        // From https://answers.unity.com/questions/62644/distance-between-a-ray-and-a-point.html

        constexpr auto EPSILON = 1E-06f;

        const auto rhs = p - this->pos();
        const auto magnitude = this->length();
        const auto lhs = magnitude > EPSILON ? (this->rel() / magnitude) : this->rel();
        const auto num2 = glm::clamp<float>(glm::dot(lhs, rhs), 0, magnitude);

        return this->m_pos + (lhs * num2);
    }

    float Segment::calcDistance(const glm::vec3& p) const {
        const auto projected = this->findNearestPointOnSeg(p);
        return glm::distance(projected, p);
    }

}


// Plane
namespace dal {

    Plane::Plane(const glm::vec3& normal, const glm::vec3& point) {
        this->set(normal, point);
    }

    Plane::Plane(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        this->set(p0, p1, p2);
    }

    Plane::Plane(const float a, const float b, const float c, const float d) {
        this->set(a, b, c, d);
    }

    const glm::vec3& Plane::normal(void) const {
        return this->m_normal;
    }

    glm::vec4 Plane::coeff(void) const {
        return glm::vec4{ this->normal(), this->m_d };
    }

    float Plane::calcSignedDist(const glm::vec3& p) const {
        return glm::dot(this->coeff(), glm::vec4{ p, 1 });
    }

    bool Plane::isInFront(const glm::vec3& p) const {
        return 0.f < this->calcSignedDist(p);
    }

    void Plane::set(const glm::vec3& normal, const glm::vec3& point) {
        this->m_normal = glm::normalize(normal);
        this->m_d = -glm::dot(this->m_normal, point);
    }

    void Plane::set(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
        this->set(calcNormalCCW(p0, p1, p2), p0);
    }

    void Plane::set(const float a, const float b, const float c, const float d) {
        this->m_normal.x = a;
        this->m_normal.y = b;
        this->m_normal.z = c;
        this->m_d = d;
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

    Triangle Triangle::transform(const glm::mat4& mat) const {
        const auto pp0 = mat * glm::vec4(this->point<0>(), 1);
        const auto pp1 = mat * glm::vec4(this->point<1>(), 1);
        const auto pp2 = mat * glm::vec4(this->point<2>(), 1);

        return Triangle{ pp0, pp1, pp2 };
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

    Sphere Sphere::transform(const glm::vec3& translate, const float scale) const {
        return Sphere{
            this->center() + translate,
            this->radius() * scale
        };
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

    AABB AABB::transform(const glm::vec3& translate, const float scale) const {
        return AABB{
            scale * this->min() + translate,
            scale * this->max() + translate
        };
    }


    std::array<glm::vec3, 8> AABB::getAllPoints(void) const {
        return this->getAllPoints([](auto vec) { return vec; });
    }

    std::array<glm::vec3, 8> AABB::getAllPoints(const glm::vec3& translate, const float scale) const {
        return this->getAllPoints([&translate, scale](auto vec) { return scale * vec + translate; });
    }

    std::array<glm::vec3, 8> AABB::getAllPoints(std::function<glm::vec3(const glm::vec3&)> modifier) const {
        std::array<glm::vec3, 8> result;

        {
            const auto p000 = this->min();
            const auto p111 = this->max();

            result[0] = modifier(p000);  // 000
            result[1] = modifier(glm::vec3{ p000.x, p000.y, p111.z });  // 001
            result[2] = modifier(glm::vec3{ p000.x, p111.y, p000.z });  // 010
            result[3] = modifier(glm::vec3{ p000.x, p111.y, p111.z });  // 011
            result[4] = modifier(glm::vec3{ p111.x, p000.y, p000.z });  // 100
            result[5] = modifier(glm::vec3{ p111.x, p000.y, p111.z });  // 101
            result[6] = modifier(glm::vec3{ p111.x, p111.y, p000.z });  // 110
            result[7] = modifier(p111);  // 111
        }

        return result;
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
            return seg.calcDistance(sphere.center()) <= sphere.radius();
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


    bool isIntersecting(const Plane& plane, const Sphere& sphere) {
        const auto distOfCenter = std::abs(plane.calcSignedDist(sphere.center()));
        return distOfCenter <= sphere.radius();
    }

    bool isIntersecting(const Plane& plane, const AABB& aabb) {
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


    bool isIntersecting(const Triangle& tri, const AABB& aabb) {
        const auto boxCenter = (aabb.min() + aabb.max()) * 0.5f;
        const auto boxHalfSize = (aabb.max() - aabb.min()) * 0.5f;
        const std::array<glm::vec3, 3> triverts = {
            tri.point<0>(),
            tri.point<1>(),
            tri.point<2>()
        };

        return fileadmin::triBoxOverlap(boxCenter, boxHalfSize, triverts);
    }


    bool isIntersecting(const Sphere& sphere, const AABB& aabb) {
        return doesCubeIntersectSphere(aabb.min(), aabb.max(), sphere.center(), sphere.radius());
    }

    bool isIntersecting(const AABB& one, const AABB& other) {
        const auto one1 = one.min();
        const auto one2 = one.max();
        const auto other1 = other.min();
        const auto other2 = other.max();

        if ( one2.x < other1.x ) return false;
        else if ( one1.x > other2.x ) return false;
        else if ( one2.y < other1.y ) return false;
        else if ( one1.y > other2.y ) return false;
        else if ( one2.z < other1.z ) return false;
        else if ( one1.z > other2.z ) return false;
        else return true;
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

        return SegIntersecInfo{ distance, distA > distB };
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
