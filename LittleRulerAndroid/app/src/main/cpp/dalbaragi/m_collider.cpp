#include "m_collider.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    size_t minIndex(const float* const arr, const size_t arrSize) {
        if ( arrSize == 0 ) {
            dalAbort("Are you mad?");
        }
        else if ( 1 == arrSize ) return 0;
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

    void AABB_2D::setPoints(const glm::vec2& p1, const glm::vec2& p2) {
        this->p1 = p1;
        this->p2 = p2;
        this->validateOrder();
    }

    bool AABB_2D::isInside(const glm::vec2& p) const {
        if ( p.x < p1.x ) return false;
        if ( p.x > p2.x ) return false;
        if ( p.y < p1.y ) return false;
        if ( p.y > p2.y ) return false;

        return true;
    }

    void AABB_2D::validateOrder(void) {
        if ( p1.x > p2.x ) {
            std::swap(p1.x, p2.x);
        }
        if ( p1.y > p2.y ) {
            std::swap(p1.y, p2.y);
        }
    }

}


namespace dal {

    AxisAlignedBoundingBox::AxisAlignedBoundingBox(const glm::vec3& p1, const glm::vec3& p2)
        : m_p1(p1), m_p2(p2)
    {
        this->validateOrder();
    }

    void AxisAlignedBoundingBox::set(const glm::vec3& p1, const glm::vec3& p2) {
        this->m_p1 = p1;
        this->m_p2 = p2;
    }

    void AxisAlignedBoundingBox::add(const glm::vec3& offset) {
        this->m_p1 += offset;
        this->m_p2 += offset;
    }

    bool AxisAlignedBoundingBox::checkCollision(const AxisAlignedBoundingBox& other) const {
        if ( this->m_p2.x < other.m_p1.x ) return false;
        else if ( this->m_p1.x > other.m_p2.x ) return false;
        else if ( this->m_p2.y < other.m_p1.y ) return false;
        else if ( this->m_p1.y > other.m_p2.y ) return false;
        else if ( this->m_p2.z < other.m_p1.z ) return false;
        else if ( this->m_p1.z > other.m_p2.z ) return false;
        else return true;
    }

    CollisionResolveInfo AxisAlignedBoundingBox::getResolveInfo(const AxisAlignedBoundingBox& other) const {
        const auto xOne = this->m_p2.x - other.m_p1.x;
        const auto xTwo = this->m_p1.x - other.m_p2.x;
        const auto xDistance = abs(xOne) < abs(xTwo) ? xOne : xTwo;

        const auto yOne = this->m_p2.y - other.m_p1.y;
        const auto yTwo = this->m_p1.y - other.m_p2.y;
        const auto yDistance = abs(yOne) < abs(yTwo) ? yOne : yTwo;

        const auto zOne = this->m_p2.z - other.m_p1.z;
        const auto zTwo = this->m_p1.z - other.m_p2.z;
        const auto zDistance = abs(zOne) < abs(zTwo) ? zOne : zTwo;

        const auto thisWeight = 1.0f;
        const auto otherWeight = 1.0f;
        const auto weightSum = thisWeight + otherWeight;
        if ( 0.0f == weightSum )
            return CollisionResolveInfo{ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } };

        const auto thisFactor = thisWeight / weightSum;
        const auto otherFactor = otherWeight / weightSum;

        const auto xForThis = -xDistance * thisFactor;
        const auto yForThis = -yDistance * thisFactor;
        const auto zForThis = -zDistance * thisFactor;

        const auto xForOther = xDistance * otherFactor;
        const auto yForOther = yDistance * otherFactor;
        const auto zForOther = zDistance * otherFactor;

        float selector[3] = { abs(xForThis), abs(yForThis), abs(zForThis) };
        const auto index = minIndex(selector, 3);
        switch ( index ) {

        case 0:
            return CollisionResolveInfo{ {xForThis, 0.0f, 0.0f}, {xForOther, 0.0f, 0.0f} };
        case 1:
            return CollisionResolveInfo{ {0.0f, yForThis, 0.0f}, {0.0f, yForOther, 0.0f} };
        case 2:
            return CollisionResolveInfo{ {0.0f, 0.0f, zForThis}, {0.0f, 0.0f, zForOther} };
        default:
            dalAbort("This cant happen!");

        }

        /*
        if ( abs(xForThis) <= abs(yForThis) && abs(xForThis) <= abs(zForThis) ) {
            return CollisionResolveInfo{ {xForThis, 0.0f, 0.0f}, {xForOther, 0.0f, 0.0f} };
        }
        else if ( abs(yForThis) < abs(xForThis) && abs(yForThis) < abs(zForThis) ) {
            return CollisionResolveInfo{ {0.0f, yForThis, 0.0f}, {0.0f, yForOther, 0.0f} };
        }
        else if ( abs(zForThis) < abs(xForThis) && abs(zForThis) < abs(yForThis) ) {
            return CollisionResolveInfo{ {0.0f, 0.0f, zForThis}, {0.0f, 0.0f, zForOther} };
        }
        else {
            dalAbort("WTF ({}, {}, {})"_format(xForThis, yForThis, zForThis));
        }
        */
    }

    // Private

    void AxisAlignedBoundingBox::validateOrder(void) {
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