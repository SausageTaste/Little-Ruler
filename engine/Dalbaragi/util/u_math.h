#pragma once

#include <array>
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


// Classes
namespace dal {

    template <typename _ValTyp, size_t _Size>
    class Polynomial {

    private:
        static inline constexpr auto ZERO = static_cast<_ValTyp>(0);
        std::array<_ValTyp, _Size> m_array = { 0 };

    public:
        Polynomial(void) = default;
        template <typename _ListTyp, size_t _ListSize>
        Polynomial(const _ListTyp(&list)[_ListSize]) {
            static_assert(_ListSize <= _Size);

            for ( size_t i = 0; i < _ListSize; ++i ) {
                this->m_array[i] = static_cast<_ValTyp>(list[i]);
            }
        }

        template <size_t _OtherSize>
        auto operator+(const Polynomial<_ValTyp, _OtherSize>& other) const {
            constexpr auto bigger = _Size > _OtherSize ? _Size : _OtherSize;
            Polynomial<_ValTyp, bigger> result;

            for ( size_t i = 0; i < bigger; ++i ) {
                result.m_array[i] = this->getCoeff(i) + other.getCoeff(i);
            }
            return result;
        }
        template <size_t _OtherSize>
        auto operator-(const Polynomial<_ValTyp, _OtherSize>& other) const {
            constexpr auto bigger = _Size > _OtherSize ? _Size : _OtherSize;
            Polynomial<_ValTyp, bigger> result;

            for ( size_t i = 0; i < bigger; ++i ) {
                result.m_array[i] = this->getCoeff(i) - other.getCoeff(i);
            }
            return result;
        }
        Polynomial& operator*=(const _ValTyp factor) {
            for ( size_t i = 0; i < _Size; ++i ) {
                this->m_array[i] *= factor;
            }
        }

        _ValTyp operator()(const _ValTyp x) const {
            _ValTyp sum = this->ZERO;

            {
                const auto coeff = this->m_array[0];
                if ( this->ZERO != coeff ) {
                    sum += coeff;
                }
            }

            for ( size_t i = 1; i < _Size; ++i ) {
                const auto coeff = this->m_array[i];
                if ( this->ZERO != coeff ) {
                    sum += coeff * std::pow(x, i);
                }
            }

            return sum;
        }

        template <size_t _Degree>
        _ValTyp getCoeff(void) const {
            static_assert(_Degree < _Size);
            return this->m_array[_Degree];
        }
        _ValTyp getCoeff(const size_t degree) const {
            if ( degree < _Size ) {
                return this->m_array[degree];
            }
            else {
                return this->ZERO;
            }
        }
        template <size_t _Degree>
        void setCoeff(const _ValTyp val) {
            static_assert(_Degree < _Size);
            this->m_array[_Degree] = val;
        }
        void setCoeff(const size_t degree, const _ValTyp val) {
            assert(degree < _Size);
            this->m_array[degree] = val;
        }

        Polynomial differentiate(void) const {
            Polynomial result;

            for ( size_t i = 1; i < _Size; ++i ) {
                result.setCoeff(i - 1, this->m_array[i] * static_cast<_ValTyp>(i));
            }

            return result;
        }
        Polynomial integrate(const _ValTyp c = ZERO) const {
            Polynomial result;
            assert(this->ZERO == this->m_array[_Size - 1]);

            for ( size_t i = 0; i < _Size - 1; ++i ) {
                const auto newCoeff = this->m_array[i] / static_cast<_ValTyp>(i + 1);
                result.m_array[i + 1] = newCoeff;
            }
            result.m_array[0] = c;

            return result;
        }

    };

}
