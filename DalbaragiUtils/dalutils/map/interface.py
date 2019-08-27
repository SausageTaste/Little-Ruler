import abc, json
from typing import Union, Dict

import dalutils.util.reporter as rep


json_t = Union[list, dict, str, int, float, bool]


class IMapElement(abc.ABC):
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
            raise ValueError("ILevelAttrib.__init__ hadn't got any paremeters, which is not a proper usage.")
        elif type(attribDict) is not dict:
            raise ValueError("dict type expected but got {} instead.".format(type(attribDict)))
        else:
            self.__attribs: Dict[str, IMapElement] = attribDict

        for k, v in attribDict.items():
            if not isinstance(k, str):
                raise ValueError("Type of key must be str, not {}.\nNote the pair: {}, {}".format(type(k), k, v))
            if not isinstance(v, IMapElement):
                raise ValueError("Type of value must be derived from IMapElement.\nNote class definition of {}.\nNote the pair: {}, {}".format(type(v), k, v))

    def getJson(self) -> json_t:
        data = {}

        for field, element in self.__attribs.items():
            data[field] = element.getJson()
            json.dumps(data[field])

        return data

    # All members must be set to default before calling this.
    def setJson(self, data: json_t) -> None:
        for field, element in self.__attribs.items():
            if field in data.keys():
                element.setJson(data[field])

    @abc.abstractmethod
    def setDefault(self) -> None:
        pass

    @abc.abstractmethod
    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        pass
