#pragma once

#include <cstddef>
#include <cstring>
#include <string_view>


namespace dal {

    template <size_t CAPACITY>
    class StringBufferFixedTemplate {

    private:
        // +1 for null terminator.
        // Interface shouldn't expose the fact that it uses null terminator.
        char m_buf[CAPACITY + 1] = { 0 };
        size_t m_pos = 0;

    public:
        size_t getSize(void) const {
            return this->m_pos;
        }
        size_t getCapacity(void) const {
            return CAPACITY;
        }
        size_t getRemaining(void) const {
            return this->getCapacity() - this->getSize();
        }
        bool isFull(void) const {
            return this->m_pos >= CAPACITY;
        }

        const char* getStrBuf(void) {
            this->m_buf[this->m_pos] = '\0';
            return this->m_buf;
        }
        StringBufferFixedTemplate<CAPACITY> getSubstr(const size_t offset, const size_t count) const {
            StringBufferFixedTemplate<CAPACITY> result;
            
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

        bool append(const char* const buf, const size_t bufSize) {
            if ( bufSize > this->getRemaining() ) {
                return false;
            }

            std::memcpy(&this->m_buf[this->m_pos], buf, bufSize);
            this->m_pos += bufSize;
            return true;
        }
        bool append(const char c) {
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


    using StringBufferBasic = StringBufferFixedTemplate<1024>;

}
