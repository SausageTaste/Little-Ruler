from typing import List
import os, json

import level.datastruct.item_builder as bif
import level.datastruct.interface as ein
from level.datastruct.interface import json_t
import level.datastruct.attrib_leaf as pri
import level.datastruct.error_reporter as ere





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

    def getBinary(self) -> bytearray:
        data = bytearray()

        for elem in self.__infoDatas:
            data += elem.getBinary()
            print(elem.getFieldTypeOfSelf(), [x for x in elem.getBinary()])

        return data

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ere.IntegrityReport("Level", self.__levelName.getStr())

        for obj in self.__infoDatas:
            reportChild = obj.getIntegrityReport(obj.getFieldTypeOfSelf())
            if reportChild.hasWarningOrWorse(): report.addChild(reportChild)

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

    def getLevelName(self) -> str:
        return self.__levelName.getStr()


def saveLevelBinary(level: LevelBuilder, outputFolder: str = "outputs/"):
    report = level.getIntegrityReport()
    if report.isFatal():
        raise RuntimeError(report.getFormattedStr())
    else:
        print(report.getFormattedStr())

    if not os.path.isdir(outputFolder):
        os.mkdir(outputFolder)

    filePath: str = "{}/{}.dlb".format(outputFolder, level.getLevelName())
    with open(filePath, "wb") as file:
        file.write(level.getBinary())


def saveLevelJson(level: LevelBuilder, outputFolder: str = "intermediates/"):
    report = level.getIntegrityReport()
    if report.isFatal():
        raise RuntimeError(report.getFormattedStr())
    else:
        print(report.getFormattedStr())

    if not os.path.isdir(outputFolder):
        os.mkdir(outputFolder)

    filePath: str = "{}/{}.json".format(outputFolder, level.getLevelName())

    jsonData = level.getJson()
    with open(filePath, "w") as file:
        try:
            json.dump(jsonData, file, indent=4)
        except TypeError:
            print(jsonData)
            raise