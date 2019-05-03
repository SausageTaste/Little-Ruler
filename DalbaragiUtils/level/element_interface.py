import abc
from typing import List, Union, Dict


json_t = Union[list, dict, str, int, float]


class ErrorJournal:
    ERROR_LEVEL_WARN = "warn"
    ERROR_LEVEL_ERROR = "error"

    def __init__(self, identifier:str="", msg:str="", errLevel:str=ERROR_LEVEL_ERROR):
        if not self.__isCorrectErrLevel(errLevel):
            raise ValueError("\"{}\" is not a correct value for errLevel.".format(errLevel))

        self.__errLevel = str(errLevel)
        self.__identifier = str(identifier)
        self.__msg = str(msg)

    def isFatal(self) -> bool:
        return self.__errLevel == self.ERROR_LEVEL_ERROR

    def hasWarningOrWorse(self) -> bool:
        mapWarn = [
            self.ERROR_LEVEL_WARN,
            self.ERROR_LEVEL_ERROR,
        ]

        return self.__errLevel in mapWarn

    def getFormattedStr(self) -> str:
        return "[{}] {} : {}".format(self.__errLevel, self.__identifier, self.__msg)

    @classmethod
    def __isCorrectErrLevel(cls, t: str) -> bool:
        return t in { cls.ERROR_LEVEL_WARN, cls.ERROR_LEVEL_ERROR }


class IntegrityReport:
    def __init__(self, typeName: str, usageName: str):
        self.__typeName = str(typeName)
        self.__usageName = str(usageName)
        self.__data: List[ErrorJournal] = []

        self.__children:List[IntegrityReport] = []

    def isFatal(self) -> bool:
        for err in self.__data:
            if err.isFatal():
                return True

        for _, child in self.__children:
            if child.isFatal(): return True

        return False

    def hasWarningOrWorse(self) -> bool:
        for err in self.__data:
            if err.hasWarningOrWorse():
                return True

        for _, child in self.__children:
            if child.hasWarningOrWorse(): return True

        return False

    def getFormattedStr(self, indent: int = 0) -> str:
        text = "\t" * indent
        text += self.__typeName
        if self.__usageName:
            text += " (" + self.__usageName + ")\n"
        else:
            text += " (no name)\n"

        for x in self.__data:
            text += "\t" * (indent + 1)
            text += x.getFormattedStr()

        for childUsage, child in self.__children:
            if child.hasWarningOrWorse():
                text += child.getFormattedStr(indent + 1)

        return text

    def emplaceBack(self, identifier:str="", msg:str="", errLevel:str=""):
        if errLevel:
            self.__data.append(ErrorJournal(identifier, msg, errLevel))
        else:
            self.__data.append(ErrorJournal(identifier, msg))

    def addChild(self, report: "IntegrityReport") -> None:
        self.__children.append(report)


class ILevelElement(abc.ABC):
    @abc.abstractmethod
    def setDefault(self) -> None: pass

    @abc.abstractmethod
    def getJson(self) -> json_t: pass

    @abc.abstractmethod
    def setJson(self, data: json_t) -> None: pass

    @abc.abstractmethod
    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport: pass


class ILevelAttribLeaf(ILevelElement):
    @abc.abstractmethod
    def setDefault(self) -> None: pass

    @abc.abstractmethod
    def getJson(self) -> json_t: pass

    @abc.abstractmethod
    def setJson(self, data: json_t) -> None: pass

    @abc.abstractmethod
    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport: pass


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

    def getIntegrityReport(self, usageName: str = "") -> IntegrityReport:
        report = IntegrityReport("ILevelItem", usageName)

        for field, elem in self.__attribs.items():
            childReport = elem.getIntegrityReport(field)
            if childReport.hasWarningOrWorse():
                report.addChild(childReport)

        return report


class ILevelItem(ILevelAttrib):
    @staticmethod
    def getFieldTypeOfSelf() -> str: pass

    def getJson(self) -> json_t:
        data = { self.s_field_type : self.getFieldTypeOfSelf() }

        for field, element in self.__attribs.items():
            data[field] = element.getJson()

        return data

    def setJson(self, data: json_t) -> None:
        if data[self.s_field_type] != self.getFieldTypeOfSelf():
            raise ValueError()

        ILevelAttrib.setJson(self, data)