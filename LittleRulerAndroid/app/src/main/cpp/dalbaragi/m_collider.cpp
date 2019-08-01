#include "m_collider.h"

#include <fmt/format.h>

#include "s_logger_god.h"


using namespace fmt::literals;


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

    AxisAlignedBoundingBox::AxisAlignedBoundingBox(const glm::vec3& p1, const glm::vec3& p2, const float massInv)
        : m_p1(p1), m_p2(p2)
        , m_massInv(massInv)
    {
        this->validateOrder();
    }

    void AxisAlignedBoundingBox::set(const glm::vec3& p1, const glm::vec3& p2) {
        this->m_p1 = p1;
        this->m_p2 = p2;

        this->validateOrder();
    }

    void AxisAlignedBoundingBox::add(const glm::vec3& offset) {
        this->m_p1 += offset;
        this->m_p2 += offset;
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


namespace dal {

    bool checkCollision(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other) {
        if ( one.m_p2.x < other.m_p1.x ) return false;
        else if ( one.m_p1.x > other.m_p2.x ) return false;
        else if ( one.m_p2.y < other.m_p1.y ) return false;
        else if ( one.m_p1.y > other.m_p2.y ) return false;
        else if ( one.m_p2.z < other.m_p1.z ) return false;
        else if ( one.m_p1.z > other.m_p2.z ) return false;
        else return true;
    }

    CollisionResolveInfo calcResolveInfo(const AxisAlignedBoundingBox& one, const AxisAlignedBoundingBox& other) {
        const auto thisWeight = 1.0f / one.m_massInv;
        const auto otherWeight = 1.0f / other.m_massInv;
        const auto weightSum = thisWeight + otherWeight;
        if ( 0.0f == weightSum ) {
            return CollisionResolveInfo{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        }

        const auto xOne = one.m_p2.x - other.m_p1.x;
        const auto xTwo = one.m_p1.x - other.m_p2.x;
        const auto xDistance = abs(xOne) < abs(xTwo) ? xOne : xTwo;

        const auto yOne = one.m_p2.y - other.m_p1.y;
        const auto yTwo = one.m_p1.y - other.m_p2.y;
        const auto yDistance = abs(yOne) < abs(yTwo) ? yOne : yTwo;

        const auto zOne = one.m_p2.z - other.m_p1.z;
        const auto zTwo = one.m_p1.z - other.m_p2.z;
        const auto zDistance = abs(zOne) < abs(zTwo) ? zOne : zTwo;

        const auto thisFactor = thisWeight / weightSum;
        const auto otherFactor = otherWeight / weightSum;

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

}