#include "u_byteutils.h"


namespace dal {

    bool isBigEndian() {
        constexpr short int number = 0x1;
        const char* const numPtr = reinterpret_cast<const char*>(&number);
        return numPtr[0] != 1;
    }

    bool makeBool1(const uint8_t* begin) {
        return (*begin) != static_cast<uint8_t>(0);
    }

    int32_t makeInt2(const uint8_t* begin) {
        static_assert(1 == sizeof(uint8_t), "Size of uint8 is not 1 byte. WTF???");
        static_assert(4 == sizeof(float), "Size of float is not 4 bytes.");

        uint8_t buf[4];

        if ( isBigEndian() ) {
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = begin[1];
            buf[3] = begin[0];
        }
        else {
            buf[0] = begin[0];
            buf[1] = begin[1];
            buf[2] = 0;
            buf[3] = 0;
        }

        int32_t res;
        memcpy(&res, buf, 4);
        return res;
    }

    int32_t makeInt4(const uint8_t* begin) {
        return assemble4Bytes<int32_t>(begin);
    }

}