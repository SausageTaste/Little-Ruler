#pragma once

#include <cstdint>
#include <cstring>


namespace dal {

    bool isBigEndian();

    bool makeBool1(const uint8_t* begin);
    int32_t makeInt2(const uint8_t* begin);
    int32_t makeInt4(const uint8_t* begin);

    template <typename T>
    T assemble4Bytes(const uint8_t* const begin) {
        static_assert(1 == sizeof(uint8_t));
        static_assert(4 == sizeof(T));

        T res;

        if ( isBigEndian() ) {
            uint8_t buf[4];
            buf[0] = begin[3];
            buf[1] = begin[2];
            buf[2] = begin[1];
            buf[3] = begin[0];
            memcpy(&res, buf, 4);
        }
        else {
            memcpy(&res, begin, 4);
        }

        return res;
    }

    template <typename T>
    const uint8_t* assemble4BytesArray(const uint8_t* src, T* const dst, const size_t size) {
        for ( size_t i = 0; i < size; ++i ) {
            dst[i] = assemble4Bytes<T>(src); src += 4;
        }
        return src;
    }

    // Returns 0 on fail.
    size_t unzip(uint8_t* const dst, const size_t dstSize, const uint8_t* const src, const size_t srcSize);

}
