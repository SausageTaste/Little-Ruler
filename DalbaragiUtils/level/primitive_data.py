import math
from typing import Dict
import abc

import level.utils as uti


class Vec3:
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0):
        self.__x: float = x
        self.__y: float = y
        self.__z: float = z

    def getLength(self) -> float:
        return math.sqrt(self.getLengthSquare())

    def getLengthSquare(self) -> float:
        return self.__x**2 + self.__y**2 + self.__z**2

    def normalize(self) -> None:
        length = self.getLength()
        self.__x /= length
        self.__y /= length
        self.__z /= length

    def overrideFromJson(self, data: dict) -> None:
        self.__x = data[0]
        self.__y = data[1]
        self.__z = data[2]

    def getJson(self) -> list:
        return [self.__x, self.__y, self.__z]

    @property
    def x(self):
        return self.__x
    @x.setter
    def x(self, v: float):
        self.__x = float(v)

    @property
    def y(self):
        return self.__y
    @y.setter
    def y(self, v: float):
        self.__y = float(v)

    @property
    def z(self):
        return self.__x
    @z.setter
    def z(self, v: float):
        self.__z = float(v)


class Vec4(Vec3):
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0, w:float=0.0):
        super().__init__(x, y, z)
        self.__w: float = float(w)

    def getLength(self) -> float:
        return math.sqrt(self.getLengthSquare())

    def getLengthSquare(self) -> float:
        return self.x ** 2 + self.y ** 2 + self.z ** 2 + self.__w ** 2

    def normalize(self) -> None:
        length = self.getLength()
        self.x /= length
        self.y /= length
        self.z /= length
        self.__w /= length

    def overrideFromJson(self, data: dict) -> None:
        self.x = data[0]
        self.y = data[1]
        self.z = data[2]
        self.__w = data[3]

    def getJson(self) -> list:
        a = super().getJson()
        a.append(self.__w)
        return a

    @property
    def w(self):
        return self.__w
    @w.setter
    def w(self, v: float):
        self.__w = float(v)


class IntegrityReport:
    def __init__(self, typeName: str):
        self.m_typeName = str(typeName)
        self.m_typeObjectName = ""
        self.m_data: Dict[str, str] = {}

    def any(self) -> bool:
        return len(self.m_data) != 0

    def getStr(self, indent: int = 0) -> str:
        text = "\t" * indent
        text += self.m_typeName
        if self.m_typeObjectName:
            text += " (" + self.m_typeObjectName + ")\n"
        else:
            text += " (no name)\n"

        for x in self.m_data:
            text += "\t" * (indent + 1)
            text += x
            if self.m_data[x]:
                text += " : " + self.m_data[x]
            text += "\n"

        return text


class BuildInfo(abc.ABC):
    s_field_type = "type"

    @abc.abstractmethod
    def overrideFromJson(self, data: dict) -> None: pass

    @abc.abstractmethod
    def getJson(self) -> dict: pass

    @abc.abstractmethod
    def checkIntegrity(self) -> IntegrityReport: pass

    def throwIfNotIntegral(self) -> None:
        report = self.checkIntegrity()
        if report.any(): raise ValueError(report.getStr())


class ActorInfo(BuildInfo):
    s_field_self = "actor"

    s_field_actor_name = "actor_name"
    s_field_pos = "pos"
    s_field_quat = "quat"

    def __init__(self):
        self.__actor_name: str = ""
        self.__pos = Vec3()
        self.__quat = Vec4(0, 0, 0, 1)

    def overrideFromJson(self, data: dict) -> None:
        if self.s_field_actor_name in data.keys():
            self.__actor_name = data[self.s_field_actor_name]
        if self.s_field_pos in data.keys():
            self.__pos.overrideFromJson(data[self.s_field_pos])
        if self.s_field_quat in data.keys():
            self.__quat.overrideFromJson(data[self.s_field_quat])

    def getJson(self) -> dict:
        self.throwIfNotIntegral()

        return {
            self.s_field_actor_name : self.__actor_name,
            self.s_field_pos : self.__pos.getJson(),
            self.s_field_quat : self.__quat.getJson(),
        }

    def checkIntegrity(self) -> IntegrityReport:
        report = IntegrityReport(self.s_field_self)
        report.m_typeObjectName = self.__actor_name

        return report

    @property
    def m_actor_name(self):
        return self.__actor_name
    @m_actor_name.setter
    def m_actor_name(self, v: str):
        uti.throwIfNotValidStrId(v)
        self.__actor_name = str(v)

    @property
    def m_pos(self):
        return self.__pos

    @property
    def m_quat(self):
        return self.__quat


class Material(BuildInfo):
    s_field_self = "material"

    s_field_diffuseColor = "diffuse_color"
    s_field_shininess = "shininess"
    s_field_specularStrngth = "specular"
    s_field_diffuseMap = "diffuse_map"
    s_field_specularMap = "specular_map"

    def __init__(self):
        self.__diffuseColor = Vec3()
        self.__shininess: float = 32.0
        self.__specularStrength: float = 1.0
        self.__diffuseMap: str = ""
        self.__specularMap: str = ""

    def overrideFromJson(self, data: dict) -> None:
        if self.s_field_diffuseColor in data.keys():
            self.__diffuseColor.overrideFromJson(data[self.s_field_diffuseColor])
        if self.s_field_shininess in data.keys():
            self.__shininess = data[self.s_field_shininess]
        if self.s_field_specularStrngth in data.keys():
            self.__specularStrength = data[self.s_field_specularStrngth]
        if self.s_field_diffuseMap in data.keys():
            self.__diffuseMap = data[self.s_field_diffuseMap]
        if self.s_field_specularMap in data.keys():
            self.__specularMap = data[self.s_field_specularMap]

    def getJson(self) -> dict:
        return {
            self.s_field_diffuseColor : self.__diffuseColor.getJson(),
            self.s_field_shininess : self.__shininess,
            self.s_field_specularStrngth : self.__specularStrength,
            self.s_field_diffuseMap : self.__diffuseMap,
            self.s_field_specularMap : self.__specularMap
        }

    def checkIntegrity(self) -> IntegrityReport:
        report = IntegrityReport(self.s_field_self)
        return report

    @property
    def m_diffuseColor(self):
        return self.__diffuseColor

    @property
    def m_shininess(self):
        return self.__shininess
    @m_shininess.setter
    def m_shininess(self, v: float):
        self.__shininess = float(v)

    @property
    def m_specularStrength(self):
        return self.__specularStrength
    @m_specularStrength.setter
    def m_specularStrength(self, v: float):
        self.__specularStrength = float(v)

    @property
    def m_diffuseMap(self):
        return self.__diffuseMap
    @m_diffuseMap.setter
    def m_diffuseMap(self, v: str):
        self.__diffuseMap = str(v)

    @property
    def m_specularMap(self):
        return self.__specularMap
    @m_specularMap.setter
    def m_specularMap(self, v: str):
        self.__specularMap = str(v)