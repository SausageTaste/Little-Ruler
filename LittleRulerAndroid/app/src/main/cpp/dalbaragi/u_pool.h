#pragma once

#include <new>
#include <cstdint>
#include <utility>


namespace dal {

    template <size_t _BlockSize, size_t SIZE>
    class ObjectSizedBytesArray {

    private:
        uint8_t m_arr[_BlockSize * SIZE];

    public:
        void* operator[](const size_t index) {
            const auto arrIndex = index * _BlockSize;
            return reinterpret_cast<void*>(m_arr + arrIndex);
        }

        size_t calcIndexOf(void* const p) const {
            const auto cp = reinterpret_cast<const uint8_t*>(p);
            return static_cast<size_t>(cp - &this->m_arr[0]) / _BlockSize;
        }

    };


    template <typename T, size_t SIZE>
    class BlockAllocator {

    private:
        ObjectSizedBytesArray<sizeof(T), SIZE> m_pool;
        bool m_allocFlags[SIZE] = { false };

        size_t m_nextToAlloc = 0;
        size_t m_allocSize = 0;

    public:
        T* alloc(void) {
            // This means pool is full. X(
            if ( this->m_nextToAlloc == SIZE ) {
                return nullptr;
            }

            const auto allocatedIndex = this->m_nextToAlloc++;
            this->m_allocFlags[allocatedIndex] = true;
            this->m_nextToAlloc = this->getFirstFreeIndex(allocatedIndex);
            return reinterpret_cast<T*>(this->m_pool[allocatedIndex]);
        }

        void free(T* const p) {
            const auto freeIndex = this->m_pool.calcIndexOf(reinterpret_cast<void*>(p));
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


    template <typename T, size_t SIZE>
    class StaticPool {

    private:
        BlockAllocator<T, SIZE> m_allocator;

    public:
        template <typename... VAL_TYPE>
        T* alloc(VAL_TYPE&&... params) {
            auto alloced = this->m_allocator.alloc();
            if ( nullptr == alloced ) {
                return nullptr;
            }
            else {
                return new (alloced) T(std::forward<VAL_TYPE>(params)...);
            }
        }

        void free(T* const p) {
            (*p).~T();
            this->m_allocator.free(p);
        }

    };

}