from typing import List, Any


class Array2D:
    def __init__(self, valType: type, xSize: int, ySize: int):
        if xSize < 1 or ySize < 1:
            ValueError("Invalid dimension for float array 2d: {} x {}".format(xSize, ySize))
        if not isinstance(valType, type):
            raise ValueError()

        self.__valType = valType
        self.__array = []
        for x in range(xSize):
            column = [valType() for _ in range(ySize)]
            self.__array.append(column)

    def __str__(self):
        buffer = "[\n"

        for y in range(self.getSizeY()):
            buffer += "\t[ "
            lineBuffer = []
            for x in range(self.getSizeX()):
                lineBuffer.append(str(self.__array[x][y]))
            buffer += ", ".join(lineBuffer)
            buffer += " ]\n"

        buffer += "]"

        return buffer

    def getAt(self, x: int, y: int):
        if x < 0 or y < 0:
            raise IndexError()
        return self.__array[x][y]

    def setAt(self, value, x: int, y: int) -> None:
        if not isinstance(value, self.__valType):
            raise TypeError("Expected {}, instead got {}".format(self.__valType, type(value)))
        self.__array[x][y] = value

    def getSizeX(self) -> int:
        return len(self.__array)

    def getSizeY(self) -> int:
        return len(self.__array[0])

    def setList(self, matrix: List[List[Any]]):
        if len(matrix) != self.getSizeY():
            raise ValueError("Dimension mismatch: array is ({}, {}), input is ({}, {})".format(
                self.getSizeX(), self.getSizeY(), "?", len(matrix)))
        for inner in matrix:
            if len(inner) != self.getSizeX():
                raise ValueError("Dimension mismatch: array is ({}, {}), input is ({}, {})".format(
                self.getSizeX(), self.getSizeY(), len(inner), len(matrix)))
            for value in inner:
                if not isinstance(value, self.__valType):
                    raise ValueError("Expected type {}, got {} instead.".format(self.__valType, type(value)))

        for y, inner in enumerate(matrix):
            for x, value in enumerate(inner):
                self.__array[x][y] = value

    def applyFunc(self, func):
        for x in range(self.getSizeX()):
            for y in range(self.getSizeY()):
                self.__array[x][y] = func(self.__array[x][y])


def test():
    arr = Array2D(int, 5, 3)
    arr.setList(
        [
            [1, 2, 3, 4, 5],
            [1, 2, 3, 4, 5],
            [4, 5, 6, 8, 7],
        ]
    )

    print(arr)


if __name__ == '__main__':
    test()
