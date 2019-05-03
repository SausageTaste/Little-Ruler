import math
import numpy as np
from typing import Tuple
import base64
from level.datastruct.interface import ILevelAttribLeaf, json_t, IntegrityReport, ErrorJournal


class Vec3(ILevelAttribLeaf):
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0):
        self.__x = float(x)
        self.__y = float(y)
        self.__z = float(z)

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

    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport:
        return IntegrityReport("attrib::Vec3", usageName)

    def getXYZ(self) -> Tuple[float, float , float]:
        return self.__x, self.__y, self.__z


class Vec4(ILevelAttribLeaf):
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0, w:float=0.0):
        self.__x = float(x)
        self.__y = float(y)
        self.__z = float(z)
        self.__w = float(w)

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

    def overrideFromJson(self, data: list) -> None:
        self.__x = data[0]
        self.__y = data[1]
        self.__z = data[2]
        self.__w = data[3]

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

    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport:
        return IntegrityReport("attrib::Vec4", usageName)


class IdentifierStr(ILevelAttribLeaf):
    def __init__(self, t: str = ""):
        self.__raiseIfInvalidID(t)
        self.__text = str(t)

    def setDefault(self) -> None:
        self.__text = ""

    def getJson(self) -> json_t:
        return self.__text

    def setJson(self, data: json_t) -> None:
        self.__raiseIfInvalidID(data)
        self.__text = str(data)

    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport:
        report = IntegrityReport("attrib::IdentifierStr", usageName)

        if not self.__text:
            report.emplaceBack("str", "Name is not defined.", ErrorJournal.ERROR_LEVEL_WARN)

        return report

    def getStr(self) -> str:
        return self.__text

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


class FloatData(ILevelAttribLeaf):
    def __init__(self, v: float = 0.0):
        self.__value = float(v)

    def setDefault(self) -> None:
        self.__value = 0.0

    def setJson(self, data: json_t) -> None:
        self.__value = float(data)

    def getJson(self) -> json_t:
        return self.__value

    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport:
        return IntegrityReport("attrib::Float", usageName)


class FloatArray(ILevelAttribLeaf):
    def __init__(self, arr: np.ndarray = None):
        if arr is None:
            self.__arr = np.array([], dtype=np.float32)
        elif isinstance(arr, np.ndarray):
            self.__arr = arr
        else:
            raise ValueError()

    def setDefault(self) -> None:
        self.__arr = np.array([], dtype=np.float32)

    def getJson(self) -> json_t:
        return base64.encodebytes(self.__arr.tobytes()).decode("utf8")

    def setJson(self, data: json_t) -> None:
        self.__arr = np.frombuffer(data.encode("utf8"), dtype=np.float32)

    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport:
        report = IntegrityReport("attrib::FloatArray", usageName)

        if self.__arr.size == 0:
            report.emplaceBack("array", "Array in empty.", ErrorJournal.ERROR_LEVEL_WARN)

        return report