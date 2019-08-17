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


class DataReport:
    def __init__(self, typeStr: str, name: str, binary: bytearray = b""):
        self.__typeStr = str(typeStr)
        self.__name = str(name)
        self.__children: List[DataReport] = []
        self.__data: Dict[str, str] = {}
        self.__binary = bytearray(binary)

    def addData(self, key: str, value: str) -> None:
        self.__data[key] = value

    def addChild(self, child: "DataReport") -> None:
        self.__children.append(child)

    def getFormattedStr(self, indent: int = 0) -> str:
        data = (indent * "\t") + "● Item : {} ({})".format(self.__typeStr, self.__name)
        if len(self.__binary) > 0:
            data += " -> {}\n".format(bytes(self.__binary))
        else:
            data += "\n"

        for k, v in self.__data.items():
            data += (indent + 1) * "\t"
            data += "○ {} : {}\n".format(k ,v)

        data += (indent + 1) * "\t"
        data += "has {} children\n".format(len(self.__children))

        for x in self.__children:
            data += x.getFormattedStr(indent + 1)
            data += "\n"

        return data


class TypeCodeInspector:
    __data: Dict[int, type] = {}

    @classmethod
    def reportUsage(cls, typeCode: int, user: type) -> int:
        typeCode = int(typeCode)

        if typeCode in cls.__data.keys():
            if cls.__data[typeCode] != user:
                raise RuntimeError("Two different types are using same type code: {} VS {} for '{}'".format(
                    cls.__data[typeCode], user, typeCode))
            else:
                return typeCode
        else:
            cls.__data[typeCode] = user
            return typeCode