#pragma once

#include <cstddef>
#include <cstring>


namespace dal {

    template <typename _CharTyp, size_t _Capacity>
    class BasicStaticString {

    public:
        static inline constexpr size_t npos = static_cast<size_t>(-1);

    private:
        // +1 for null terminator.
        _CharTyp m_buf[_Capacity + 1] = { 0 };
        size_t m_size = 0;

    public:
        BasicStaticString(void) = default;
        BasicStaticString(const _CharTyp* const buf, const size_t bufSize) {
            this->set(buf, bufSize);
        }

        BasicStaticString& operator=(const _CharTyp* const str) {
            const auto len = std::strlen(str);
            this->set(str, len);
            return *this;
        }
        BasicStaticString& operator=(const _CharTyp c) {
            this->set(c);
            return *this;
        }
        _CharTyp& operator[](const size_t i) {
            return this->m_buf[i];
        }
        const _CharTyp& operator[](const size_t i) const {
            return this->m_buf[i];
        }

        size_t size(void) const {
            return this->m_size;
        }
        size_t capacity(void) const {
            return _Capacity;
        }
        size_t remaining(void) const {
            return this->capacity() - this->size();
        }
        bool full(void) const {
            return this->m_size >= _Capacity;
        }
        bool empty(void) const {
            return this->m_size == 0;
        }

        const _CharTyp* data(void) const {
            return this->m_buf;
        }
        _CharTyp& back(void) {
            return this->m_buf[this->m_size - 1];
        }
        const _CharTyp& back(void) const {
            return this->m_buf[this->m_size - 1];
        }

        size_t find(const _CharTyp* const str) const {
            const auto result = std::strstr(this->m_buf, str);
            if ( nullptr == result ) {
                return npos;
            }
            else {
                return result - this->m_buf;
            }
        }
        BasicStaticString substr(const size_t offset, const size_t count = npos) const {
            BasicStaticString<_CharTyp, _Capacity> result;

            const auto sizeAfterOffset = this->m_size - offset;
            const auto sizeToCpy = sizeAfterOffset < count ? sizeAfterOffset : count;

            result.m_size = sizeToCpy;
            for ( size_t i = 0; i < sizeToCpy; ++i ) {
                result.m_buf[i] = this->m_buf[offset + i];
            }
            result.m_buf[sizeToCpy] = 0;

            return result;
        }

        void clear(void) {
            this->m_size = 0; this->m_buf[0] = '\0';
        }

        bool set(const _CharTyp* const buf, const size_t bufSize) {
            if ( bufSize > this->capacity() ) {
                return false;
            }
            else {
                std::memcpy(this->m_buf, buf, bufSize);
                this->m_size = bufSize;
                return true;
            }
        }
        bool set(const _CharTyp c) {
            static_assert(_Capacity > 0);

            this->m_buf[0] = c;
            this->m_size = 1; this->m_buf[1] = '\0';
            return true;
        }
        bool append(const _CharTyp* const buf, const size_t bufSize) {
            if ( bufSize > this->remaining() ) {
                return false;
            }
            else {
                std::memcpy(&this->m_buf[this->m_size], buf, bufSize);
                this->m_size += bufSize; this->m_buf[this->m_size] = '\0';
                return true;
            }
        }
        bool append(const _CharTyp c) {
            if ( this->full() ) {
                return false;
            }
            else {
                this->m_buf[this->m_size] = c;
                ++this->m_size; this->m_buf[this->m_size] = '\0';
                return true;
            }
        }

    };


    template <size_t _Capacity>
    using StaticString = BasicStaticString<char, _Capacity>;

    using StringBufferBasic = StaticString<1024>;

}
