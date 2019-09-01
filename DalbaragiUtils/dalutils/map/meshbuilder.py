import abc
from typing import List, Tuple


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
        return self.__array[x][y]

    def setAt(self, value, x: int, y: int) -> None:
        if not isinstance(value, self.__valType):
            raise TypeError()
        self.__array[x][y] = value

    def getSizeX(self) -> int:
        return len(self.__array)

    def getSizeY(self) -> int:
        return len(self.__array[0])


class IMeshBuilder(abc.ABC):
    @abc.abstractmethod
    def makeMeshData(self) -> Tuple[List[float], List[float], List[float]]:
        pass


class HeightGrid(IMeshBuilder):
    def __init__(self, xSize: float, zSize: float, xGridCount: int, zGridCount: int):
        self.__xSize = float(xSize)
        self.__zSize = float(zSize)
        self.__heightMap = Array2D(float, xGridCount, zGridCount)

    def makeMeshData(self) -> Tuple[List[float], List[float], List[float]]:
        raise NotImplemented()

    def setHeightAt(self, value: float, x: int, z: int) -> None:
        self.__heightMap.setAt(value, x, z)

    def __checkValidity(self) -> bool:
        if self.__heightMap.getSizeX() < 2 or self.__heightMap.getSizeY() < 2:
            return False


def test():
    grid = HeightGrid(5, 5, 3, 3)
    grid.setHeightAt(1.0, 0, 0)
    grid.setHeightAt(2.0, 1, 1)
    grid.setHeightAt(3.0, 2, 2)

    print(grid.makeMeshData())


if __name__ == '__main__':
    test()
