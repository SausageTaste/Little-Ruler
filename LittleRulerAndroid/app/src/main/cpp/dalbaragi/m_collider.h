#pragma once

#include <array>
#include <vector>
#include <variant>
#include <optional>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


// Structs
namespace dal {

    struct CollisionResolveInfo {
        glm::vec3 m_this, m_other;
    };

    struct RayCastingResult {
        bool m_isFromFront = false;
        float m_distance = 0.0f;
    };


    class Transform {

    private:
        struct MatCache {
            bool m_needUpdate = true;
            glm::mat4 m_mat;
        };

    private:
        mutable MatCache m_mat;
        glm::quat m_quat;
        glm::vec3 m_pos;
        float m_scale;
        
    public:
        Transform(void);
        Transform(const glm::vec3& pos, const glm::quat& quat, const float scale);

        const glm::mat4& getMat(void) const;
        const glm::vec3& getPos(void) const noexcept {
            return this->m_pos;
        }
        float getScale(void) const noexcept {
            return this->m_scale;
        }

        void setPos(const glm::vec3& v) noexcept {
            this->m_pos = v;
            this->setNeedUpdate();
        }
        void setPos(const float x, const float y, const float z) noexcept {
            this->m_pos.x = x;
            this->m_pos.y = y;
            this->m_pos.z = z;
            this->setNeedUpdate();
        }
        void addPos(const glm::vec3& v) {
            this->m_pos += v;
            this->setNeedUpdate();
        }
        void addPos(const float v) {
            this->m_pos += v;
            this->setNeedUpdate();
        }

        void setQuat(const glm::quat& q) {
            this->m_quat = q;
            this->setNeedUpdate();
        }
        void rotate(const float v, const glm::vec3& selector);
        void setScale(const float v) {
            this->m_scale = v;
            this->setNeedUpdate();
        }

    private:
        void updateMat(void) const;
        void setNeedUpdate(void) {
            this->m_mat.m_needUpdate = true;
        }
        bool needUpdate(void) const {
            return this->m_mat.m_needUpdate;
        }

    };


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

    public:
        virtual ~ICollider(void) = default;
        virtual ColliderType getColType(void) const noexcept = 0;

    };

}


// Primitive colliders
namespace dal {

    class Ray {

    private:
        glm::vec3 m_pos, m_rel;
        float m_len;

    public:
        Ray(void);
        Ray(const glm::vec3& pos, const glm::vec3& rel);

        const glm::vec3& getStartPos(void) const {
            return this->m_pos;
        }
        const glm::vec3& getRel(void) const {
            return this->m_rel;
        }
        float getLength(void) const {
            return this->m_len;
        }
        void setStartPos(const glm::vec3& v) {
            this->m_pos = v;
        }
        void setRel(const glm::vec3& v);

    };


    class Plane {

    private:
        glm::vec4 m_coeff;

    public:
        Plane(void);
        Plane(const glm::vec3& normal, const glm::vec3& point);
        Plane(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
        Plane(const float a, const float b, const float c, const float d);

        glm::vec3 getNormal(void) const {
            return glm::vec3{ this->m_coeff.x, this->m_coeff.y, this->m_coeff.z };
        }

        float getSignedDist(const glm::vec3 v) const {
            const auto numerator = glm::dot(this->m_coeff, glm::vec4{v, 1.0f});
            const auto denominatorInv = glm::inversesqrt(
                this->m_coeff.x * this->m_coeff.x + this->m_coeff.y * this->m_coeff.y  + this->m_coeff.z * this->m_coeff.z
            );
            return numerator * denominatorInv;
        }
        bool isInFront(const glm::vec3 v) const {
            return 0.0f < glm::dot(this->m_coeff, glm::vec4{v, 1.0f});
        }

    };


    class Triangle {

    private:
        glm::vec3 m_points[3];

    public:
        Triangle(void) = default;
        Triangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

        const glm::vec3& getPoint1(void) const {
            return this->m_points[0];
        }
        const glm::vec3& getPoint2(void) const {
            return this->m_points[1];
        }
        const glm::vec3& getPoint3(void) const {
            return this->m_points[2];
        }

    private:
        float calcArea(void) const;

    };


    class Sphere {

    private:
        glm::vec3 m_center;
        float m_radius;

    public:
        Sphere(void);
        Sphere(const glm::vec3& center, const float radius);

        const glm::vec3& getCenter(void) const noexcept {
            return this->m_center;
        }
        float getRadius(void) const noexcept {
            return this->m_radius;
        }

        float getDistance(const glm::vec3& p) const;
        bool isInside(const glm::vec3& p) const;

    };


    class AABB {

    private:
        glm::vec3 m_p1, m_p2;

    public:
        AABB(void) = default;
        AABB(const glm::vec3& p1, const glm::vec3& p2);

        glm::vec3 getPoint000(void) const {
            return this->m_p1;
        }
        glm::vec3 getPoint111(void) const {
            return this->m_p2;
        }

        // The order is
        // 000, 001, 010, 011, 100, 101, 110, 111
        // Each digit means x, y, z, 0 means lower value on the axis, 1 means higher.
        std::array<glm::vec3, 8> getAllPoints(void) const;
        std::array<glm::vec3, 8> getAllPoints(const Transform & trans) const;
        std::array<glm::vec3, 8> getAllPoints(std::function<glm::vec3(const glm::vec3&)> modifier) const;

        void set(const glm::vec3& p1, const glm::vec3& p2);
        AABB transform(const Transform& trans) const;
        //void add(const glm::vec3& offset);
        //void scale(const float mag);

        float calcArea(void) const;

    private:
        void validateOrder(void);

    };

}


// Polymorphic colliders
namespace dal {

    class ColSphere : public Sphere, public ICollider {

    public:
        virtual ColliderType getColType(void) const noexcept override {
            return ColliderType::sphere;
        }

    };


    class ColAABB : public AABB, public ICollider {

    public:
        ColAABB(void) = default;
        ColAABB(const glm::vec3& p1, const glm::vec3& p2)
            : AABB(p1, p2)
        {

        }

        virtual ColliderType getColType(void) const noexcept override {
            return ColliderType::aabb;
        }

    };

}


// checkCollision funcs
namespace dal {

    bool checkCollisionAbs(const ICollider& one, const ICollider& two, const Transform& transOne, const Transform& transTwo);

    bool checkCollision(const Ray& ray, const Triangle& tri);
    bool checkCollision(const Ray& ray, const Plane& plane);
    bool checkCollision(const Ray& ray, const AABB& aabb);
    bool checkCollision(const Ray& ray, const AABB& aabb, const Transform& transAABB);
    
    bool checkCollision(const Plane& plane, const AABB& aabb);
    bool checkCollision(const Plane& plane, const AABB& aabb, const Transform& transAABB);

    bool checkCollision(const AABB& one, const AABB& other);
    bool checkCollision(const AABB& one, const AABB& two, const Transform& transOne, const Transform& transTwo);
    
}


// calcResolveInfo funcs
namespace dal {

    CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other, const PhysicalProperty& physicsOne, const PhysicalProperty& physicsTwo);
    CollisionResolveInfo calcResolveInfo(const AABB& one, const AABB& other, const PhysicalProperty& physicsOne, const PhysicalProperty& physicsTwo,
        const Transform& transOne, const Transform& transTwo);

}


// calcCollisionInfo funcs
namespace dal {

    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const Triangle& tri, const bool ignoreFromBack = false);
    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const Plane& plane);
    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const AABB& aabb);
    std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray, const AABB& aabb, const Transform& transAABB);

}


// Complex colliders
namespace dal {

    class ColTriangleSoup : public ICollider {

    private:
        std::vector<Triangle> m_triangles;
        bool m_faceCull = true;

    public:
        virtual ColliderType getColType(void) const noexcept override {
            return ColliderType::triangle_soup;
        }

        void addTriangle(const Triangle& tri) {
            this->m_triangles.push_back(tri);
        }

        std::optional<RayCastingResult> calcCollisionInfo(const Ray& ray) const;

    };

}
