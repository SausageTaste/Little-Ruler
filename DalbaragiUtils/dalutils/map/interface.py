import abc, json
from typing import Union, Dict


json_t = Union[list, dict, str, int, float, bool]


class IMapElement(abc.ABC):
    @abc.abstractmethod
    def setDefault(self) -> None:
        pass

    @abc.abstractmethod
    def getJson(self) -> json_t:
        pass

    @abc.abstractmethod
    def setJson(self, data: json_t) -> None:
        pass

    @abc.abstractmethod
    def getBinary(self) -> bytearray:
        pass


class IDataBlock(IMapElement):
    def __init__(self, attribDict: Dict[str, IMapElement] = None):
        if attribDict is None:
            raise RuntimeError("ILevelAttrib.__init__ hadn't got any paremeters, which is not a proper usage.")
        else:
            self.__attribs: Dict[str, IMapElement] = attribDict

    def setDefault(self) -> None:
        for element in self.__attribs.values():
            element.setDefault()

    def getJson(self) -> json_t:
        data = {}

        for field, element in self.__attribs.items():
            data[field] = element.getJson()
            json.dumps(data[field])

        return data

    def setJson(self, data: json_t) -> None:
        for field, element in self.__attribs.items():
            if field in data.keys():
                element.setJson(data[field])
            else:
                element.setDefault()

    @abc.abstractmethod
    def getBinary(self) -> bytearray:
        pass
