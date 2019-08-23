import zlib

import dalutils.util.binutil as but


def compressWithSizeInt32(data: bytes) -> bytes:
    zipData: bytes = zlib.compress(data, zlib.Z_BEST_COMPRESSION)
    originalSize: bytes = but.get4BytesInt(len(data))
    return originalSize + zipData
