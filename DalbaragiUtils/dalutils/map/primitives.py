import sys
import base64
from typing import Tuple, Type, List, Optional, Union, Iterable, Any

import glm
import numpy as np

from dalutils.map.interface import json_t, IMapElement
import dalutils.util.binutil as but


class JsonDataMismatch(Exception):
    def __init__(self, jsonData: json_t):
        self.__jsonData = jsonData

    def __str__(self):
        return str(self.__jsonData)


class FloatValue(IMapElement):
    def __init__(self, v: float = 0.0):
        self.__val = float(v)

    def __str__(self):
        typeof = type(self)
        return "{}{{ {} }}".format(typeof.__name__, self.__val)

    def __iadd__(self, other: float):
        self.__val += float(other)
        return self

    def getJson(self) -> json_t:
        return self.__val

    def setJson(self, data: json_t) -> None:
        if not isinstance(data, float):
            raise JsonDataMismatch(data)
        self.__val = float(data)

    # float4
    def getBinary(self) -> bytearray:
        return bytearray(but.get4BytesFloat(self.__val))

    def get(self) -> float:
        return self.__val

    def set(self, v: float):
        self.__val = float(v)


class IntValue(IMapElement):
    def __init__(self, v: int = 0):
        self.__value = int(v)

    def __str__(self):
        return "{}{{ {} }}".format(type(self).__name__, self.__value)

    def getJson(self) -> json_t:
        return self.__value

    def setJson(self, data: json_t) -> None:
        if not isinstance(data, int):
            raise JsonDataMismatch(data)
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
        return "{}{{ {} }} ".format(type(self).__name__, self.__v)

    def getJson(self) -> json_t:
        return self.__v

    def setJson(self, data: json_t) -> None:
        if not isinstance(data, bool):
            raise JsonDataMismatch(data)
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
        self.__vec = glm.vec3(float(x), float(y), float(z))

    def __str__(self):
        return "{}{{ {}, {}, {} }}".format(type(self).__name__, self.__vec.x, self.__vec.y, self.__vec.z)

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

    def getJson(self) -> json_t:
        return [self.__vec.x, self.__vec.y, self.__vec.z]

    def setJson(self, data: json_t) -> None:
        if 3 != len(data):
            raise JsonDataMismatch(data)

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
        if not isinstance(other, Vec3):
            raise TypeError()

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
        return "{}{{ x={}, y={}, z={}, w={} }}".format(type(self).__name__, self.__quat.x, self.__quat.y, self.__quat.z, self.__quat.w)

    def setDefault(self) -> None:
        self.__quat = glm.quat(1, 0, 0, 0)

    def getJson(self) -> json_t:
        return [self.__quat.x, self.__quat.y, self.__quat.z, self.__quat.w]

    def setJson(self, data: json_t) -> None:
        if 4 != len(data):
            raise JsonDataMismatch(data)

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


class StringValue(IMapElement):
    def __init__(self, t: str = ""):
        self.__text = str(t)

    def __str__(self):
        return "{}{{ {} }}".format(type(self).__name__, repr(self.__text))

    def getJson(self) -> json_t:
        return self.__text

    def setJson(self, data: json_t) -> None:
        if not isinstance(data, str):
            raise JsonDataMismatch(data)
        self.__text = str(data)

    def getBinary(self) -> bytearray:
        return bytearray(self.__text.encode(encoding="utf8")) + b'\0'

    def get(self) -> str:
        return self.__text

    def set(self, t: str) -> None:
        self.__text = t


class UniformList(IMapElement):
    def __init__(self, templateType: Type[IMapElement]):
        if (templateType is not None) and (not isinstance(templateType(), IMapElement)):
            ValueError("Type '{}' is not derived from {}.".format(templateType, IMapElement))

        self.__type: Type[IMapElement] = templateType
        self.__list: List[IMapElement] = []

    def __str__(self):
        return "{}<{}>{ size={}, list={} }".format(type(self).__name__, self.__type.__name__, len(self.__list), self.__list)

    def __iter__(self):
        return iter(self.__list)

    def __len__(self):
        return len(self.__list)

    def __iadd__(self, other: "UniformList"):
        if not isinstance(other, UniformList):
            raise ValueError()
        if self.__type is not other.__type:
            raise ValueError("Only UniformList with same elements type can be merged: {} vs {}".format(self.__type, other.__type))

        self.__list += other.__list
        return self

    def getJson(self) -> json_t:
        data = []
        for x in self.__list:
            data.append(x.getJson())
        return data

    def setJson(self, data: json_t) -> None:
        newList = []

        for x in data:
            elem = self.__type()
            elem.setJson(x)
            newList.append(elem)

        self.__list = newList

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

    def clear(self) -> None:
        self.__list = []

    def __isObjectValid(self, obj):
        return isinstance(obj, self.__type)


class FloatList(IMapElement):
    def __init__(self, *args: float):
        self.__arr = np.array(args, dtype=np.float32)

    def __str__(self):
        if self.__arr.size > 5:
            numStr = str([x for x in self.__arr[0:5]])[1:-1] + ", ..."
        else:
            numStr = str(list(self.__arr))[1:-1]

        return "{}{{ size={}, list=[{}] }}".format(type(self).__name__, self.__arr.size, numStr)

    def __len__(self):
        return self.getSize()

    def __getitem__(self, index: int) -> float:
        return self.__arr[int(index)]

    def __setitem__(self, key: int, value: float):
        self.__arr[int(key)] = float(value)

    def getJson(self) -> json_t:
        if sys.byteorder != "little":
            raise RuntimeError("Big endian system is not supported yet.")
        return base64.encodebytes(self.__arr.tobytes()).decode("utf8")

    def setJson(self, data: json_t) -> None:
        if not isinstance(data, str):
            raise JsonDataMismatch(data)
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

    def setArray(self, arr: Union[np.ndarray, Iterable[float]]) -> None:
        if isinstance(arr, np.ndarray):
            self.__arr = arr
        elif isinstance(arr, Iterable):
            arr: Iterable
            self.__arr = np.array(list(arr), dtype=np.float32)
        else:
            raise ValueError("Input value's type {} cannot be converted to ndarray.".format(type(arr)))

    def append(self, arr: Union[np.ndarray, Iterable[float]]) -> None:
        if isinstance(arr, np.ndarray):
            self.__arr = np.append(self.__arr, arr)
        elif isinstance(arr, Iterable):
            arr: Iterable
            nparr = np.array(arr, dtype=np.float32)
            self.__arr = np.append(self.__arr, nparr)
        else:
            raise ValueError("Input value's type {} cannot be converted to ndarray.".format(type(arr)))

    def clear(self) -> None:
        self.__arr = np.array([], dtype=np.float32)


class Variant(IMapElement):
    __s_errMsgOnNone = "Variant needs to have value set."

    def __init__(self, *args: Type[IMapElement]):
        if not len(args):
            raise TypeError("Variant needs to be initialized with types.")

        typesList: List[Type[IMapElement]] = []
        for i in args:
            if not isinstance(i, type):
                raise ValueError()

            try:
                obj = i()  # This means the type must have a default ctor.
            except TypeError:
                raise TypeError("{} cannot be used in Variant.".format(i))
            else:
                if not isinstance(obj, IMapElement):
                    raise TypeError("{} is not derived from IMapElement, so cannot used in Variant.".format(i))
            typesList.append(i)

        self.__types: Tuple[Type[IMapElement], ...] = tuple(typesList)
        self.__data: Optional[IMapElement] = None

    def __str__(self):
        typeNames = [x.__name__ for x in self.__types]
        return "{}<{}>{{ {} }}".format(type(self).__name__, ", ".join(typeNames), self.__data)

    def getJson(self) -> json_t:
        if not self.isValid():
            raise RuntimeError(self.__s_errMsgOnNone)

        return {
            "type": self.__findTypeIndex(type(self.__data)),
            "data": self.__data.getJson(),
        }

    def setJson(self, data: json_t) -> None:
        typeIndex = int(data["type"])
        tmpObj: IMapElement = self.__types[typeIndex]()
        tmpObj.setJson(data["data"])

        self.__data = tmpObj

    def getBinary(self) -> bytearray:
        if not self.isValid():
            raise RuntimeError(self.__s_errMsgOnNone)

        data = bytearray()

        data += bytearray(but.get2BytesInt(self.__findTypeIndex(type(self.__data))))
        data += self.__data.getBinary()

        return data

    def set(self, data) -> None:
        try:
            self.__findTypeIndex(type(data))
        except TypeError:
            typesStr = ", ".join(str(x) for x in self.__types)
            raise ValueError("Input value's type is {} but it must be one of among {}.".format(type(data), typesStr))
        else:
            self.__data = data

    def clear(self) -> None:
        self.__data = None

    def get(self) -> Any:
        if not self.isValid():
            raise RuntimeError(self.__s_errMsgOnNone)
        return self.__data

    def isValid(self) -> bool:
        if self.__data is None:
            return False
        else:
            return True

    def __findTypeIndex(self, t: type) -> int:
        for i, typ in enumerate(self.__types):
            if t is typ:
                return i
        else:
            raise TypeError()


class VariantList(IMapElement):
    def __init__(self, *args: Type[IMapElement]):
        # To check if arg types are vaild.
        Variant(*args)

        self.__types = args
        self.__list: List[Variant] = []

    def __str__(self):
        elemStr = ", ".join(str(i.get()) for i in self.__list)
        return "{}{{ size={} }}".format(type(self).__name__, len(self.__list))

    def __len__(self):
        return len(self.__list)

    def __iter__(self):
        return iter(self.__list)

    def getJson(self) -> json_t:
        data = []
        for x in self.__list:
            data.append(x.getJson())
        return data

    def setJson(self, data: json_t) -> None:
        newList = []

        for x in data:
            tmp = Variant(*self.__types)
            tmp.setJson(x)
            newList.append(tmp)

        self.__list = newList

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += but.get4BytesInt(len(self.__list))
        for item in self.__list:
            data += item.getBinary()
        return data

    def pushBack(self, obj) -> None:
        tmp = Variant(*self.__types)
        tmp.set(obj)
        self.__list.append(tmp)

    def clear(self) -> None:
        self.__list = []


def test():
    varlist = VariantList(FloatList, BoolValue, FloatValue, IntValue)
    varlist.pushBack(BoolValue(False))
    varlist.pushBack(FloatList(1, 2, 3, 4, 5, 6))
    varlist.pushBack(FloatValue(1))
    varlist.pushBack(IntValue(1))

    jsonData = varlist.getJson()
    print(jsonData)
    varlist.setJson(jsonData)

    for x in varlist:
        print(x)

    print()
    print(varlist)


if __name__ == '__main__':
    test()
