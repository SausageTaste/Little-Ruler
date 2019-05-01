from typing import List
import abc

import level.primitive_data as pri
import level.utils as uti


ERROR_LEVEL_WARN = "warn"
ERROR_LEVEL_ERROR = "error"


class ErrorJourner:
    def __init__(self, identifier:str="", msg:str="", errLevel:str=ERROR_LEVEL_ERROR):
        self.m_significant: str = str(errLevel)
        self.m_identifier: str = str(identifier)
        self.m_msg:str = str(msg)


class IntegrityReport:
    def __init__(self, typeName: str):
        self.__typeName = str(typeName)
        self.__typeObjectName = ""
        self.__data: List[ErrorJourner] = []

        self.__children:List[IntegrityReport] = []

    def setObjName(self, s: str):
        self.__typeObjectName = str(s)

    def any(self) -> bool:
        if len(self.__data) != 0: return True
        for child in self.__children:
            if child.any(): return True

        return False

    def getStr(self, indent: int = 0) -> str:
        text = "\t" * indent
        text += self.__typeName
        if self.__typeObjectName:
            text += " (" + self.__typeObjectName + ")\n"
        else:
            text += " (no name)\n"

        for x in self.__data:
            text += "\t" * (indent + 1)
            text += "[{}] {}".format(x.m_significant, x.m_identifier)
            if x.m_msg:
                text += " -> " + x.m_msg
            text += "\n"

        for child in self.__children:
            if child.any():
                text += child.getStr(indent + 1)

        return text

    def pushBack(self, err:ErrorJourner):
        if not isinstance(err, ErrorJourner): raise ValueError()
        self.__data.append(err)

    def emplaceBack(self, identifier:str="", msg:str="", errLevel:str=ERROR_LEVEL_ERROR):
        self.__data.append(ErrorJourner(identifier, msg, errLevel))


class BuildInfo(abc.ABC):
    s_field_type = "type"

    #@abc.abstractmethod
    #def overrideFromJson(self, data: dict) -> None: pass

    @abc.abstractmethod
    def getJson(self) -> dict: pass

    @abc.abstractmethod
    def getIntegrityReport(self) -> IntegrityReport: pass

    def throwIfNotIntegral(self) -> None:
        report = self.getIntegrityReport()
        if report.any(): raise ValueError(report.getStr())


class ActorInfo(BuildInfo):
    s_field_self = "actor"

    s_field_actor_name = "actor_name"
    s_field_pos = "pos"
    s_field_quat = "quat"

    def __init__(self):
        self.__actor_name: str = ""
        self.__pos = pri.Vec3()
        self.__quat = pri.Vec4(0, 0, 0, 1)

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

    def getIntegrityReport(self) -> IntegrityReport:
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
        self.__diffuseColor = pri.Vec3()
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

    def getIntegrityReport(self) -> IntegrityReport:
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