from typing import List

import level.element_interface as ein
from level.element_interface import json_t
import level.primitive_data as pri


class ActorInfo(ein.ILevelItem):
    __s_field_actor_name = "actor_name"
    __s_field_pos = "pos"
    __s_field_quat = "quat"

    def __init__(self):
        self.__actor_name = pri.IdentifierStr()
        self.__pos = pri.Vec3()
        self.__quat = pri.Vec4(0, 0, 0, 1)

        super().__init__({
            self.__s_field_actor_name: self.__actor_name,
            self.__s_field_pos: self.__pos,
            self.__s_field_quat: self.__quat,
        })

    def getFieldTypeOfSelf(self) -> str:
        return "actor"


class Material(ein.ILevelAttrib):
    __s_field_diffuseColor = "diffuse_color"
    __s_field_shininess = "shininess"
    __s_field_specularStrngth = "specular"
    __s_field_diffuseMap = "diffuse_map"
    __s_field_specularMap = "specular_map"

    def __init__(self):
        self.__diffuseColor = pri.Vec3()
        self.__shininess = pri.FloatData(32)
        self.__specularStrength = pri.FloatData(1)
        self.__diffuseMap = pri.IdentifierStr()
        self.__specularMap = pri.IdentifierStr()

        super().__init__({
            self.__s_field_diffuseColor : self.__diffuseColor,
            self.__s_field_shininess : self.__shininess,
            self.__s_field_specularStrngth : self.__specularStrength,
            self.__s_field_diffuseMap : self.__diffuseMap,
            self.__s_field_specularMap : self.__specularMap,
        })


class UniformList(ein.ILevelAttribLeaf):
    def __init__(self, templateType: type):
        self.__type = templateType
        self.__list:List[ein.ILevelElement] = []

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

    def getIntegrityReport(self, usageName: str = "") -> ein.IntegrityReport:
        report = ein.IntegrityReport("UniformList<{}>".format(self.__type.__name__), usageName)

        for i, x in enumerate(self.__list):
            childReport = x.getIntegrityReport("index {}".format(i))
            if childReport.hasWarningOrWorse(): report.addChild(childReport)

        return report