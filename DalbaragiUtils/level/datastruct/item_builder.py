import abc
from typing import Dict

import level.datastruct.attrib_complex as bas
import level.datastruct.interface as eim
import level.datastruct.attrib_leaf as pri
import level.datastruct.bytesutils as but
import level.datastruct.error_reporter as ere
import level.datastruct.attrib_collider as col


class StaticMesh(eim.ILevelItem):
    __s_field_model_id: str = "model_name"
    __s_field_actors = "actors"
    __s_field_render_units = "render_units"
    __s_field_bounding_type = "bounding_type"

    def __init__(self):
        # Field that become binary directly.
        self.__model_id = pri.IdentifierStr()
        self.__actors = pri.UniformList(bas.ActorInfo)
        self.__renderUnits = pri.UniformList(bas.RenderUnit)
        self.__boundingType = pri.IntValue(0)

        # Datas to build binary, but not used directly.

        super().__init__({
            self.__s_field_model_id      : self.__model_id,
            self.__s_field_actors        : self.__actors,
            self.__s_field_render_units  : self.__renderUnits,
            self.__s_field_bounding_type : self.__boundingType,
        })

    def getBinary(self) -> bytearray:
        data = self.__model_id.getBinary()
        data += self.__actors.getBinary()
        data += self.__renderUnits.getBinary()
        return data

    @classmethod
    @abc.abstractmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(6, cls)
        return bytearray(but.get2BytesInt(6))

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "static_mesh"

    def addRenderUnit(self, mesh: bas.VertexArray, material: bas.Material):
        unit = bas.RenderUnit()
        unit.setMesh(mesh)
        unit.setMaterial(material)
        self.__renderUnits.pushBack(unit)

    def addActor(self, actor: bas.ActorInfo):
        self.__actors.pushBack(actor)

    def setModelID(self, v: str) -> None:
        self.__model_id.setStr(v)

    def setBoundingVolumeType(self, i: col.BoundingVolume):
        self.__boundingType.set(i.value)


class ILevelItemModel(eim.ILevelItem):
    __s_field_model_id: str = "model_name"
    __s_field_actors = "actors"

    def __init__(self, attribDict: Dict[str, eim.ILevelElement]):
        self.__model_id = pri.IdentifierStr()
        self.__actors = pri.UniformList(bas.ActorInfo)

        attribDict[self.__s_field_model_id] = self.__model_id
        attribDict[self.__s_field_actors] = self.__actors

        super().__init__(attribDict)

    # As listed
    def getBinary(self) -> bytearray:
        data = self.__model_id.getBinary()
        data += self.__actors.getBinary()
        return data

    @classmethod
    @abc.abstractmethod
    def getTypeCode(cls) -> bytearray:
        raise NotImplemented()

    def addActor(self, actor: bas.ActorInfo):
        self.__actors.pushBack(actor)

    def getModelID(self) -> str:
        return self.__model_id.getStr()

    def setModelID(self, v: str) -> None:
        self.__model_id.setStr(v)


class BuildInfo_ModelDefined(ILevelItemModel):
    __s_field_mesh = "mesh"
    __s_field_material = "material"

    def __init__(self):
        self.__mesh = bas.VertexArray()
        self.__material = bas.Material()

        super().__init__({
            self.__s_field_mesh       : self.__mesh,
            self.__s_field_material   : self.__material,
        })

    # int2 : Type code
    # from ILevelItemModel
    # as listed
    def getBinary(self) -> bytearray:
        data = self.getTypeCode()
        data += ILevelItemModel.getBinary(self)
        data += self.__mesh.getBinary()
        data += self.__material.getBinary()
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(1, cls)
        return bytearray(but.get2BytesInt(1))

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_defined"

    def getMeshHandle(self) -> bas.VertexArray:
        return self.__mesh

    def getMaterialHandle(self) -> bas.Material:
        return self.__material


class BuildInfo_ModelImported(ILevelItemModel):
    def __init__(self):
        super().__init__({})

    # int2 : Type code
    # from ILevelItemModel
    def getBinary(self) -> bytearray:
        data = self.getTypeCode()
        data += ILevelItemModel.getBinary(self)
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(2, cls)
        return bytearray(but.get2BytesInt(2))

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ILevelItemModel.getIntegrityReport(self, usageName)

        if "" == self.getModelID():
            report.emplaceBack("model_name", "Model name to import must be defined.")

        return report

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_imported"


class BuildInfo_ModelImportedAnimated(ILevelItemModel):
    def __init__(self):
        super().__init__({})

    # int2 : Type code
    # from ILevelItemModel
    def getBinary(self) -> bytearray:
        data = self.getTypeCode()
        data += ILevelItemModel.getBinary(self)
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(6, cls)
        return bytearray(but.get2BytesInt(6))

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ILevelItemModel.getIntegrityReport(self, usageName)

        if "" == self.getModelID():
            report.emplaceBack("model_name", "Model name to import must be defined.")

        return report

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_imported_animated"


class ILevelItem_Light(eim.ILevelItem):
    __s_field_name = "light_name"
    __s_field_static = "static"
    __s_field_color = "color"

    def __init__(self, attribDict: Dict[str, eim.ILevelElement]):
        self.__name = pri.IdentifierStr()
        self.__static = pri.BoolValue(False)
        self.__color = pri.Vec3(1, 1, 1)

        attribDict[self.__s_field_name] = self.__name
        attribDict[self.__s_field_static] = self.__static
        attribDict[self.__s_field_color] = self.__color

        super().__init__(attribDict)

    # As listed
    def getBinary(self) -> bytearray:
        data = self.__name.getBinary()
        data += self.__static.getBinary()
        data += self.__color.getBinary()
        return data

    @classmethod
    @abc.abstractmethod
    def getTypeCode(cls) -> bytearray: pass

    def getColorHandle(self) -> pri.Vec3:
        return self.__color

    def setName(self, s: str) -> None:
        self.__name.setStr(s)

    def setStatic(self, v: bool):
        self.__static.set(v)


class BuildInfo_LightPoint(ILevelItem_Light):
    __s_field_pos = "pos"
    __s_field_maxDist = "max_dist"

    def __init__(self):
        self.__pos = pri.Vec3(0, 0, 0)
        self.__maxDist = pri.FloatData(5)

        super().__init__({
            self.__s_field_pos     : self.__pos,
            self.__s_field_maxDist : self.__maxDist,
        })

    def getBinary(self) -> bytearray:
        data = self.getTypeCode()
        data += ILevelItem_Light.getBinary(self)
        data += self.__pos.getBinary()
        data += self.__maxDist.getBinary()
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(4, cls)
        return bytearray(but.get2BytesInt(4))

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = super().getIntegrityReport(usageName)
        return report

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "light_point"

    def getPosHandle(self) -> pri.Vec3:
        return self.__pos

    def setMaxDistance(self, v: float):
        self.__maxDist.set(v)


class BuildInfo_WaterPlane(eim.ILevelItem):
    __s_field_pos = "pos"
    __s_field_width = "width"
    __s_field_height = "height"
    __s_field_shininess = "shininess"
    __s_field_sepcularStrength = "spec_streng"
    __s_field_moveSpeed = "move_speed"
    __s_field_waveStrength = "wave_streng"
    __s_field_darkestDepthPoint = "darkest_depth_point"
    __s_field_deepColor = "depth_color"
    __s_field_reflectivity = "reflectivity"

    def __init__(self):
        self.__pos = pri.Vec3()
        self.__width = pri.FloatData(5)
        self.__height = pri.FloatData(5)
        self.__shininess = pri.FloatData(128)
        self.__sepcularStrength = pri.FloatData(10)
        self.__moveSpeed = pri.FloatData(0.03)
        self.__waveStrength = pri.FloatData(0.02)
        self.__darkestDepthPoint = pri.FloatData(5)
        self.__deepColor = pri.Vec3(0.07, 0.07, 0.15)
        self.__reflectivity = pri.FloatData(0.05)

        super().__init__({
            self.__s_field_pos : self.__pos,
            self.__s_field_width : self.__width,
            self.__s_field_height : self.__height,
            self.__s_field_shininess : self.__shininess,
            self.__s_field_sepcularStrength : self.__sepcularStrength,
            self.__s_field_moveSpeed : self.__moveSpeed,
            self.__s_field_waveStrength : self.__waveStrength,
            self.__s_field_darkestDepthPoint : self.__darkestDepthPoint,
            self.__s_field_deepColor : self.__deepColor,
            self.__s_field_reflectivity : self.__reflectivity,
        })

    def getBinary(self) -> bytearray:
        data = self.getTypeCode()
        data += self.__pos.getBinary()
        data += self.__width.getBinary()
        data += self.__height.getBinary()
        data += self.__shininess.getBinary()
        data += self.__sepcularStrength.getBinary()
        data += self.__moveSpeed.getBinary()
        data += self.__waveStrength.getBinary()
        data += self.__darkestDepthPoint.getBinary()
        data += self.__deepColor.getBinary()
        data += self.__reflectivity.getBinary()
        return data

    @classmethod
    def getTypeCode(cls) -> bytearray:
        ere.TypeCodeInspector.reportUsage(5, cls)
        return bytearray(but.get2BytesInt(5))

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "water_plane"

    def getPosHandle(self) -> pri.Vec3:
        return self.__pos

    def getDepthColorHandle(self) -> pri.Vec3:
        return self.__deepColor

    def setWidth(self, v: float) -> None:
        self.__width.set(v)

    def setHeight(self, v: float) -> None:
        self.__height.set(v)

    def setShininess(self, v: float) -> None:
        self.__shininess.set(v)

    def setSpecStreng(self, v: float) -> None:
        self.__sepcularStrength.set(v)

    def setMoveSpeed(self, v: float) -> None:
        self.__moveSpeed.set(v)

    def setWaveStreng(self, v: float) -> None:
        self.__waveStrength.set(v)

    def setDarkestDepthPoint(self, v: float) -> None:
        self.__darkestDepthPoint.set(v)

    def setReflectivity(self, v: float) -> None:
        self.__reflectivity.set(v)
