#pragma once

#include <numeric>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


// Geometry
namespace dal {

    glm::quat rotateQuat(const glm::quat& q, const float radians, const glm::vec3& selector);

    /*
    In OpenGL coordinate system, if input is (x, z), rotation follows left hand rule.
    */
    glm::vec2 rotateVec2(const glm::vec2& v, const float radians);

    /*
    Normalize xz components on the plane (0, 1, 0, -v.y).
    So if v.y is longer than length, all 3 components must be resized to make it onto sphere.
    */
    glm::vec3 resizeOnlyXZ(const glm::vec3& v, const float sphereRadius);

    template <unsigned int MAX_LEN>
    glm::vec2 clampVec(glm::vec2 v) {
        constexpr auto maxLen = static_cast<float>(MAX_LEN);
        constexpr auto maxLenSqr = static_cast<float>(MAX_LEN * MAX_LEN);

        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLenSqr ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }
    glm::vec2 clampVec(glm::vec2 v, const float maxLen);

    float getSignedDistance_point2Plane(const glm::vec3& v, const glm::vec4& p);

}


// Utils
namespace dal {

    template <typename T>
    T mix(const T x, const T y, const T a) {
        return x * (1.0 - a) + y * a;
    }

    template <typename T>
    T calcWeightedAvrg(const T* const values, const T* const weights, const size_t arrSize) {
        static_assert(std::is_same<T, double>::value || std::is_same<T, float>::value);

        const T sumWeightsInv = 1.0 / std::accumulate(weights, weights + arrSize, 0.0);
        T result = 0;

        for ( size_t i = 0; i < arrSize; ++i ) {
            result += (values[i]) * (weights[i]) * sumWeightsInv;
        }

        return result;
    }

}
