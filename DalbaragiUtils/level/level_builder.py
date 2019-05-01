from typing import List, Union, Tuple

import level.primitive_data as pri
import level.build_info as bif
import level.utils as uti


LEVEL_COMPONENT_TYPE = Union[
    bif.BuildInfo_ModelImported
]


class LevelBuilder(pri.BuildInfo):
    def __init__(self, name:str):
        uti.throwIfNotValidStrId(name)

        self.__levelName: str = str(name)
        self.__infoDatas:List[LEVEL_COMPONENT_TYPE] = []

    def add(self, obj: LEVEL_COMPONENT_TYPE):
        if isinstance(obj, bif.BuildInfo_ModelImported):
            obj.throwIfNotIntegral()
            self.__infoDatas.append(obj)
        else:
            raise ValueError('Invalid type: ' + type(obj).__name__)

    def overrideFromJson(self, data: dict) -> None:
        raise NotImplemented()

    def getJson(self) -> list:
        data = []

        for obj in self.__infoDatas:
            data.append(obj.getJson())

        return data

    def checkIntegrity(self) -> List[pri.IntegrityReport]:
        toreturn = []

        for obj in self.__infoDatas:
            report = obj.checkIntegrity()
            if report.any(): toreturn.append(report)

        return toreturn

    def throwIfNotIntegral(self) -> None:
        errMsg: str = "Level ({})\n".format(self.__levelName)
        errAny :bool = False

        for obj in self.__infoDatas:
            report = obj.checkIntegrity()
            if report.any():
                errMsg += report.getStr(1)
                errAny = True

        if errAny:
            raise ValueError(errMsg)

    @property
    def m_levelName(self):
        return self.__levelName