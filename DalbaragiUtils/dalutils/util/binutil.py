import struct


def get2BytesInt(v: int) -> bytes:
    res = struct.pack("<i", int(v))
    if res[2] or res[3]:
        raise OverflowError(v)
    return res[0:2]

def get4BytesInt(v: int) -> bytes:
    return struct.pack("<i", int(v))

def get4BytesFloat(v: float) -> bytes:
    return struct.pack("<f", float(v))