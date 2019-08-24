import sys
import base64
from typing import Tuple, Type, List

import glm
import numpy as np

from dalutils.map.interface import json_t, IMapElement
import dalutils.util.binutil as but


class FloatData(IMapElement):
    def __init__(self, v: float = 0.0):
        self.__val = float(v)

    def setDefault(self) -> None:
        self.__val = 0.0

    def getJson(self) -> json_t:
        return self.__val

    def setJson(self, data: json_t) -> None:
        self.__val = float(data)

    # float4
    def getBinary(self) -> bytearray:
        return bytearray(but.get4BytesFloat(self.__val))

    def get(self) -> float:
        return self.__val

    def set(self, v: float):
        self.__val = float(v)


class IntData(IMapElement):
    def __init__(self, v: int = 0):
        self.__value = int(v)

    def __str__(self):
        return "<IntValue {{ {} }}>".format(self.__value)

    def setDefault(self) -> None:
        self.__value = 0

    def getJson(self) -> json_t:
        return self.__value

    def setJson(self, data: json_t) -> None:
        self.__value = int(data)

    # int4
    def getBinary(self) -> bytearray:
        return bytearray(but.get4BytesInt(self.__value))

    def get(self) -> int:
        return self.__value

    def set(self, v: int):
        self.__value = int(v)


class BoolValue(IMapElement):
    def __init__(self, v: bool = False):
        self.__v = bool(v)

    def __str__(self):
        return "< BoolValue {{ {} }} >".format(self.__v)

    def setDefault(self) -> None:
        self.__v = False

    def getJson(self) -> json_t:
        return self.__v

    def setJson(self, data: json_t) -> None:
        self.__v = bool(data)

    # bool1
    def getBinary(self) -> bytearray:
        return bytearray(b"\x01") if self.__v else bytearray(b"\x00")

    def get(self) -> bool:
        return self.__v

    def set(self, v: bool):
        self.__v = bool(v)


class Vec3(IMapElement):
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0):
        self.__vec = glm.vec3(x, y, z)

    def __add__(self, other: "Vec3") -> "Vec3":
        newone = Vec3()
        newone.__vec = self.__vec + other.__vec
        return newone

    def __sub__(self, other: "Vec3") -> "Vec3":
        newone = Vec3()
        newone.__vec = self.__vec - other.__vec
        return newone

    def __truediv__ (self, divider: float):
        newone = Vec3()
        newone.__vec = self.__vec / float(divider)
        return newone

    def setDefault(self) -> None:
        self.__vec = glm.vec3(0, 0, 0)

    def getJson(self) -> json_t:
        return [self.__vec.x, self.__vec.y, self.__vec.z]

    def setJson(self, data: json_t) -> None:
        self.__vec.x = data[0]
        self.__vec.y = data[1]
        self.__vec.z = data[2]

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += but.get4BytesFloat(self.__vec.x)
        data += but.get4BytesFloat(self.__vec.y)
        data += but.get4BytesFloat(self.__vec.z)

        return data

    def getX(self) -> float:
        return self.__vec.x

    def getY(self) -> float:
        return self.__vec.y

    def getZ(self) -> float:
        return self.__vec.z

    def getXYZ(self) -> Tuple[float, float , float]:
        return self.__vec.x, self.__vec.y, self.__vec.z

    def getVec(self) -> glm.vec3:
        return self.__vec

    def set(self, other: "Vec3"):
        self.__vec.x = other.__vec.x
        self.__vec.y = other.__vec.y
        self.__vec.z = other.__vec.z

    def setX(self, v: float):
        self.__vec.x = float(v)

    def setY(self, v: float):
        self.__vec.y = float(v)

    def setZ(self, v: float):
        self.__vec.z = float(v)

    def setXYZ(self, x: float, y: float, z: float) -> None:
        self.setX(x)
        self.setY(y)
        self.setZ(z)

    def getLen(self) -> float:
        return glm.length(self.__vec)

    def getLenSqr(self) -> float:
        return glm.dot(self.__vec, self.__vec)

    def normalize(self) -> None:
        self.__vec = glm.normalize(self.__vec)


class Quat(IMapElement):
    def __init__(self):
        self.__quat = glm.quat(1, 0, 0, 0)

    def __str__(self):
        return "< Quat {{ {}, {}, {}, {} }} >".format(self.__quat.x, self.__quat.y, self.__quat.z, self.__quat.w)

    def setDefault(self) -> None:
        self.__quat = glm.quat(1, 0, 0, 0)

    def getJson(self) -> json_t:
        return [self.__quat.x, self.__quat.y, self.__quat.z, self.__quat.w]

    def setJson(self, data: json_t) -> None:
        self.__quat.x = data[0]
        self.__quat.y = data[1]
        self.__quat.z = data[2]
        self.__quat.w = data[3]

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += but.get4BytesFloat(self.__quat.x)
        data += but.get4BytesFloat(self.__quat.y)
        data += but.get4BytesFloat(self.__quat.z)
        data += but.get4BytesFloat(self.__quat.w)

        return data

    def rotate(self, degree: float, selector: Tuple[float, float, float]):
        selcet = glm.vec3(selector[0], selector[1], selector[2])
        what = glm.angleAxis(glm.radians(degree), selcet) * self.__quat
        # Why do I need to do this???
        # TODO Find out what is this.
        self.__quat = glm.quat(what.z, what.y, -what.x, -what.w)
        self.__quat = glm.normalize(self.__quat)

    def isDefaultValue(self) -> bool:
        if self.__quat.x != 0.0:
            return False
        elif self.__quat.y != 0.0:
            return False
        elif self.__quat.z != 0.0:
            return False
        elif self.__quat.w != 1.0:
            return False
        else:
            return True


class StrData(IMapElement):
    def __init__(self, t: str = ""):
        self.__text = str(t)

    def __str__(self):
        return "< IdentifierStr {} >".format(repr(self.__text))

    def setDefault(self) -> None:
        self.__text = ""

    def getJson(self) -> json_t:
        return self.__text

    def setJson(self, data: json_t) -> None:
        self.__text = str(data)

    def getBinary(self) -> bytearray:
        return bytearray(self.__text.encode(encoding="utf8")) + b'\0'

    def get(self) -> str:
        return self.__text

    def set(self, t: str) -> None:
        self.__text = t


class UniformList(IMapElement):
    def __init__(self, templateType: Type[IMapElement]):
        if not isinstance(templateType(), IMapElement):
            ValueError("Type '{}' is not derived from {}.".format(templateType, IMapElement))

        self.__type: Type[IMapElement] = templateType
        self.__list: List[IMapElement] = []

    def __str__(self):
        return "< UniformList<{}> size={} >".format(str(self.__type)[8:-2], len(self.__list))

    def __iter__(self):
        return iter(self.__list)

    def __iadd__(self, other: "UniformList"):
        if not isinstance(other, UniformList):
            raise ValueError()
        if self.__type is not other.__type:
            raise ValueError("Only UniformList with same elements type can be merged: {} vs {}".format(self.__type, other.__type))

        self.__list += other.__list
        return self

    def setDefault(self) -> None:
        self.__list = []

    def getJson(self) -> json_t:
        data = []
        for x in self.__list:
            data.append(x.getJson())
        return data

    def setJson(self, data: json_t) -> None:
        for x in data:
            elem = self.__type()
            elem.setJson(x)
            self.__list.append(elem)

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += but.get4BytesInt(len(self.__list))
        for item in self.__list:
            data += item.getBinary()
        return data

    def pushBack(self, item):
        if not self.__isObjectValid(item):
            errMsg = "Value '{}' is not a proper value for UniformList< {} >.".format(type(item), str(self.__type)[1:-1] )
            raise ValueError(errMsg)

        self.__list.append(item)

    def __isObjectValid(self, obj):
        return isinstance(obj, self.__type)


class FloatArray(IMapElement):
    def __init__(self):
        self.__arr = np.array([], dtype=np.float32)

    def __str__(self):
        if self.__arr.size > 5:
            numStr = str([x for x in self.__arr[0:5]])[1:-1]
            return "< FloatArray size={} {{ {}, ... }} >".format(self.__arr.size, numStr)
        else:
            numStr = str(list(self.__arr))[1:-1]
            return "< FloatArray size={} {{ {} }} >".format(self.__arr.size, numStr)

    def __getitem__(self, index: int) -> float:
        return self.__arr[int(index)]

    def setDefault(self) -> None:
        self.__arr = np.array([], dtype=np.float32)

    def getJson(self) -> json_t:
        if sys.byteorder != "little":
            raise RuntimeError("Big endian system is not supported yet.")
        return base64.encodebytes(self.__arr.tobytes()).decode("utf8")

    def setJson(self, data: json_t) -> None:
        self.__arr = np.frombuffer(base64.decodebytes(data.encode("utf8")), dtype=np.float32)

    # int4                    : Number of float values
    # Vector of finite float4 : Finite float array
    def getBinary(self) -> bytearray:
        if sys.byteorder != "little":
            raise RuntimeError("Big endian system is not supported yet.")

        data = bytearray()
        data += but.get4BytesInt(self.__arr.size)
        data += self.__arr.tobytes()

        return data

    def getSize(self) -> int:
        return int(self.__arr.size)

    def setArray(self, arr: np.ndarray) -> None:
        if not isinstance(arr, np.ndarray):
            raise ValueError()
        self.__arr = arr

    def append(self, arr: np.ndarray) -> None:
        if not isinstance(arr, np.ndarray):
            raise ValueError("Requires numpy::ndarray, got {} instead.".format(type(arr)))
        np.append(self.__arr, arr)
