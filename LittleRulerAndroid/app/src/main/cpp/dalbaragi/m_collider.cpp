#include "m_collider.h"

#include <tuple>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"
#include "u_vecutil.h"


using namespace fmt::literals;


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

    inline void makeTrianglesFromRect(const glm::vec3& p1, const glm::vec3 & p2,
        const glm::vec3 & p3, const glm::vec3 & p4, dal::Triangle& tri1, dal::Triangle& tri2)
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

    std::array<dal::Triangle, 12> makeTriangles(const dal::AABB& aabb, const dal::Transform & transAABB) {
        std::array<dal::Triangle, 12> result;

        const auto ps = aabb.getAllPoints(transAABB);

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
        const auto edge1 = tri.getPoint2() - tri.getPoint1();
        const auto edge2 = tri.getPoint3() - tri.getPoint2();
        
        const auto toPoint1 = p - tri.getPoint1();
        const auto toPoint2 = p - tri.getPoint2();
        
        const auto crossed1 = glm::cross(edge1, toPoint1);
        const auto crossed2 = glm::cross(edge2, toPoint2);
        
        const auto dotted1 = glm::dot(crossed1, crossed2);
        
        if ( dotted1 < 0.0f ) {
            return false;
        }

        const auto edge3 = tri.getPoint1() - tri.getPoint3();
        const auto toPoint3 = p - tri.getPoint3();
        const auto crossed3 = glm::cross(edge3, toPoint3);
        const auto dotted2 = glm::dot(crossed1, crossed3);

        if ( dotted2 < 0.0f ) {
            return false;
        }

        return true;
    }

}


// Collider resolver
namespace {

    class ColliderResolver {

    private:
        using checkColFunc_t = bool (*)(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo);

    private:
        static constexpr unsigned int k_numColliders = static_cast<unsigned int>(dal::ColliderType::eoe);
        checkColFunc_t m_checkColTable[k_numColliders][k_numColliders];

    public:
        ColliderResolver(void) {
            this->getItem(dal::ColliderType::sphere, dal::ColliderType::sphere) = nullptr;
            this->getItem(dal::ColliderType::sphere, dal::ColliderType::aabb) = checkCol_sphere_aabb;
            this->getItem(dal::ColliderType::sphere, dal::ColliderType::triangle_soup) = nullptr;

            this->getItem(dal::ColliderType::aabb, dal::ColliderType::sphere) = checkCol_aabb_sphere;
            this->getItem(dal::ColliderType::aabb, dal::ColliderType::aabb) = checkCol_aabb_aabb;
            this->getItem(dal::ColliderType::aabb, dal::ColliderType::triangle_soup) = checkCol_aabb_trisoup;

            this->getItem(dal::ColliderType::triangle_soup, dal::ColliderType::sphere) = nullptr;
            this->getItem(dal::ColliderType::triangle_soup, dal::ColliderType::aabb) = nullptr;
            this->getItem(dal::ColliderType::triangle_soup, dal::ColliderType::triangle_soup) = nullptr;
        }

        bool checkCollision(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) const {
            const auto pFunc = this->getItem(one.getColType(), two.getColType());
            if ( nullptr == pFunc ) {
                dalWarn("No function registered for those colliders: {}, {}"_format(this->getIndexOf(one.getColType()), this->getIndexOf(two.getColType())));
                return false;
            }
            else {
                return pFunc(one, two, transOne, transTwo);
            }
        }

    private:
        static unsigned int getIndexOf(const dal::ColliderType e) {
            return static_cast<unsigned int>(e);
        }

        checkColFunc_t& getItem(const dal::ColliderType one, const dal::ColliderType two) {
            return this->m_checkColTable[this->getIndexOf(one)][this->getIndexOf(two)];
        }

        const checkColFunc_t& getItem(const dal::ColliderType one, const dal::ColliderType two) const {
            return this->m_checkColTable[this->getIndexOf(one)][this->getIndexOf(two)];
        }

    private:
        static bool checkCol_sphere_aabb(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& sphere = reinterpret_cast<const dal::ColSphere&>(one);
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(two);
            return dal::checkCollision(sphere, aabb, transOne, transTwo);
        }

        static bool checkCol_aabb_sphere(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& sphere = reinterpret_cast<const dal::ColSphere&>(two);
            return dal::checkCollision(sphere, aabb, transTwo, transOne);
        }

        static bool checkCol_aabb_aabb(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& oneAABB = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& twoAABB = reinterpret_cast<const dal::ColAABB&>(two);
            return dal::checkCollision(oneAABB, twoAABB, transOne, transTwo);
        }

        static bool checkCol_aabb_trisoup(const dal::ICollider& one, const dal::ICollider& two, const dal::Transform& transOne, const dal::Transform& transTwo) {
            const auto& aabb = reinterpret_cast<const dal::ColAABB&>(one);
            const auto& soup = reinterpret_cast<const dal::ColTriangleSoup&>(two);
            return dal::checkCollision(aabb, soup, transOne, transTwo);
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

    bool checkCollision_rayVsMinMax(const dal::Ray& ray, const glm::vec3& aabbMin, const glm::vec3& aabbMax) {
        // From https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

        const glm::vec3 bounds[2] = { aabbMin, aabbMax };
        const auto orig = ray.getStartPos();
        const auto invdir = 1.f / ray.getRel();
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
            return dal::CollisionResolveInfo{ { xForThis, 0.0f, 0.0f }, { xForOther, 0.0f, 0.0f } };
        case 1:
            return dal::CollisionResolveInfo{ { 0.0f, yForThis, 0.0f }, { 0.0f, yForOther, 0.0f } };
        case 2:
            return dal::CollisionResolveInfo{ { 0.0f, 0.0f, zForThis }, { 0.0f, 0.0f, zForOther } };
        default:
            dalAbort("This can't happen!");

        }
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


// Transform
namespace dal {

    Transform::Transform(void)
        : m_scale(1.f)
        , m_isDefault(true)
    {
        this->updateMat();
    }

    Transform::Transform(const glm::vec3& pos, const glm::quat& quat, const float scale)
        : m_quat(quat)
        , m_pos(pos)
        , m_scale(scale)
        , m_isDefault(false)
    {
        this->updateMat();
    }

    const glm::mat4& Transform::getMat(void) const {
        if ( this->needUpdate() ) {
            this->updateMat();
        }
        return this->m_mat.m_mat;
    }

    void Transform::rotate(const float v, const glm::vec3& selector) {
        this->m_quat = rotateQuat(this->m_quat, v, selector);
        this->onValueSet();
    }

    // Private

    void Transform::updateMat(void) const {
        const auto identity = glm::mat4{ 1.0f };
        const auto scaleMat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
        const auto translateMat = glm::translate(identity, this->m_pos);
        this->m_mat.m_mat = translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
        this->m_mat.m_needUpdate = false;
    }

}


// ICollider
namespace dal {

    ICollider::ICollider(const ColliderType type)
        : m_type(type)
    {
        dalAssert(ColliderType::eoe != this->m_type);
    }

    ColliderType ICollider::getColType(void) const noexcept {
        return this->m_type;
    }

}


// Plane
namespace dal {

    Plane::Plane(void)
        : m_coeff(0.0f, 1.0f, 0.0f, 0.0f)
    {

    }

    Plane::Plane(const glm::vec3& normal, const glm::vec3& point)
        : m_coeff(normal.x, normal.y, normal.z, -glm::dot(normal, point))
    {

    }

    Plane::Plane(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
        : Plane(glm::cross(p2 - p1, p3 - p1), p1)
    {

    }

    Plane::Plane(const float a, const float b, const float c, const float d)
        : m_coeff(a, b, c, d)
    {

    }

}


// Ray
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

    void Ray::setRel(const glm::vec3 v) {
        this->m_rel = v;
        this->m_len = glm::length(this->m_rel);
        dalAssert(this->m_len > 0.0f);
    }

    glm::vec3 Ray::projectPointOn(const glm::vec3 p) const {
        // From https://answers.unity.com/questions/62644/distance-between-a-ray-and-a-point.html

        const auto rhs = p - this->m_pos;
        const float magnitude = this->getLength();

        auto lhs = this->m_rel;
        if ( magnitude > 1E-06f ) {
            lhs /= magnitude;
        }
        float num2 = std::clamp(glm::dot(lhs, rhs), 0.f, magnitude);
        return this->m_pos + (lhs * num2);
    }

    float Ray::calcDistance(const glm::vec3 p) const {
        const auto projected = this->projectPointOn(p);
        return glm::length(projected - p);
    }

}


// Triangle
namespace dal {

    Triangle::Triangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
        this->m_points[0] = p1;
        this->m_points[1] = p2;
        this->m_points[2] = p3;

        if ( this->calcArea() == 0.0f ) {
            dalAbort("Triangle area is zero!");
        }
    }

    glm::vec3 Triangle::calcNormal(void) const {
        const auto edge1 = this->m_points[2] - this->m_points[1];
        const auto edge2 = this->m_points[0] - this->m_points[1];
        return glm::cross(edge1, edge2);
    }

    Triangle Triangle::transform(const Transform& trans) const {
        const auto& mat = trans.getMat();

        const auto pp1 = mat * glm::vec4(this->getPoint1(), 1.0f);
        const auto pp2 = mat * glm::vec4(this->getPoint2(), 1.0f);
        const auto pp3 = mat * glm::vec4(this->getPoint3(), 1.0f);

        return Triangle{ pp1, pp2, pp3 };
    }

    // Private

    float Triangle::calcArea(void) const {
        const auto a = glm::length(this->getPoint2() - this->getPoint1());
        const auto b = glm::length(this->getPoint3() - this->getPoint2());
        const auto c = glm::length(this->getPoint1() - this->getPoint3());

        const auto s = (a + b + c) * 0.5f;
        return std::sqrt(s * (s - a) * (s - b) * (s - c));
    }

}


// AABB
namespace dal {

    AABB::AABB(const glm::vec3& p1, const glm::vec3& p2)
        : m_p1(p1), m_p2(p2)
    {
        this->validateOrder();
    }

    std::array<glm::vec3, 8> AABB::getAllPoints(void) const {
        return this->getAllPoints([](auto vec) { return vec; });
    }

    std::array<glm::vec3, 8> AABB::getAllPoints(const Transform& trans) const {
        return this->getAllPoints([&trans](auto vec) { return trans.getScale() * vec + trans.getPos(); });
    }

    std::array<glm::vec3, 8> AABB::getAllPoints(std::function<glm::vec3(const glm::vec3&)> modifier) const {
        std::array<glm::vec3, 8> result;

        {
            const auto p000 = this->getPoint000();
            const auto p111 = this->getPoint111();

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


    void AABB::set(const glm::vec3& p1, const glm::vec3& p2) {
        this->m_p1 = p1;
        this->m_p2 = p2;

        this->validateOrder();
    }

    AABB AABB::transform(const Transform& trans) const {
        return AABB{
            trans.getScale() * this->m_p1 + trans.getPos(),
            trans.getScale() * this->m_p2 + trans.getPos()
        };
    }

    void AABB::resizeToInclude(const float x, const float y, const float z) {
        if ( this->m_p1.x > x ) {
            this->m_p1.x = x;
        }
        else if ( this->m_p2.x < x ) {
            this->m_p2.x = x;
        }

        if ( this->m_p1.y > y ) {
            this->m_p1.y = y;
        }
        else if ( this->m_p2.y < y ) {
            this->m_p2.y = y;
        }

        if ( this->m_p1.z > z ) {
            this->m_p1.z = z;
        }
        else if ( this->m_p2.z < z ) {
            this->m_p2.z = z;
        }
    }

    /*
    void AABB::add(const glm::vec3& offset) {
        this->m_p1 += offset;
        this->m_p2 += offset;
    }

    void AABB::scale(const float mag) {
        this->m_p1 *= mag;
        this->m_p2 *= mag;
    }
    */

    float AABB::calcArea(void) const {
        return (this->m_p2.x - this->m_p1.x) * (this->m_p2.y - this->m_p1.y) * (this->m_p2.z - this->m_p1.z);
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


// Sphere
namespace dal {

    Sphere::Sphere(void)
        : m_radius(1.f)
    {

    }

    Sphere::Sphere(const glm::vec3& center, const float radius)
        : m_center(center)
        , m_radius(radius)
    {

    }

    float Sphere::getDistance(const glm::vec3& p) const {
        return glm::distance(this->m_center, p) - this->m_radius;
    }

    bool Sphere::isInside(const glm::vec3& p) const {
        const auto rel = p - this->m_center;
        return glm::dot(rel, rel) <= (this->m_radius * this->m_radius);
    }

    Sphere Sphere::transform(const Transform& trans) const {
        Sphere result;

        if ( glm::dot(this->m_center, this->m_center) != 0.0f ) {
            dalWarn("Sphere's transformation is defined for only one's center is at origin.");
        }

        result.setCenter(this->m_center + trans.getPos());
        result.setRadius(this->m_radius * trans.getScale());

        return result;
    }

    void Sphere::resizeToInclude(const glm::vec3 p) {
        const auto dist = this->getDistance(p);
        if ( dist > this->m_radius ) {
            this->m_radius = dist;
        }
    }

}


// checkCollision funcs
namespace dal {

    bool checkCollisionAbs(const ICollider& one, const ICollider& two, const Transform& transOne, const Transform& transTwo) {
        return g_colResolver.checkCollision(one, two, transOne, transTwo);
    }

    bool checkCollisionAbs(const Ray& ray, const ICollider& col, const Transform& transCol) {
        const auto colType = col.getColType();
        switch ( colType ) {

        case dal::ColliderType::sphere:
        {
            const auto newSphere = reinterpret_cast<const ColSphere&>(col).transform(transCol);
            return checkCollision(ray, newSphere);
        }
        case dal::ColliderType::aabb:
        {
            const auto newAABB = reinterpret_cast<const ColAABB&>(col);
            return checkCollision(ray, newAABB, transCol);
        }
        case dal::ColliderType::triangle_soup:
        {
            dalAbort("Not implemented.");
        }
        default:
            dalAbort("Unkown collider type code: {}"_format(static_cast<int>(colType)));

        }
    }


    bool checkCollision(const Ray& ray, const Plane& plane) {
        const auto pointA = ray.getStartPos();
        const auto pointB = pointA + ray.getRel();

        const auto distA = plane.getSignedDist(pointA);
        const auto distB = plane.getSignedDist(pointB);

        return (distA * distB) <= 0.0f;
    }

    bool checkCollision(const Ray& ray, const Sphere& sphere) {
        if ( sphere.isInside(ray.getStartPos()) ) {
            return true;
        }
        else if ( sphere.isInside(ray.getStartPos() + ray.getRel()) ) {
            return true;
        }
        else {
            return ray.calcDistance(sphere.getCenter()) <= sphere.getRadius();
        }
    }

    bool checkCollision(const Ray& ray, const Sphere& sphere, const Transform& transSphere) {
        return checkCollision(ray, sphere.transform(transSphere));
    }

    bool checkCollision(const Ray& ray, const Triangle& tri) {
        const Plane plane{ tri.getPoint1(), tri.getPoint2(), tri.getPoint3() };
        const auto planeCollision = calcCollisionInfo(ray, plane);

        if ( !planeCollision ) {
            return false;
        }
        else {
            const auto collisionPoint = ray.getStartPos() + glm::normalize(ray.getRel()) * planeCollision->m_distance;
            return isPointInsideTriangle(collisionPoint, tri);
        }
    }

    bool checkCollision(const Ray& ray, const AABB& aabb) {
        return checkCollision_rayVsMinMax(ray, aabb.getPoint000(), aabb.getPoint111());
    }

    bool checkCollision(const Ray& ray, const AABB& aabb, const Transform& transAABB) {
        return checkCollision_rayVsMinMax(ray,
            transAABB.getScale() * aabb.getPoint000() + transAABB.getPos(),
            transAABB.getScale() * aabb.getPoint111() + transAABB.getPos()
        );
    }


    bool checkCollision(const Plane& plane, const Sphere& sphere) {
        const auto distOfCenter = std::abs(plane.getSignedDist(sphere.getCenter()));
        return distOfCenter <= sphere.getRadius();
    }

    bool checkCollision(const Plane& plane, const Sphere& sphere, const Transform& transSphere) {
        return checkCollision(plane, sphere.transform(transSphere));
    }

    bool checkCollision(const Plane& plane, const AABB& aabb) {
        const auto points = aabb.getAllPoints();
        return checkCollision_withAllPoints(points, plane);
    }

    bool checkCollision(const Plane& plane, const AABB& aabb, const Transform& transAABB) {
        const auto points = aabb.getAllPoints(transAABB);
        return checkCollision_withAllPoints(points, plane);
    }


    bool checkCollision(const Triangle& tri, const AABB& aabb, const Transform& transTri, const Transform& transAABB) {
        const auto newTri = tri.transform(transTri);
        const auto newBox = aabb.transform(transAABB);

        const auto boxCenter = (newBox.getPoint000() + newBox.getPoint111()) * 0.5f;
        const auto boxHalfSize = (newBox.getPoint111() - newBox.getPoint000()) * 0.5f;
        const std::array<glm::vec3, 3> triverts = {
            newTri.getPoint1(),
            newTri.getPoint2(),
            newTri.getPoint3()
        };

        return fileadmin::triBoxOverlap(boxCenter, boxHalfSize, triverts);
    }


    bool checkCollision(const Sphere& sphere, const AABB& aabb, const Transform& transSphere, const Transform& transAABB) {
        // TODO : Sphere's transform is not correct atm.

        const auto newAABB = aabb.transform(transAABB);

        return doesCubeIntersectSphere(newAABB.getPoint000(), newAABB.getPoint111(), sphere.getCenter() + transSphere.getPos(), sphere.getRadius());
    }


    bool checkCollision(const AABB& one, const AABB& other) {
        const auto one1 = one.getPoint000();
        const auto one2 = one.getPoint111();
        const auto other1 = other.getPoint000();
        const auto other2 = other.getPoint111();

        return checkCollisionWithMinMax(one1, one2, other1, other2);
    }

    bool checkCollision(const AABB& one, const AABB& two, const Transform& transOne, const Transform& transTwo) {
        const auto one1 = transOne.getScale() * one.getPoint000() + transOne.getPos();
        const auto one2 = transOne.getScale() * one.getPoint111() + transOne.getPos();
        const auto two1 = transTwo.getScale() * two.getPoint000() + transTwo.getPos();
        const auto two2 = transTwo.getScale() * two.getPoint111() + transTwo.getPos();

        return checkCollisionWithMinMax(one1, one2, two1, two2);
    }

    bool checkCollision(const AABB& aabb, const ColTriangleSoup triSoup, const Transform& transAABB, const Transform& transTriSoup) {
        for ( auto& tri : triSoup ) {
            if ( dal::checkCollision(tri, aabb, transTriSoup, transAABB) ) {
                return true;
            }
        }
        return false;
    }

}


// calcResolveInfo funcs
namespace dal {

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

    CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other, const PhysicalProperty& physicsOne, const PhysicalProperty& physicsTwo,
        const Transform& transOne, const Transform& transTwo)
    {
        const auto sumOfMassInv = physicsOne.getMassInv() + physicsTwo.getMassInv();
        if ( sumOfMassInv == 0.0f ) {
            return CollisionResolveInfo{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        }

        const auto thisFactor = physicsOne.getMassInv() / sumOfMassInv;
        const auto otherFactor = physicsTwo.getMassInv() / sumOfMassInv;

        const auto one1 = transOne.getScale() * one.getPoint000() + transOne.getPos();
        const auto one2 = transOne.getScale() * one.getPoint111() + transOne.getPos();
        const auto other1 = transTwo.getScale() * other.getPoint000() + transTwo.getPos();
        const auto other2 = transTwo.getScale() * other.getPoint111() + transTwo.getPos();

        return calcResolveInfo_withMinMax(one1, one2, other1, other2, thisFactor, otherFactor);
    }

}


// calcCollisionInfo funcs
namespace dal {

    std::optional<RayCastingResult> calcCollisionInfoAbs(const Ray& ray, const ICollider& col, const Transform& transCol) {
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
            dalAbort("Unkown collider type code: {}"_format(static_cast<int>(colType)));

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

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const Triangle& tri, const bool ignoreFromBack) {
        Plane plane{ tri.getPoint1(), tri.getPoint2(), tri.getPoint3() };
        const auto planeCol = calcCollisionInfo(ray, plane);
        if ( !planeCol ) {
            return std::nullopt;
        }
        else if ( ignoreFromBack && !planeCol->m_isFromFront ) {
            return std::nullopt;
        }

        if ( checkCollision(ray, tri) ) {
            return planeCol;
        }
        else {
            return std::nullopt;
        }
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const Sphere& sphere, const Transform& transSphere) {
        dalAbort("Not implemented.");
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const AABB& aabb) {
        if ( aabb.calcArea() == 0.0f ) {
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

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const AABB& aabb, const Transform& transAABB) {
        if ( aabb.calcArea() == 0.0f ) {
            return std::nullopt;
        }

        const auto triangles = makeTriangles(aabb.getAllPoints(transAABB));
        for ( auto& tri : triangles ) {
            const auto triCol = calcCollisionInfo(ray, tri);
            if ( triCol ) {
                return triCol;
            }
        }

        return std::nullopt;
    }

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const ColTriangleSoup triSoup, const Transform& transTriSoup) {
        std::optional<RayCastingResult> result{ std::nullopt };
        float leastDistance = std::numeric_limits<float>::max();

        for ( const auto& tri : triSoup ) {
            const auto newTri = tri.transform(transTriSoup);
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
