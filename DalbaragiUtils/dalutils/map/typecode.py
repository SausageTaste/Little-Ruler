from typing import Dict


class TypeCodeRegistry:
    def __init__(self):
        self.__registry: Dict[int, type] = {}

    def confirm(self, user: type, typeCode: int) -> int:
        if not isinstance(typeCode, int):
            raise ValueError("Parameter typeCode is not int, but {}".format(type(typeCode)))

        if 0 == typeCode:
            raise ValueError("Type code 0 is reserved for null.")

        if typeCode in self.__registry.keys():
            if self.__registry[typeCode] != user:
                raise RuntimeError("Two different types are using same type code: {} VS {} for '{}'".format(
                    self.__registry[typeCode], user, typeCode))
            else:
                return typeCode
        else:
            self.__registry[typeCode] = user
            return typeCode
