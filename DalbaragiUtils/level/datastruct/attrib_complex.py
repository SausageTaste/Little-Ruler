import numpy as np

import level.datastruct.interface as ein
from level.datastruct.interface import json_t
import level.datastruct.attrib_leaf as pri
import level.datastruct.error_reporter as ere


class ActorInfo(ein.ILevelAttrib):
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
        self.__list:list= []

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

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ere.IntegrityReport("UniformList< {} >".format(str(self.__type)[1:-1]), usageName)

        for i, x in enumerate(self.__list):
            childReport = x.getIntegrityReport("index {}".format(i))
            if childReport.hasWarningOrWorse(): report.addChild(childReport)

        if len(self.__list) == 0:
            report.emplaceBack("size", "List is empty.")

        return report

    def pushBack(self, item):
        if not isinstance(item, self.__type):
            errMsg = "Value '{}' is not a proper value for UniformList< {} >.".format(
                type(item), str(self.__type)[1:-1]
            )
            raise ValueError(errMsg)

        self.__list.append(item)


class VertexArray(ein.ILevelAttrib):
    __s_field_vertices = "vertices"
    __s_field_texcoords = "texcoords"
    __s_field_normals = "normals"

    def __init__(self):
        self.__vertices = pri.FloatArray()
        self.__texcoords = pri.FloatArray()
        self.__normals = pri.FloatArray()

        super().__init__({
            self.__s_field_vertices : self.__vertices,
            self.__s_field_texcoords : self.__texcoords,
            self.__s_field_normals : self.__normals
        })

    def setArray(self, v: np.ndarray, t: np.ndarray, n: np.ndarray):
        if not isinstance(v, np.ndarray) or not isinstance(t, np.ndarray) or not isinstance(n, np.ndarray):
            raise ValueError()

        self.__vertices = v
        self.__texcoords = t
        self.__normals = n

    def setAABB(self, p1: pri.Vec3, p2: pri.Vec3) -> None:
        x1, y1, z1 = p1.getXYZ()
        x2, y2, z2 = p2.getXYZ()

        if x1 < x2:
            xOne = x1; xTwo = x2
        else:
            xOne = x2; xTwo = x1

        if y1 < y2:
            yOne = y1; yTwo = y2
        else:
            yOne = y2; yTwo = y1

        if z1 < z2:
            zOne = z1; zTwo = z2
        else:
            zOne = z2; zTwo = z1

        del x1, y1, z1
        del x2, y2, z2

        self.__vertices.setArray(np.array([
            xOne, yTwo, zTwo,
            xOne, yOne, zTwo,
            xTwo, yOne, zTwo,
            xOne, yTwo, zTwo,
            xTwo, yOne, zTwo,
            xTwo, yTwo, zTwo,

            xTwo, yTwo, zTwo,
            xTwo, yOne, zTwo,
            xTwo, yOne, zOne,
            xTwo, yTwo, zTwo,
            xTwo, yOne, zOne,
            xTwo, yTwo, zOne,

            xTwo, yTwo, zOne,
            xTwo, yOne, zOne,
            xOne, yOne, zOne,
            xTwo, yTwo, zOne,
            xOne, yOne, zOne,
            xOne, yTwo, zOne,

            xOne, yTwo, zOne,
            xOne, yOne, zOne,
            xOne, yOne, zTwo,
            xOne, yTwo, zOne,
            xOne, yOne, zTwo,
            xOne, yTwo, zTwo,

            xOne, yTwo, zOne,
            xOne, yTwo, zTwo,
            xTwo, yTwo, zTwo,
            xOne, yTwo, zOne,
            xTwo, yTwo, zTwo,
            xTwo, yTwo, zOne,

            xOne, yOne, zTwo,
            xOne, yOne, zOne,
            xTwo, yOne, zOne,
            xOne, yOne, zTwo,
            xTwo, yOne, zOne,
            xTwo, yOne, zTwo,
        ], np.float32))
        self.__texcoords.setArray(np.array([
            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,

            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,

            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,

            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,

            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,

            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1,
        ], np.float32))
        self.__normals.setArray(np.array([
            0, 0, 1,
            0, 0, 1,
            0, 0, 1,
            0, 0, 1,
            0, 0, 1,
            0, 0, 1,

            1, 0, 0,
            1, 0, 0,
            1, 0, 0,
            1, 0, 0,
            1, 0, 0,
            1, 0, 0,

            0, 0, -1,
            0, 0, -1,
            0, 0, -1,
            0, 0, -1,
            0, 0, -1,
            0, 0, -1,

            -1, 0, 0,
            -1, 0, 0,
            -1, 0, 0,
            -1, 0, 0,
            -1, 0, 0,
            -1, 0, 0,

            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,

            0, -1, 0,
            0, -1, 0,
            0, -1, 0,
            0, -1, 0,
            0, -1, 0,
            0, -1, 0,
        ], np.float32))

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ein.ILevelAttrib.getIntegrityReport(self, usageName)

        if self.__vertices.getSize() <= 0:
            report.emplaceBack("arrays", "Vertex array is empty. What's the point of this?")

        return report