#include "u_math.h"

#include <glm/gtc/matrix_transform.hpp>


namespace dal {

    glm::quat rotateQuat(const glm::quat& q, const float radians, const glm::vec3& selector) {
        return glm::normalize(glm::angleAxis(radians, selector) * q);
    }

    glm::vec2 rotateVec2(const glm::vec2& v, const float radians) {
        const auto sinVal = sin(radians);
        const auto cosVal = cos(radians);

        return glm::vec2{
            v.x * cosVal - v.y * sinVal,
            v.x * sinVal + v.y * cosVal
        };
    }

    glm::vec3 resizeOnlyXZ(const glm::vec3& v, const float sphereRadius) {
        const auto circleOfIntersecRadiusSqr = sphereRadius * sphereRadius - v.y * v.y;
        if ( circleOfIntersecRadiusSqr > 0.0f ) {
            const auto circleOfIntersecRadius = sqrt(circleOfIntersecRadiusSqr);
            const auto resizedVecXZ = glm::normalize(glm::vec2{ v.x, v.z }) * circleOfIntersecRadius;
            return glm::vec3{ resizedVecXZ.x, v.y, resizedVecXZ.y };
        }
        else {
            return glm::normalize(v) * sphereRadius;
        }
    }

    glm::vec2 clampVec(glm::vec2 v, const float maxLen) {
        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLen * maxLen ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }

    float getSignedDistance_point2Plane(const glm::vec3& v, const glm::vec4& p) {
        return abs(p.x * v.x + p.y * v.y + p.z * v.z + p.w) * glm::inversesqrt(p.x * p.x + p.y * p.y + p.z + p.z);
    }

}
