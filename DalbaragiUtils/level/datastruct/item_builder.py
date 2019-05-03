from typing import Dict

import level.datastruct.attrib_complex as bas
import level.datastruct.interface as eim
import level.datastruct.attrib_leaf as pri


class ILevelItemModel(eim.ILevelItem):
    __s_field_model_id: str = "model_name"
    __s_field_actors = "actors"

    def __init__(self, attribDict: Dict[str, eim.ILevelElement]):
        self.__model_id = pri.IdentifierStr()
        self.__actors = bas.UniformList(bas.ActorInfo)

        attribDict[self.__s_field_model_id] = self.__model_id
        attribDict[self.__s_field_actors] = self.__actors

        super().__init__(attribDict)

    def addActor(self, actor: bas.ActorInfo):
        self.__actors.pushBack(actor)


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

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_defined"

    def getMeshHandle(self) -> bas.VertexArray:
        return self.__mesh


class BuildInfo_ModelImported(ILevelItemModel):
    def __init__(self):
        super().__init__({})

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_imported"