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

    void makeTrianglesFromRect(const glm::vec3& p1, const glm::vec3& p2,
        const glm::vec3& p3, const glm::vec3& p4, dal::Triangle& tri1, dal::Triangle& tri2)
    {
        tri1 = dal::Triangle{ p1, p2, p3 };
        tri2 = dal::Triangle{ p1, p3, p4 };
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


    // From https://stackoverflow.com/questions/17458562/efficient-aabb-triangle-intersection-in-c-sharp
    template <unsigned _NumPoints>
    std::pair<double, double> Project(const std::array<glm::vec3, _NumPoints>& points, const glm::vec3& axis) {
        double min = std::numeric_limits<double>::infinity();
        double max = -std::numeric_limits<double>::infinity();

        for ( auto p : points ) {
            double val = glm::dot(axis, p);
            if ( val < min )
                min = val;
            if ( val > max )
                max = val;
        }

        return { min, max };
    }

    bool IsIntersecting(const dal::Triangle& triangle, const dal::AABB& box) {
        // Test the box normals (x-, y- and z-axes)
        static const std::array<glm::vec3, 3> boxNormals{
            glm::vec3{ 1, 0, 0 },
            glm::vec3{ 0, 1, 0 },
            glm::vec3{ 0, 0, 1 }
        };
        for ( int i = 0; i < 3; i++ ) {
            const glm::vec3 n = boxNormals[i];
            const auto [triangleMin, triangleMax] = ::Project(triangle.points(), boxNormals[i]);
            if ( triangleMax < box.min()[i] || triangleMin > box.max()[i] )
                return false; // No intersection possible.
        }

        // Test the triangle normal
        {
            const double triangleOffset = glm::dot(triangle.normal(), triangle.point0());
            const auto [boxMin, boxMax] = ::Project(box.vertices(), triangle.normal());
            if ( boxMax < triangleOffset || boxMin > triangleOffset )
                return false; // No intersection possible.
        }

        // Test the nine edge cross-products
        const std::array<glm::vec3, 3> triangleEdges{
            triangle.point0() - triangle.point1(),
            triangle.point1() - triangle.point2(),
            triangle.point2() - triangle.point0()
        };
        for ( int i = 0; i < 3; i++ ) {
            for ( int j = 0; j < 3; j++ ) {
                // The box normals are the same as it's edge tangents
                const glm::vec3 axis = glm::cross(triangleEdges[i], boxNormals[j]);
                const auto [boxMin, boxMax] = ::Project(box.vertices(), axis);
                const auto [triangleMin, triangleMax] = ::Project(triangle.points(), axis);
                if ( boxMax < triangleMin || boxMin > triangleMax )
                    return false; // No intersection possible
            }
        }

        // No separating axis found.
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

    Segment Segment::transform(const glm::mat4& trans) const {
        const auto spoint = glm::vec3{ trans * glm::vec4{ this->pos(), 1 } };
        const auto epoint = glm::vec3{ trans * glm::vec4{ this->endpoint(), 1 } };
        return dal::Segment{ spoint, epoint - spoint };
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

    Plane::Plane(const glm::vec4& p) {
        this->set(p);
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

    Plane Plane::transform(const glm::mat4& trans) const {
        const auto p = glm::transpose(glm::inverse(trans)) * this->coeff();
        return Plane{ p };
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

    void Plane::set(const glm::vec4& p) {
        this->set(p.x, p.y, p.z, p.w);
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

    std::array<dal::Segment, 3> Triangle::makeEdges(void) const {
        std::array<dal::Segment, 3> result;

        result[0].set(this->point0(), this->point1() - this->point0());
        result[1].set(this->point1(), this->point2() - this->point1());
        result[2].set(this->point2(), this->point0() - this->point2());

        return result;
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


    std::array<dal::Segment, 12> AABB::makeEdges(void) const {
        std::array<dal::Segment, 12> result;
        const auto [p000, p001, p010, p011, p100, p101, p110, p111] = this->vertices();

        result[0].set(p000, p100 - p000);
        result[1].set(p100, p101 - p100);
        result[2].set(p101, p001 - p101);
        result[3].set(p001, p000 - p001);

        result[4].set(p000, p010 - p000);
        result[5].set(p100, p110 - p100);
        result[6].set(p101, p111 - p101);
        result[7].set(p001, p011 - p001);

        result[8].set(p010, p110 - p010);
        result[9].set(p110, p111 - p110);
        result[10].set(p111, p011 - p111);
        result[11].set(p011, p010 - p011);

        return result;
    }

    std::array<dal::Triangle, 12> AABB::makeTriangles(void) const {
        const auto ps = this->vertices();
        std::array<dal::Triangle, 12> result;

        makeTrianglesFromRect(ps[3], ps[1], ps[5], ps[7], result[0], result[1]);
        makeTrianglesFromRect(ps[7], ps[5], ps[4], ps[6], result[2], result[3]);
        makeTrianglesFromRect(ps[6], ps[4], ps[0], ps[2], result[4], result[5]);
        makeTrianglesFromRect(ps[2], ps[0], ps[1], ps[3], result[6], result[7]);
        makeTrianglesFromRect(ps[2], ps[3], ps[7], ps[6], result[8], result[9]);
        makeTrianglesFromRect(ps[4], ps[5], ps[1], ps[0], result[10], result[11]);

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

        this->updateVertices(this->m_vertices);
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

        this->updateVertices(this->m_vertices);
    }

    // Private

    void AABB::updateVertices(std::array<glm::vec3, 8>& result) const {
        const auto p000 = this->min();
        const auto p111 = this->max();

        result[0] = p000;  // 000
        result[1] = glm::vec3{ p000.x, p000.y, p111.z };  // 001
        result[2] = glm::vec3{ p000.x, p111.y, p000.z };  // 010
        result[3] = glm::vec3{ p000.x, p111.y, p111.z };  // 011
        result[4] = glm::vec3{ p111.x, p000.y, p000.z };  // 100
        result[5] = glm::vec3{ p111.x, p000.y, p111.z };  // 101
        result[6] = glm::vec3{ p111.x, p111.y, p000.z };  // 110
        result[7] = p111;  // 111
    }

}


// OBB
namespace dal {

    OBB::OBB(const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& trans)
        : m_aabb(p0, p1)
        , m_trans(trans)
        , m_transInv(glm::inverse(trans))
    {

    }

    OBB::OBB(const dal::AABB& aabb, const glm::mat4& trans)
        : m_aabb(aabb)
        , m_trans(trans)
        , m_transInv(glm::inverse(trans))
    {

    }

    OBB OBB::transform(const glm::mat4& mat) const {
        auto result = *this;
        result.setTransMat(mat * result.transMat());
        return result;
    }

    void OBB::setTransMat(const glm::mat4& mat) {
        this->m_trans = mat;
        this->m_transInv = glm::inverse(mat);
    }

}


// Triangle Sorter
namespace dal {

    void TriangleSorter::add(const dal::Triangle& tri) {
        this->m_list.push_back(TrianglePair{ tri });
        std::sort_heap(this->m_list.begin(), this->m_list.end(), this->sortFunc);
    }

    void TriangleSorter::insert(const dal::Triangle* const begin, const dal::Triangle* const end) {
        for ( auto head = begin; end != head; ++head ) {
            this->add(*head);
        }
    }

    bool TriangleSorter::sortFunc(const TrianglePair& one, const TrianglePair& other) {
        return one.m_dot > other.m_dot;
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

    bool isIntersecting_old(const Segment& seg, const AABB& aabb) {
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

    bool isIntersecting(const Segment& seg, const AABB& aabb) {
        if ( aabb.isInside(seg.pos()) )
            return true;
        if ( aabb.isInside(seg.endpoint()) )
            return true;

        for ( auto& tri : aabb.makeTriangles() ) {
            if ( dal::isIntersecting(seg, tri) )
                return true;
        }

        return false;
    }

    bool isIntersecting(const Segment& seg, const OBB& obb) {
        const auto newSeg = seg.transform(obb.transMatInv());
        return dal::isIntersecting(newSeg, obb.aabb());
    }


    bool isIntersecting(const Plane& plane, const Sphere& sphere) {
        const auto distOfCenter = std::abs(plane.calcSignedDist(sphere.center()));
        return distOfCenter <= sphere.radius();
    }

    bool isIntersecting(const Plane& plane, const AABB& aabb) {
        const auto firstOne = plane.isInFront(aabb.vertices()[0]);

        for ( size_t i = 1; i < aabb.vertices().size(); ++i ) {
            const auto thisOne = plane.isInFront(aabb.vertices()[i]);
            if ( firstOne != thisOne ) {
                return true;
            }
        }

        return false;
    }

    bool isIntersecting(const Plane& plane, const OBB& obb) {
        const auto newPlane = plane.transform(obb.transMatInv());
        return dal::isIntersecting(newPlane, obb.aabb());
    }


    bool isIntersecting_naive(const Triangle& tri, const AABB& aabb) {
        for ( const auto& edge : tri.makeEdges() ) {
            if ( dal::isIntersecting(edge, aabb) ) {
                return true;
            }
        }

        for ( const auto& edge : aabb.makeEdges() ) {
            if ( dal::isIntersecting(edge, tri) ) {
                return true;
            }
        }

        return false;
    }

    bool isIntersecting(const Triangle& tri, const AABB& aabb) {
        return ::IsIntersecting(tri, aabb);
    }

    bool isIntersecting(const Triangle& tri, const OBB& obb) {
        const auto newTri = tri.transform(obb.transMatInv());
        return ::IsIntersecting(newTri, obb.aabb());
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

    bool isIntersecting(const AABB& aabb, const TriangleSoup& soup, const glm::mat4& trans) {
        for ( const auto& tri : soup ) {
            const auto newTri = tri.transform(trans);
            if ( dal::isIntersecting(newTri, aabb) ) {
                return true;
            }
        }
        return false;
    }

}


// Misc
namespace dal {

    std::pair<float, glm::vec3> calcIntersectingDepth(const AABB& aabb, const Plane& plane) {
        float smallestDistValue = 0;

        for ( auto& p : aabb.vertices() ) {
            const auto signedDist = plane.calcSignedDist(p);
            if ( signedDist < smallestDistValue ) {
                smallestDistValue = signedDist;
            }
        }

        return { std::abs(smallestDistValue), -plane.normal() };
    }

    size_t getIntersectingTriangles(const AABB& aabb, const TriangleSoup& soup, const glm::mat4& trans, std::vector<dal::Triangle>& result) {
        for ( const auto& tri : soup ) {
            const auto newTri = tri.transform(trans);
            if ( dal::isIntersecting(newTri, aabb) ) {
                result.push_back(newTri);
            }
        }
        return result.size();
    }

    std::vector<dal::Triangle> getIntersectingTriangles(const AABB& aabb, const TriangleSoup& soup, const glm::mat4& trans) {
        std::vector<dal::Triangle> result;
        dal::getIntersectingTriangles(aabb, soup, trans, result);
        return result;
    }

}


// Calc intersection info
namespace dal {

    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Plane& plane) {
        const auto pointA = seg.pos();
        const auto pointB = pointA + seg.rel();

        const auto distA = plane.calcSignedDist(pointA);
        const auto distB = plane.calcSignedDist(pointB);

        if ( (distA * distB) > 0.f )
            return std::nullopt;

        const auto absDistA = std::abs(distA);
        const auto denominator = absDistA + std::abs(distB);
        if ( 0.f == denominator )
            return SegIntersecInfo{ 0, distA > distB };

        const auto distance = seg.length() * absDistA / denominator;
        return SegIntersecInfo{ distance, distA > distB };
    }

    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const Triangle& tri, const bool ignoreFromBack) {
        const auto plane = tri.plane();
        const auto planeCol = dal::findIntersection(seg, plane);

        if ( !planeCol )
            return std::nullopt;
        if ( ignoreFromBack && !planeCol->m_isFromFront )
            return std::nullopt;

        const auto collisionPoint = seg.pos() + glm::normalize(seg.rel()) * planeCol->m_distance;
        if ( ::isPointInsideTri(collisionPoint, tri) ) {
            return planeCol;
        }

        return std::nullopt;
    }

    std::optional<SegIntersecInfo> findIntersection(const Segment& seg, const AABB& aabb) {
        if ( 0.f == aabb.volume() )
            return std::nullopt;

        std::optional<SegIntersecInfo> result = std::nullopt;
        float maxDist = std::numeric_limits<float>::max();

        const auto triangles = aabb.makeTriangles();
        for ( auto& tri : triangles ) {
            const auto triCol = dal::findIntersection(seg, tri, false);
            if ( triCol && triCol->m_distance < maxDist ) {
                result = triCol;
                maxDist = triCol->m_distance;
            }
        }

        return result;
    }

}


namespace {

    template <size_t _ArrSize>
    size_t minValueIndex(const float* const arr) {
        static_assert(_ArrSize > 1);

        float minValue = arr[0];
        size_t minIndex = 0;

        for ( size_t i = 1; i < _ArrSize; ++i ) {
            if ( arr[i] < minValue ) {
                minValue = arr[i];
                minIndex = i;
            }
        }

        return minIndex;
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
        switch ( ::minValueIndex<3>(selector) ) {

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


// Resolve collision of aabb againt static objects
namespace dal {

    glm::vec3 calcResolveForAABB(const dal::MovingAABBInfo& aabb, const dal::AABB& other) {
        const auto newAABB = aabb.m_aabb.transform(aabb.m_thisPos, aabb.m_thisScale);

        if ( !dal::isIntersecting(newAABB, other) ) {
            return glm::vec3{ 0 };
        }

        const auto result = calcResolveInfo_withMinMax(newAABB.min(), newAABB.max(), other.min(), other.max(), 1, 0);
        if ( !result.m_valid )
            return glm::vec3{ 0 };

        return result.m_this;
    }

}
