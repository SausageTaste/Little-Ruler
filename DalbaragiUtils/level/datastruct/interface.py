import abc
from typing import Union, Dict

import level.datastruct.error_reporter as ere


json_t = Union[list, dict, str, int, float]


class ILevelElement(abc.ABC):
    @abc.abstractmethod
    def setDefault(self) -> None: pass

    @abc.abstractmethod
    def getJson(self) -> json_t: pass

    @abc.abstractmethod
    def setJson(self, data: json_t) -> None: pass

    @abc.abstractmethod
    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport: pass


class ILevelAttribLeaf(ILevelElement):
    @abc.abstractmethod
    def setDefault(self) -> None: pass

    @abc.abstractmethod
    def getJson(self) -> json_t: pass

    @abc.abstractmethod
    def setJson(self, data: json_t) -> None: pass

    @abc.abstractmethod
    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport: pass


class ILevelAttrib(ILevelElement):
    s_field_type = "type"

    def __init__(self, attribDict: Dict[str, ILevelElement]):
        self.__attribs: Dict[str, ILevelElement] = attribDict

    def setDefault(self) -> None:
        for element in self.__attribs.values():
            element.setDefault()

    def getJson(self) -> json_t:
        data = {}

        for field, element in self.__attribs.items():
            data[field] = element.getJson()

        return data

    def setJson(self, data: json_t) -> None:
        for field, element in self.__attribs.items():
            if field in data.keys():
                element.setJson(data[field])
            else:
                element.setDefault()

    def getIntegrityReport(self, usageName: str = "") -> ere.IntegrityReport:
        report = ere.IntegrityReport("ILevelItem", usageName)

        for field, elem in self.__attribs.items():
            childReport = elem.getIntegrityReport(field)
            if childReport.hasWarningOrWorse():
                report.addChild(childReport)

        return report


class ILevelItem(ILevelAttrib):
    @staticmethod
    def getFieldTypeOfSelf() -> str: pass

    def getJson(self) -> json_t:
        data = ILevelAttrib.getJson(self)
        data[self.s_field_type] = self.getFieldTypeOfSelf()

        return data

    def setJson(self, data: json_t) -> None:
        if data[self.s_field_type] != self.getFieldTypeOfSelf():
            raise ValueError()

        ILevelAttrib.setJson(self, data)
