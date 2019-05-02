import level.mesh_info as mes
import level.level_item as bas
import level.element_interface as eim
import level.primitive_data as pri


class BuildInfo_ModelDefined(eim.ILevelItem):
    __s_field_model_name: str = "model_name"
    __s_field_mesh = "mesh"
    __s_field_material = "material"
    __s_field_actors = "actors"

    def __init__(self):
        self.__name = pri.IdentifierStr()
        self.__mesh = mes.VertexArray()
        self.__material = bas.Material()
        self.__actors = bas.UniformList(bas.ActorInfo)

        super().__init__({
            self.__s_field_model_name : self.__name,
            self.__s_field_mesh       : self.__mesh,
            self.__s_field_material   : self.__material,
            self.__s_field_actors     : self.__actors,
        })

    def getFieldTypeOfSelf(self) -> str:
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

    def getFieldTypeOfSelf(self) -> str:
        return "model_imported"