import level.datastruct.attrib_complex as bas
import level.datastruct.interface as eim
import level.datastruct.attrib_leaf as pri


class BuildInfo_ModelDefined(eim.ILevelItem):
    __s_field_model_name: str = "model_name"
    __s_field_mesh = "mesh"
    __s_field_material = "material"
    __s_field_actors = "actors"

    def __init__(self):
        self.__name = pri.IdentifierStr()
        self.__mesh = bas.VertexArray()
        self.__material = bas.Material()
        self.__actors = bas.UniformList(bas.ActorInfo)

        super().__init__({
            self.__s_field_model_name : self.__name,
            self.__s_field_mesh       : self.__mesh,
            self.__s_field_material   : self.__material,
            self.__s_field_actors     : self.__actors,
        })

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_defined"


class BuildInfo_ModelImported(eim.ILevelItem):
    __s_field_model_name:str = "model_name"
    __s_field_actors = "actors"

    def __init__(self):
        self.__model_name = pri.IdentifierStr()
        self.__actors = bas.UniformList(bas.ActorInfo)

        super().__init__({
            self.__s_field_model_name : self.__model_name,
            self.__s_field_actors      : self.__actors,
        })

    @staticmethod
    def getFieldTypeOfSelf() -> str:
        return "model_imported"