from typing import List, Union

import level.build_info as bif
import level.level_item as bas
import level.element_interface as ein
from level.element_interface import json_t
import level.primitive_data as pri


LEVEL_COMPONENT_TYPE = Union[
    bif.BuildInfo_ModelImported,
    bif.BuildInfo_ModelDefined
]


class LevelBuilder(ein.ILevelElement):
    __mapItems = {
        bif.BuildInfo_ModelDefined.getFieldTypeOfSelf(): bif.BuildInfo_ModelDefined,
        bif.BuildInfo_ModelImported.getFieldTypeOfSelf(): bif.BuildInfo_ModelImported,
    }

    def __init__(self, name:str):
        self.__levelName = pri.IdentifierStr(name)
        self.__infoDatas: List[ein.ILevelItem] = []

    def add(self, obj: ein.ILevelItem):
        if isinstance(obj, ein.ILevelItem):
            self.__infoDatas.append(obj)
        else:
            raise ValueError('Invalid type: ' + type(obj).__name__)

    def setDefault(self) -> None:
        self.__infoDatas: List[ein.ILevelItem] = []

    def getJson(self) -> json_t:
        data = []

        for obj in self.__infoDatas:
            data.append(obj.getJson())

        return data

    def getIntegrityReport(self, usageName: str = "") -> ein.IntegrityReport:
        report = ein.IntegrityReport("Level", self.__levelName.getStr())

        for obj in self.__infoDatas:
            report = obj.getIntegrityReport(obj.getFieldTypeOfSelf())
            if report.hasWarningOrWorse(): report.addChild(report)

        return report

    def raiseIfFatal(self) -> None:
        report = self.getIntegrityReport()
        if report.isFatal():
            raise RuntimeError(report.getFormattedStr())

    def setJson(self, data: json_t) -> None:
        self.__infoDatas: List[ein.ILevelItem] = []

        for elemJson in data:
            if not isinstance(elemJson, dict): raise ValueError()
            if ein.ILevelItem.s_field_type not in elemJson.keys(): raise ValueError()
            newElem = self.__mapItems[elemJson[ein.ILevelItem.s_field_type]]()
            newElem.setJson(elemJson)
            self.__infoDatas.append(newElem)