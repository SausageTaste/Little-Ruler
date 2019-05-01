import level.primitive_data as pri
import level.utils as uti
import level.mesh_info as mes


class Build_DefineMesh(pri.BuildInfo):
    def __init__(self):
        self.__mesh = None
        self.__material = pri.Material()
        self.__actor = pri.ActorInfo()

    @property
    def m_actor(self):
        return self.__actor


class BuildInfo_ModelImported(pri.BuildInfo):
    s_field_self = "model_imported"

    s_field_model_name:str = "model_name"

    def __init__(self):
        self.__model_name: str = ""
        self.m_actor = pri.ActorInfo()

    def overrideFromJson(self, data:dict) -> None:
        if self.s_field_model_name in data.keys():
            self.m_model_name = data[self.s_field_model_name]
        else:
            self.m_model_name = ""
            print("error in overrideFromJson: Model name not defined")
        if self.m_actor.s_field_self in data.keys():
            self.m_actor.overrideFromJson(data[self.m_actor.s_field_self])
        else:
            self.m_actor = pri.ActorInfo()

    def getJson(self) -> dict:
        self.throwIfNotIntegral()

        return {
            self.s_field_type : self.s_field_self,
            self.s_field_model_name : self.m_model_name,
            self.m_actor.s_field_self : self.m_actor.getJson(),
        }

    def checkIntegrity(self) -> pri.IntegrityReport:
        report = pri.IntegrityReport(self.s_field_self)
        report.m_typeObjectName = self.m_model_name

        if len(self.m_model_name) == 0:
            report.m_data["model_name"] = "Not defined"

        return report

    @property
    def m_model_name(self):
        return self.__model_name
    @m_model_name.setter
    def m_model_name(self, v: str):
        uti.throwIfNotValidStrId(v)
        self.__model_name = str(v)