#pragma once


namespace dal {

    template <typename T, unsigned int SIZE>
    class StaticPool {

    private:
        T m_pool[SIZE];
        bool m_allocFlags[SIZE] = { false };

        unsigned int m_nextToAlloc = 0;
        unsigned int m_allocSize = 0;

    public:
        constexpr unsigned int getSize(void) {
            return SIZE;
        }

        T* alloc(void) {
            // This means pool is full. X(
            if ( m_nextToAlloc == SIZE ) return nullptr;

            const auto allocatedIndex = m_nextToAlloc;
            m_allocFlags[allocatedIndex] = true;
            m_nextToAlloc = this->getFirstFreeIndex(allocatedIndex);
            m_allocSize++;
            return &m_pool[allocatedIndex];
        }

        void free(const T* const p) {
            const auto freeIndex = this->getIndex(p);
            m_allocFlags[freeIndex] = false;
            this->m_nextToAlloc = freeIndex;
            m_allocSize--;
        }

    private:
        unsigned int getFirstFreeIndex(const unsigned int startIndex = 0) {
            for ( unsigned int i = startIndex; i < SIZE; i++ ) {
                if ( !this->m_allocFlags[i] ) {
                    return i;
                }
            }
            return SIZE;
        }

        unsigned int getIndex(const T* const p) const {
            return static_cast<unsigned int>(p - &m_pool[0]);
        }

    };

}