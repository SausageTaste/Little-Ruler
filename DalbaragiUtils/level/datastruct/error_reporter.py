from typing import List, Dict


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

        for child in self.__children:
            if child.isFatal(): return True

        return False

    def hasWarningOrWorse(self) -> bool:
        for err in self.__data:
            if err.hasWarningOrWorse():
                return True

        for child in self.__children:
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
            text += "\n"

        for child in self.__children:
            if child.hasWarningOrWorse():
                text += child.getFormattedStr(indent + 1)

        if text[-2] != "\n": text += "\n"

        return text

    def emplaceBack(self, identifier:str="", msg:str="", errLevel:str=""):
        if errLevel:
            self.__data.append(ErrorJournal(identifier, msg, errLevel))
        else:
            self.__data.append(ErrorJournal(identifier, msg))

    def addChild(self, report: "IntegrityReport") -> None:
        self.__children.append(report)


class TypeCodeInspector:
    __data: Dict[int, type] = {}

    @classmethod
    def reportUsage(cls, typeCode: int, user: type) -> None:
        if typeCode in cls.__data.keys():
            if cls.__data[typeCode] != user:
                raise RuntimeError("Two different types are using same type code: {} VS {} for '{}'".format(
                    cls.__data[typeCode], user, typeCode))
            else: return
        else:
            cls.__data[typeCode] = user