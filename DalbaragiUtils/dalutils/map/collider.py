import abc
from typing import Dict, Tuple

import glm

import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.util.typecode as typ


class ICollider(inf.IDataBlock):
    def __init__(self, attribDict: Dict[str, inf.IMapElement] = None):
        super().__init__(attribDict)

        self._typeReg = typ.TypeCodeRegistry()

    @abc.abstractmethod
    def getTypeCode(self) -> int:
        pass


class Sphere(ICollider):
    def __init__(self, xCenter: float = 0.0, yCenter: float = 0.0, zCenter: float = 0.0, radius: float = 1.0):
        self.__center = pri.Vec3(xCenter, yCenter, zCenter)
        self.__radius = pri.FloatData(radius)

        super().__init__({
            "center": self.__center,
            "radius": self.__radius,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__center.getBinary()
        data += self.__radius.getBinary()
        return data

    def setDefault(self) -> None:
        self.__center.setXYZ(0, 0, 0)
        self.__radius.set(1)

    def getTypeCode(self) -> int:
        return self._typeReg.confirm(type(self), 1)

    def makeContaining(self, other: "Sphere"):
        a: glm.vec3 = self.__center.getVec()
        b: glm.vec3 = other.__center.getVec()
        c: float = self.__radius.get()
        d: float = other.__radius.get()

        rel = b - a
        relLen = glm.length(rel)
        relNormal = glm.normalize(rel)

        newCenter = ( a + b + (relNormal * (d - c)) ) / 2
        newRadius: float = c + d + relLen

        return Sphere(newCenter.x, newCenter.y, newCenter.z, newRadius)

    @property
    def m_center(self):
        return self.__center
    @property
    def m_radius(self):
        return self.__radius


class AABB(ICollider):
    __3ftuple = Tuple[float, float, float]

    def __init__(self):
        self.__min = pri.Vec3()
        self.__max = pri.Vec3()

        super().__init__({
            "min": self.__min,
            "max": self.__max,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__min.getBinary()
        data += self.__max.getBinary()
        return data

    def setDefault(self) -> None:
        self.__min.setXYZ(0, 0, 0)
        self.__max.setXYZ(0, 0, 0)

    def getTypeCode(self) -> int:
        return self._typeReg.confirm(type(self), 2)

    def makeItCover(self, x: float, y: float, z: float) -> None:
        if x < self.__min.getX():
            self.__min.setX(x)
        elif x > self.__max.getX():
            self.__max.setX(x)

        if y < self.__min.getY():
            self.__min.setY(y)
        elif y > self.__max.getY():
            self.__max.setY(y)

        if z < self.__min.getZ():
            self.__min.setZ(z)
        elif z > self.__max.getZ():
            self.__max.setZ(z)

    def makeContaining(self, other: "AABB"):
        newAABB = AABB()
        newAABB.makeItCover(*self.__min.getXYZ())
        newAABB.makeItCover(*self.__max.getXYZ())
        newAABB.makeItCover(*other.__min.getXYZ())
        newAABB.makeItCover(*other.__max.getXYZ())
        return newAABB

    def set(self, pMin: __3ftuple, pMax: __3ftuple) -> None:
        self.__min.setXYZ(pMin[0], pMin[1], pMin[2])
        self.__max.setXYZ(pMax[0], pMax[1], pMax[2])
        self.__validateOrder()

    def calcVolume(self) -> float:
        rel = self.__max - self.__min
        x, y, z = rel.getXYZ()
        return x * y * z

    def __validateOrder(self) -> None:
        if self.__min.getX() > self.__max.getX():
            tmp = self.__min.getX()
            self.__min.setX(self.__max.getX())
            self.__max.setX(tmp)

        if self.__min.getY() > self.__max.getY():
            tmp = self.__min.getY()
            self.__min.setY(self.__max.getY())
            self.__max.setY(tmp)

        if self.__min.getZ() > self.__max.getZ():
            tmp = self.__min.getZ()
            self.__min.setZ(self.__max.getZ())
            self.__max.setZ(tmp)


class Triangle(ICollider):
    def __init__(self):
        self.__p1 = pri.Vec3()
        self.__p2 = pri.Vec3()
        self.__p3 = pri.Vec3()

        super().__init__({
            "p1": self.__p1,
            "p2": self.__p2,
            "p3": self.__p3,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__p1.getBinary()
        data += self.__p2.getBinary()
        data += self.__p3.getBinary()
        return data

    def setDefault(self) -> None:
        self.__p1.setXYZ(0, 0, 0)
        self.__p2.setXYZ(0, 0, 0)
        self.__p3.setXYZ(0, 0, 0)

    def getTypeCode(self) -> int:
        return self._typeReg.confirm(type(self), 3)

    @property
    def m_p1(self):
        return self.__p1
    @property
    def m_p2(self):
        return self.__p2
    @property
    def m_p3(self):
        return self.__p3


class TriangleSoup(ICollider):
    def __init__(self):
        self.__triangles = pri.UniformList(Triangle)

        super().__init__({
            "triangles": self.__triangles,
        })

    def __iadd__(self, other: "TriangleSoup"):
        self.__triangles += other.__triangles
        return self

    def getBinary(self) -> bytearray:
        return self.__triangles.getBinary()

    def setDefault(self) -> None:
        self.__triangles.clear()

    def getTypeCode(self) -> int:
        return self._typeReg.confirm(type(self), 4)

    @property
    def m_triangles(self):
        return self.__triangles
