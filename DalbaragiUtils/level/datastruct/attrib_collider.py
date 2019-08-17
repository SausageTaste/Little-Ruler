import enum
import math

import level.datastruct.interface as ein
import level.datastruct.error_reporter as ere
import level.datastruct.bytesutils as but
import level.datastruct.attrib_leaf as ale


class BoundingVolume(enum.Enum):
    AABB = 0
    SPHERE = 1


class AABB(ein.ILevelAttrib):
    __s_field_p1 = "p1"
    __s_field_p2 = "p2"

    def __init__(self):
        self.__p1 = ale.Vec3()
        self.__p2 = ale.Vec3()

        super().__init__({
            self.__s_field_p1: self.__p1,
            self.__s_field_p2: self.__p2,
        })

    # After that all as listed in init.
    def getBinary(self) -> bytearray:
        if self.__calcVolume() <= 0.0:
            raise ValueError()

        data = bytearray()
        data += self.__p1.getBinary()
        data += self.__p2.getBinary()
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(7, cls)
        return bytearray(but.get2BytesInt(7))

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ein.ILevelAttrib.getIntegrityReport(self, usageName)

        if self.__calcVolume() <= 0.0:
            report.emplaceBack("box", "Box volume is 0.", ere.ErrorJournal.ERROR_LEVEL_ERROR)

        return report

    def set(self, p1: ale.Vec3(), p2: ale.Vec3()):
        if not isinstance(p1, ale.Vec3): raise ValueError()
        if not isinstance(p2, ale.Vec3): raise ValueError()

        self.__p1 = p1
        self.__p2 = p2

        self.__validate()

    def __calcVolume(self) -> float:
        x1, y1, z1 = self.__p1.getXYZ()
        x2, y2, z2 = self.__p2.getXYZ()

        return abs(x2 - x1) * abs(y2 - y1) * abs(z1 - z2)

    def __validate(self) -> None:
        x1, y1, z1 = self.__p1.getXYZ()
        x2, y2, z2 = self.__p2.getXYZ()

        if x1 > x2:
            self.__p1.setX(x2); self.__p2.setX(x1)
        if y1 > y2:
            self.__p1.setY(y2); self.__p2.setY(y1)
        if z1 > z2:
            self.__p1.setZ(z2); self.__p2.setZ(z1)


class Triangle(ein.ILevelAttrib):
    __s_field_p1 = "p1"
    __s_field_p2 = "p2"
    __s_field_p3 = "p3"

    def __init__(self):
        self.__p1 = ale.Vec3()
        self.__p2 = ale.Vec3()
        self.__p3 = ale.Vec3()

        super().__init__({
            self.__s_field_p1: self.__p1,
            self.__s_field_p2: self.__p2,
            self.__s_field_p3: self.__p3,
        })

    # After that all as listed in init.
    def getBinary(self) -> bytearray:
        if self.__calcArea() <= 0.0:
            raise ValueError()

        data = bytearray()
        data += self.__p1.getBinary()
        data += self.__p2.getBinary()
        data += self.__p3.getBinary()
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        return bytearray(but.get2BytesInt(ere.TypeCodeInspector.reportUsage(8, cls)))

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ein.ILevelAttrib.getIntegrityReport(self, usageName)

        if self.__calcArea() <= 0.0:
            report.emplaceBack("triangle", "Triangle area is 0.", ere.ErrorJournal.ERROR_LEVEL_ERROR)

        return report

    def set(self, p1: ale.Vec3(), p2: ale.Vec3(), p3: ale.Vec3()):
        if not isinstance(p1, ale.Vec3): raise ValueError()
        if not isinstance(p2, ale.Vec3): raise ValueError()
        if not isinstance(p3, ale.Vec3): raise ValueError()

        self.__p1 = p1
        self.__p2 = p2
        self.__p3 = p3

    def __calcArea(self) -> float:
        a = (self.__p2 - self.__p1).getLength()
        b = (self.__p3 - self.__p2).getLength()
        c = (self.__p1 - self.__p3).getLength()

        s = (a + b + c) * 0.5
        return math.sqrt(s * (s - a) * (s - b) * (s - c))


class TriangleSoup(ein.ILevelAttrib):
    __s_field_array = "array"

    def __init__(self):
        self.__array = ale.UniformList(Triangle)

        super().__init__({
            self.__s_field_array: self.__array,
        })

    # After that all as listed in init.
    def getBinary(self) -> bytearray:
        return self.__array.getBinary()

    @classmethod
    def getTypeCode(cls) -> bytearray:
        return bytearray(but.get2BytesInt(ere.TypeCodeInspector.reportUsage(9, cls)))

    def add(self, tri: Triangle):
        self.__array.pushBack(tri)