import level.utils as uti
import level.mesh_info as mes
import level.base_info as bas


class Build_ModelDefined(bas.BuildInfo):
    s_field_self = "model_defined"

    s_field_mesh = "mesh"
    s_field_model_name: str = "model_name"

    def __init__(self):
        self.__name: str = ""
        self.__mesh: mes.VertexArray = None
        self.__material = bas.Material()
        self.__actor = bas.ActorInfo()

    def getJson(self) -> dict:
        return {
            self.s_field_type : self.s_field_self,
            self.s_field_mesh : self.__mesh.getJson(),
            self.__material.s_field_self : self.__material.getJson(),
            self.__actor.s_field_self : self.__actor.getJson(),
        }

    def getIntegrityReport(self) -> bas.IntegrityReport:
        report = bas.IntegrityReport(self.s_field_self)

        if len(self.__name) == 0:
            report.m_data[self.s_field_model_name] = "[Warning] Not defined"

        if self.__mesh is None:
            report.m_data[self.s_field_mesh] = "[Error] Mesh is null"
        else:
            childReport = self.__mesh.getIntegrityReport()
            if childReport.any(): report.m_children.append(childReport)

        childReport = self.__material.getIntegrityReport()
        if childReport.any(): report.m_children.append(childReport)

        childReport = self.__actor.getIntegrityReport()
        if childReport.any(): report.m_children.append(childReport)

        return report

    @property
    def m_actor(self):
        return self.__actor


class BuildInfo_ModelImported(bas.BuildInfo):
    s_field_self = "model_imported"

    s_field_model_name:str = "model_name"

    def __init__(self):
        self.__model_name: str = ""
        self.m_actor = bas.ActorInfo()

    def overrideFromJson(self, data:dict) -> None:
        if self.s_field_model_name in data.keys():
            self.m_model_name = data[self.s_field_model_name]
        else:
            self.m_model_name = ""
            print("error in overrideFromJson: Model name not defined")
        if self.m_actor.s_field_self in data.keys():
            self.m_actor.overrideFromJson(data[self.m_actor.s_field_self])
        else:
            self.m_actor = bas.ActorInfo()

    def getJson(self) -> dict:
        self.throwIfNotIntegral()

        return {
            self.s_field_type : self.s_field_self,
            self.s_field_model_name : self.m_model_name,
            self.m_actor.s_field_self : self.m_actor.getJson(),
        }

    def getIntegrityReport(self) -> bas.IntegrityReport:
        report = bas.IntegrityReport(self.s_field_self)
        report.m_typeObjectName = self.m_model_name

        if len(self.__model_name) == 0:
            report.m_data["model_name"] = "[Warning] Not defined"

        childReport = self.m_actor.getIntegrityReport()
        if childReport.any(): report.m_children.append(childReport)

        return report

    @property
    def m_model_name(self):
        return self.__model_name
    @m_model_name.setter
    def m_model_name(self, v: str):
        uti.throwIfNotValidStrId(v)
        self.__model_name = str(v)