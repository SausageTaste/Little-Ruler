#pragma once


namespace dal {

	template <typename T>
	class AutoFree {

	private:
		T* m_ptr;

	public:
		AutoFree(T* p) : m_ptr(p) {

		}

		~AutoFree(void) {
			free(m_ptr);
		}

		T* operator->(void) {
			return m_ptr;
		}

		T* operator*(void) {
			return m_ptr;
		}

		bool isNull(void) {
			return m_ptr == nullptr;
		}

	};

}