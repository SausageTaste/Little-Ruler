#pragma once

#include <cstdint>


namespace dal {

	template <uint32_t _Denom>
	class Fixed {

		static_assert(_Denom < 99999999);  // Too random...
		static_assert(_Denom != 0);

	private:
		static constexpr uint32_t m_denom = _Denom;

		int32_t m_data = 0;

	public:
		Fixed(void) = default;
		Fixed(const double v) {
			this->set(v);
		}
		Fixed(const float v) {
			this->set(v);
		}
		Fixed(const int v) {
			this->set(v);
		}

		void set(const double v) {
			this->m_data = v * this->m_denom;
		}
		void set(const float v) {
			this->m_data = v * this->m_denom;
		}
		void set(const int v) {
			this->m_data = v * this->m_denom;
		}

		template <uint32_t _ThatDenom>
		Fixed& operator+=(Fixed<_ThatDenom> that) {
			this->m_data += that.m_data;
			return *this;
		}

		template <uint32_t _ThatDenom>
		Fixed& operator-=(Fixed<_ThatDenom> that) {
			this->m_data -= that.m_data;
			return *this;
		}

		template <uint32_t _ThatDenom>
		Fixed& operator*=(Fixed<_ThatDenom> that) {
			this->m_data = this->m_data * that.m_data / that.m_denom;
			return *this;
		}

		template <uint32_t _ThatDenom>
		Fixed& operator/=(Fixed<_ThatDenom> that) {
			this->m_data = this->m_data * that.m_denom / that.m_data;
			return *this;
		}

		// Converter

		operator double(void) const {
			return static_cast<double>(this->m_data) / static_cast<double>(this->m_denom);
		}
		operator float(void) const {
			return static_cast<float>(this->m_data) / static_cast<float>(this->m_denom);
		}
		operator int(void) const {

		}

	};


	template <uint32_t _Denom>
	Fixed<_Denom> operator+(Fixed<_Denom> lhs, Fixed<_Denom> rhs) {
		lhs += rhs; return lhs; 
	}

	template <uint32_t _Denom>
	Fixed<_Denom> operator-(Fixed<_Denom> lhs, Fixed<_Denom> rhs) { 
		lhs -= rhs; return lhs;
	}

	template <uint32_t _Denom>
	Fixed<_Denom> operator*(Fixed<_Denom> lhs, Fixed<_Denom> rhs) { 
		lhs *= rhs; return lhs;
	}

	template <uint32_t _Denom>
	Fixed<_Denom> operator/(Fixed<_Denom> lhs, Fixed<_Denom> rhs) { 
		lhs /= rhs; return lhs;
	}

}
