#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>


namespace dal {

    template <typename _CharTyp, size_t _Capacity>
    class BasicStaticString {

    private:
        // +1 for null terminator.
        // Interface shouldn't expose the fact that it uses null terminator.
        _CharTyp m_buf[_Capacity + 1] = { 0 };
        size_t m_pos = 0;

    public:
        size_t getSize(void) const {
            return this->m_pos;
        }
        size_t getCapacity(void) const {
            return _Capacity;
        }
        size_t getRemaining(void) const {
            return this->getCapacity() - this->getSize();
        }
        bool isFull(void) const {
            return this->m_pos >= _Capacity;
        }

        const _CharTyp* getStrBuf(void) {
            this->m_buf[this->m_pos] = '\0';
            return this->m_buf;
        }
        BasicStaticString<_CharTyp, _Capacity> getSubstr(const size_t offset, const size_t count = std::string::npos) const {
            BasicStaticString<_CharTyp, _Capacity> result;
            
            const auto sizeAfterOffset = this->m_pos - offset;
            const auto sizeToCpy = sizeAfterOffset < count ? sizeAfterOffset : count;

            result.m_pos = sizeToCpy;
            for ( size_t i = 0; i < sizeToCpy; ++i ) {
                result.m_buf[i] = this->m_buf[offset + i];
            }
            result.m_buf[sizeToCpy] = 0;

            return result;
        }

        void clear(void) {
            this->m_pos = 0;
        }

        bool append(const _CharTyp* const buf, const size_t bufSize) {
            if ( bufSize > this->getRemaining() ) {
                return false;
            }

            std::memcpy(&this->m_buf[this->m_pos], buf, bufSize);
            this->m_pos += bufSize;
            return true;
        }
        bool append(const _CharTyp c) {
            if ( this->isFull() ) {
                return false;
            }
            else {
                this->m_buf[this->m_pos] = c;
                ++this->m_pos;
                return true;
            }
        }

    };


    template <size_t _Capacity>
    class StaticString : public BasicStaticString<char, _Capacity> {

    };


    using StringBufferBasic = StaticString<1024>;

}
