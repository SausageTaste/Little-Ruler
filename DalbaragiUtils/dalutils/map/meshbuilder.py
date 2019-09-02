import abc
from typing import List, Tuple, Any

import glm


# Input and output both must be counter clock wise.
def _triangulateRect(p1: Any, p2: Any, p3: Any, p4: Any) -> Tuple[Any, Any, Any, Any, Any, Any]:
    return p1, p2, p3, p1, p3, p4

def assertMeshDataListSizeValidity(vertices: List[float], texcoords: List[float], normals: List[float]) -> None:
    if len(vertices) % 9 != 0:
        errmsg = "The number of float nums for vertices ({}) is not multiple of 9, which means It cannot form last triangle.".format(
            len(vertices)
        )
        raise RuntimeError(errmsg)
    elif (len(vertices) / 3) != (len(texcoords) / 2):
        raise RuntimeError("Vertices list size ({}) and texcoords list size do not match.".format(
            len(vertices), len(texcoords)
        ))
    elif len(vertices) != len(normals):
        raise RuntimeError("Vertices list size ({}) and normals list size do not match.".format(
            len(vertices), len(normals)
        ))


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
        xGridSize = self.__heightMap.getSizeX()
        zGridSize = self.__heightMap.getSizeY()

        pointsMap = Array2D(glm.vec3, xGridSize, zGridSize)
        for x in range(xGridSize):
            for z in range(zGridSize):
                p = self.__makePointAt(x, z)
                pointsMap.setAt(p, x, z)

        texcoordsMap = Array2D(glm.vec2, xGridSize, zGridSize)
        for x in range(xGridSize):
            for z in range(zGridSize):
                p = self.__makeTexcoordsAt(x, z)
                texcoordsMap.setAt(p, x, z)

        normalsMap = Array2D(glm.vec3, xGridSize, zGridSize)
        for x in range(xGridSize):
            for z in range(zGridSize):
                p = self.__makePointNormalFor(x, z)
                normalsMap.setAt(p, x, z)

        vertices = []
        texcoords = []
        normals = []

        for x in range(xGridSize - 1):
            for z in range(zGridSize - 1):
                triangulatedIndices = _triangulateRect(
                    (x    , z    ),
                    (x    , z + 1),
                    (x + 1, z + 1),
                    (x + 1, z    ),
                )

                for xIndex, zIndex in triangulatedIndices:
                    thisVertex: glm.vec3 = pointsMap.getAt(xIndex, zIndex)
                    vertices.append(thisVertex.x)
                    vertices.append(thisVertex.y)
                    vertices.append(thisVertex.z)
                    del thisVertex

                    thisTexcoords: glm.vec2 = texcoordsMap.getAt(xIndex, zIndex)
                    texcoords.append(thisTexcoords.x)
                    texcoords.append(thisTexcoords.y)
                    del thisTexcoords

                    thisNormal: glm.vec3 = normalsMap.getAt(xIndex, zIndex)
                    normals.append(thisNormal.x)
                    normals.append(thisNormal.y)
                    normals.append(thisNormal.z)
                    del thisNormal

        return vertices, texcoords, normals

    def setHeightAt(self, value: float, x: int, z: int) -> None:
        self.__heightMap.setAt(value, x, z)

    def getHeightAt(self, x: int, y: int) -> float:
        return self.__heightMap.getAt(x, y)

    def __checkValidity(self) -> bool:
        if self.__heightMap.getSizeX() < 2 or self.__heightMap.getSizeY() < 2:
            return False

    def __makePointAt(self, xGrid: int , zGrid: int) -> glm.vec3:
        xGrid = int(xGrid); zGrid = int(zGrid)

        xGridCount = self.__heightMap.getSizeX()
        zGridCount = self.__heightMap.getSizeY()

        xLeftInGlobal = -(self.__xSize * 0.5)
        zFarInGlobal = -(self.__zSize * 0.5)

        xPos = xLeftInGlobal + (self.__xSize / (xGridCount - 1)) * xGrid
        zPos = zFarInGlobal + (self.__zSize / (zGridCount - 1)) * zGrid
        yPos = self.getHeightAt(xGrid, zGrid)

        return glm.vec3(xPos, yPos, zPos)

    def __makeTexcoordsAt(self, xGrid: int, zGrid: int) -> glm.vec2:
        xGridCount = self.__heightMap.getSizeX()
        zGridCount = self.__heightMap.getSizeY()
        return glm.vec2(xGrid / (xGridCount - 1), zGrid / (zGridCount - 1))

    def __makePointNormalFor(self, xGrid: int , zGrid: int) -> glm.vec3:
        xGrid = int(xGrid); zGrid = int(zGrid)
        inputPoint = self.__makePointAt(xGrid, zGrid)

        # The order is -z, -x, +z, +x.
        # The coordinate system is same as OpenGL.
        # So index 1 means vec2(xGrid, zGrid) + vec2(-1, 1)
        adjacentOffsets = (
            (0, -1),
            (-1, 0),
            (0, 1),
            (1, 0),
        )
        adjacentPoints = [ None, None, None, None ]

        for i, offset in enumerate(adjacentOffsets):
            try:
                point = self.__makePointAt(xGrid + offset[0], zGrid + offset[1])
            except IndexError:
                adjacentPoints[i] = None
            else:
                adjacentPoints[i] = point

        normalAccum = glm.vec3()
        addedCount = 0

        for i in range(4):
            toEdge1 = i
            toEdge2 = (i + 1) % 4
            try:
                edge1 = adjacentPoints[toEdge1] - inputPoint
                edge2 = adjacentPoints[toEdge2] - inputPoint
            except TypeError:
                continue
            normalAccum += glm.cross(edge1, edge2)
            addedCount += 1

        if 0 == addedCount:
            raise RuntimeError("Failed make normal for grid index ({}, {})".format(xGrid, zGrid))
        else:
            return glm.normalize(normalAccum / addedCount)


def test():
    grid = HeightGrid(5, 5, 3, 3)
    grid.setHeightAt(1.0, 0, 0)
    #grid.setHeightAt(2.0, 1, 1)
    grid.setHeightAt(3.0, 2, 2)

    print(grid.makeMeshData())


if __name__ == '__main__':
    test()
