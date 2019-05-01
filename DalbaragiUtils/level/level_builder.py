from typing import List, Union

import level.build_info as bif
import level.utils as uti
import level.base_info as bas


LEVEL_COMPONENT_TYPE = Union[
    bif.BuildInfo_ModelImported
]


class LevelBuilder(bas.BuildInfo):
    def __init__(self, name:str):
        uti.throwIfNotValidStrId(name)

        self.__levelName: str = str(name)
        self.__infoDatas:List[LEVEL_COMPONENT_TYPE] = []

    def add(self, obj: LEVEL_COMPONENT_TYPE):
        if isinstance(obj, bas.BuildInfo):
            #obj.throwIfNotIntegral()
            self.__infoDatas.append(obj)
        else:
            raise ValueError('Invalid type: ' + type(obj).__name__)

    def getJson(self) -> list:
        data = []

        for obj in self.__infoDatas:
            data.append(obj.getJson())

        return data

    def getIntegrityReport(self) -> bas.IntegrityReport:
        report = bas.IntegrityReport("Level")
        report.m_typeObjectName = self.__levelName

        for obj in self.__infoDatas:
            report = obj.getIntegrityReport()
            if report.any(): report.m_children.append(report)

        return report

    def throwIfNotIntegral(self) -> None:
        errMsg: str = "Level ({})\n".format(self.__levelName)
        errAny :bool = False

        for obj in self.__infoDatas:
            report = obj.getIntegrityReport()
            if report.any():
                errMsg += report.getStr(1)
                errAny = True

        if errAny:
            raise ValueError(errMsg)

    @property
    def m_levelName(self):
        return self.__levelName