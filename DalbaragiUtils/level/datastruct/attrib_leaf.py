import sys
import math
import base64
from typing import Tuple, List, Type

import numpy as np
import glm

from level.datastruct.interface import json_t
import level.datastruct.interface as ein
import level.datastruct.error_reporter as ere
import level.datastruct.bytesutils as but


class Vec3(ein.ILevelAttribLeaf):
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0):
        self.__x = float(x)
        self.__y = float(y)
        self.__z = float(z)

    def __str__(self):
        return "< Vec3 {{ {}, {}, {} }} >".format(self.__x, self.__y, self.__z)

    def getLength(self) -> float:
        return math.sqrt(self.getLengthSquare())

    def getLengthSquare(self) -> float:
        return self.__x**2 + self.__y**2 + self.__z**2

    def normalize(self) -> None:
        length = self.getLength()
        self.__x /= length
        self.__y /= length
        self.__z /= length

    def setDefault(self) -> None:
        self.__x: float = 0.0
        self.__y: float = 0.0
        self.__z: float = 0.0

    def setJson(self, data: json_t) -> None:
        self.__x = data[0]
        self.__y = data[1]
        self.__z = data[2]

    def getJson(self) -> json_t:
        return [self.__x, self.__y, self.__z]

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        return ere.IntegrityReport(type(self).__name__, usageName)

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("value", str(self.getJson()))

        return report

    # Vector of 3 float4 : Vec4 values
    def getBinary(self) -> bytearray:
        data = bytearray()

        data += but.get4BytesFloat(self.__x)
        data += but.get4BytesFloat(self.__y)
        data += but.get4BytesFloat(self.__z)

        return data

    def getXYZ(self) -> Tuple[float, float , float]:
        return self.__x, self.__y, self.__z

    def setX(self, v: float):
        self.__x = float(v)

    def setY(self, v: float):
        self.__y = float(v)

    def setZ(self, v: float):
        self.__z = float(v)


class Vec4(ein.ILevelAttribLeaf):
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0, w:float=0.0):
        self.__x = float(x)
        self.__y = float(y)
        self.__z = float(z)
        self.__w = float(w)

    def __str__(self):
        return "< Vec4 {{ {}, {}, {}, {} }} >".format(self.__x, self.__y, self.__z, self.__w)

    def setDefault(self) -> None:
        self.__x = 0.0
        self.__y = 0.0
        self.__z = 0.0
        self.__w = 0.0

    def getJson(self) -> json_t:
        return [self.__x, self.__y, self.__z, self.__w]

    def setJson(self, data: json_t) -> None:
        self.__x = data[0]
        self.__y = data[1]
        self.__z = data[2]
        self.__w = data[3]

    # Vector of 4 float4 : Vec4 values
    def getBinary(self) -> bytearray:
        data = bytearray()

        data += but.get4BytesFloat(self.__x)
        data += but.get4BytesFloat(self.__y)
        data += but.get4BytesFloat(self.__z)
        data += but.get4BytesFloat(self.__w)

        return data

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        return ere.IntegrityReport(type(self).__name__, usageName)

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("value", str(self.getJson()))

        return report

    def getLength(self) -> float:
        return math.sqrt(self.getLengthSquare())

    def getLengthSquare(self) -> float:
        return self.__x ** 2 + self.__y ** 2 + self.__z ** 2 + self.__w ** 2

    def normalize(self) -> None:
        length = self.getLength()
        self.__x /= length
        self.__y /= length
        self.__z /= length
        self.__w /= length


class Quat(ein.ILevelAttribLeaf):
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

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        return ere.IntegrityReport(type(self).__name__, usageName)

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("value", str(self.getJson()))

        return report

    def rotate(self, degree: float, selector: Tuple[float, float, float]):
        selcet = glm.vec3(*selector)
        what = glm.angleAxis(glm.radians(degree), selcet) * self.__quat
        # Why do I need to do this???
        self.__quat = glm.quat(what.z, what.y, -what.x, -what.w)
        self.__normalize()

    def __normalize(self) -> None:
        # Why should I do this!!!!
        length = math.sqrt(self.__quat.x**2 + self.__quat.y**2 + self.__quat.z**2 + self.__quat.w**2)
        self.__quat.x /= length
        self.__quat.y /= length
        self.__quat.z /= length
        self.__quat.w /= length


class IdentifierStr(ein.ILevelAttribLeaf):
    def __init__(self, t: str = ""):
        self.__raiseIfInvalidID(t)
        self.__text = str(t)

    def __str__(self):
        return "< IdentifierStr {} >".format(repr(self.__text))

    def setDefault(self) -> None:
        self.__text = ""

    def getJson(self) -> json_t:
        return self.__text

    def setJson(self, data: json_t) -> None:
        self.__raiseIfInvalidID(data)
        self.__text = str(data)

    # Vector of char : NULL terminated utf-8 str
    def getBinary(self) -> bytearray:
        data =  bytearray(self.__text.encode(encoding="utf8")) + '\0'.encode()

        return data

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ere.IntegrityReport(type(self).__name__, usageName)

        if not self.__text:
            report.emplaceBack("str", "Name is not defined.", ere.ErrorJournal.ERROR_LEVEL_WARN)

        return report

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("value", str(self.getJson()))

        return report

    def getStr(self) -> str:
        return self.__text

    def setStr(self, v: str) -> None:
        self.__raiseIfInvalidID(v)
        self.__text = str(v)

    @staticmethod
    def __isValidIdentifier(text: str):
        if text == "":
            return True
        elif 0 != text.count(" "):
            return False
        elif 0 != text.count("\n"):
            return False
        elif 0 != text.count("\t"):
            return False
        elif text[0].isnumeric():
            return False

        return True

    def __raiseIfInvalidID(self, text) -> None:
        if not self.__isValidIdentifier(text): raise ValueError("Invalid identifier: " + str(text))


class FloatData(ein.ILevelAttribLeaf):
    def __init__(self, v: float = 0.0):
        self.__value = float(v)

    def __str__(self):
        return "< FloatData {{ {} }} >".format(self.__value)

    def setDefault(self) -> None:
        self.__value = 0.0

    def setJson(self, data: json_t) -> None:
        self.__value = float(data)

    def getJson(self) -> json_t:
        return self.__value

    # float4 : Value
    def getBinary(self) -> bytearray:
        data = bytearray(but.get4BytesFloat(self.__value))

        return data

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        return ere.IntegrityReport(type(self).__name__, usageName)

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("value", str(self.getJson()))

        return report

    def set(self, v: float):
        self.__value = float(v)


class FloatArray(ein.ILevelAttribLeaf):
    def __init__(self):
        self.__arr = np.array([], dtype=np.float32)

    def __str__(self):
        if self.__arr.size > 5:
            numStr = str([x for x in self.__arr[0:5]])[1:-1]
            return "< FloatArray size={} {{ {}, ... }} >".format(self.__arr.size, numStr)
        else:
            numStr = str(list(self.__arr))[1:-1]
            return "< FloatArray size={} {{ {} }} >".format(self.__arr.size, numStr)

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

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ere.IntegrityReport(type(self).__name__, usageName)

        if self.__arr.size == 0:
            report.emplaceBack("array", "Array in empty.", ere.ErrorJournal.ERROR_LEVEL_WARN)

        return report

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("size", str(len(self.__arr)))
        report.addData("value", str(self))

        return report

    def getSize(self) -> int:
        return int(self.__arr.size)

    def setArray(self, arr: np.ndarray) -> None:
        if not isinstance(arr, np.ndarray): raise ValueError()
        self.__arr = arr


class UniformList(ein.ILevelAttribLeaf):
    def __init__(self, templateType: Type[ein.ILevelAttrib]):
        if not isinstance(templateType(), ein.ILevelAttrib):
            ValueError("Type '{}' is not derived from {}.".format(templateType, ein.ILevelAttrib))

        self.__type: Type[ein.ILevelAttrib] = templateType
        self.__list: List[ein.ILevelAttrib] = []

    def __str__(self):
        return "< UniformList<{}> size={} >".format(str(self.__type)[8:-2], len(self.__list))

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

    # One int2                                   : Template type code
    # One int4                                   : Number of elements
    # sizeof(self.__type) * (Number of elements) : Contents
    def getBinary(self) -> bytearray:
        data = self.__type.getTypeCode()
        data += but.get4BytesInt(len(self.__list))

        for item in self.__list:
            data += item.getBinary()

        return data

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ere.IntegrityReport("UniformList< {} >".format(str(self.__type)[1:-1]), usageName)

        for i, x in enumerate(self.__list):
            childReport = x.getIntegrityReport("index {}".format(i))
            if childReport.hasWarningOrWorse(): report.addChild(childReport)

        if len(self.__list) == 0:
            report.emplaceBack("size", "List is empty.")

        return report

    def getDataReport(self, usageName: str = "") -> ere.DataReport:
        report = ere.DataReport(type(self).__name__, usageName, self.getBinary())

        report.addData("type", self.__type.__name__)
        report.addData("size", str(len(self.__list)))
        report.addData("value", str(self.getJson()))

        return report

    def pushBack(self, item):
        if not isinstance(item, self.__type):
            errMsg = "Value '{}' is not a proper value for UniformList< {} >.".format(
                type(item), str(self.__type)[1:-1]
            )
            raise ValueError(errMsg)

        self.__list.append(item)