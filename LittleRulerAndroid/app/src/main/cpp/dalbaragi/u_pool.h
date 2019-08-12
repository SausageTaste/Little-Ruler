#pragma once

#include <new>
#include <cstdint>
#include <utility>


namespace dal {

    template <typename T, size_t SIZE>
    class ObjectSizedBytesArray {

    private:
        uint8_t m_arr[sizeof(T) * SIZE];

    public:
        T* operator[](const size_t index) {
            const auto arrIndex = index * sizeof(T);
            return reinterpret_cast<T*>(m_arr + arrIndex);
        }

        size_t calcIndexOf(const T* const p) const {
            const auto cp = reinterpret_cast<const uint8_t*>(p);
            return static_cast<size_t>(cp - &this->m_arr[0]) / sizeof(T);
        }

    };


    template <typename T, size_t SIZE>
    class BlockAllocator {

    private:
        ObjectSizedBytesArray<T, SIZE> m_pool;
        bool m_allocFlags[SIZE] = { false };

        size_t m_nextToAlloc = 0;
        size_t m_allocSize = 0;

    public:
        constexpr unsigned int getSize(void) {
            return SIZE;
        }

        T* alloc(void) {
            // This means pool is full. X(
            if ( this->m_nextToAlloc == SIZE ) {
                return nullptr;
            }

            const auto allocatedIndex = this->m_nextToAlloc++;
            this->m_allocFlags[allocatedIndex] = true;
            this->m_nextToAlloc = this->getFirstFreeIndex(allocatedIndex);
            return this->m_pool[allocatedIndex];
        }

        void free(const T* const p) {
            const auto freeIndex = this->m_pool.calcIndexOf(p);
            this->m_allocFlags[freeIndex] = false;
            this->m_nextToAlloc = freeIndex;
            this->m_allocSize--;
        }

    private:
        size_t getFirstFreeIndex(const size_t startIndex = 0) {
            for ( size_t i = startIndex; i < SIZE; i++ ) {
                if ( !this->m_allocFlags[i] ) {
                    return i;
                }
            }
            return SIZE;
        }

    };


    template <typename T, unsigned int SIZE>
    class StaticPool {

    private:
        ObjectSizedBytesArray<T, SIZE> m_pool;
        bool m_allocFlags[SIZE] = { false };

        unsigned int m_nextToAlloc = 0;
        unsigned int m_allocSize = 0;

    public:
        constexpr unsigned int getSize(void) {
            return SIZE;
        }

        template <typename... VAL_TYPE>
        T* alloc(VAL_TYPE&&... params) {
            // This means pool is full. X(
            if ( this->m_nextToAlloc == SIZE ) {
                return nullptr;
            }

            const auto allocatedIndex = this->m_nextToAlloc++;
            this->m_allocFlags[allocatedIndex] = true;
            this->m_nextToAlloc = this->getFirstFreeIndex(allocatedIndex);
            T* pObj = this->m_pool[allocatedIndex];
            return new (pObj) T(std::forward<VAL_TYPE>(params)...);
        }

        void free(const T* const p) {
            (*p).~T();
            const auto freeIndex = this->m_pool.calcIndexOf(p);
            this->m_allocFlags[freeIndex] = false;
            this->m_nextToAlloc = freeIndex;
            this->m_allocSize--;
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

    };

}