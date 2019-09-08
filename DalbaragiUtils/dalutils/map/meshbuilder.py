import abc
from typing import List, Tuple, Any, Callable, Iterable

import glm

import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.util.containers as dcn
import dalutils.util.math as dmt
import dalutils.util.reporter as rep


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


class FloatArray2D(inf.IDataBlock):
    def __init__(self, rows: int, columns: int):
        self.__rows = pri.IntValue(rows)
        self.__columns = pri.IntValue(columns)
        self.__array = pri.FloatList()

        self.setDefault()

        super().__init__({
            "rows" : self.__rows,
            "columns" : self.__columns,
            "array" : self.__array,
        })

    def __str__(self):
        buffer = "[\n"

        for row in range(self.getRowSize()):
            buffer += "\t[ "
            lineBuffer = []
            for column in range(self.getColumnSize()):
                lineBuffer.append(str(self.getAt(row, column)))
            buffer += ", ".join(lineBuffer)
            buffer += " ]\n"

        buffer += "]"

        return buffer

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__rows.getBinary()
        data += self.__columns.getBinary()
        data += self.__array.getBinary()

        return data

    def setDefault(self) -> None:
        self.__remakeArray()

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        pass

    def getRowSize(self) -> int:
        return self.__rows.get()

    def getColumnSize(self) -> int:
        return self.__columns.get()

    def getAt(self, row: int, column: int) -> float:
        return float(self.__array[self.__calcTotalIndex(row, column)])

    def resetDimension(self, row: int, column: int) -> None:
        self.__rows.set(row)
        self.__columns.set(column)
        self.setDefault()

    def setAt(self, value: float, row: int, column: int) -> None:
        index = self.__calcTotalIndex(row, column)
        self.__array[index] = value

    def forEach(self, func: Callable[[float], float]):
        for i, val in enumerate(self.__array):
            self.__array[i] = func(val)

    def setRowMajor(self, array: Iterable[float]):
        sizeof = len(list(array))
        if sizeof != len(self.__array):
            raise ValueError("Array size mismatch : input {} vs data {}".format(sizeof, len(self.__array)))
        self.__array.setArray(array)

    def __remakeArray(self) -> None:
        self.__array.setArray(0.0 for _ in range(self.__rows.get() * self.__columns.get()))

    def __calcTotalIndex(self, row: int, column: int) -> int:
        return int(row) * self.__columns.get() + int(column)

    def __calcRowColumnPair(self, index: int) -> Tuple[int, int]:
        row = index // self.__columns.get()
        column = index % self.__columns.get()
        return row, column


class IMeshBuilder(inf.IDataBlock):
    @abc.abstractmethod
    def makeMeshData(self) -> Tuple[List[float], List[float], List[float]]:
        pass


class HeightGrid(IMeshBuilder):
    def __init__(self, xSize: float = 10.0, zSize: float = 10.0, xGridCount: int = 5, zGridCount: int = 5, smoothShading: bool = True):
        self.__xLen = pri.FloatValue(xSize)
        self.__zLen = pri.FloatValue(zSize)
        self.__heightMap = FloatArray2D(zGridCount, xGridCount)
        self.__smoothShading = pri.BoolValue(smoothShading)

        super().__init__({
            "len_x" : self.__xLen,
            "len_z" : self.__zLen,
            "height_map" : self.__heightMap,
            "smooth_shading" : self.__smoothShading,
        })

    def __str__(self):
        return self.__heightMap.__str__()

    def getBinary(self) -> bytearray:
        return self._makeBinaryAsListed()

    def setDefault(self) -> None:
        self.__xLen.set(10)
        self.__zLen.set(10)
        self.__heightMap.resetDimension(5, 5)
        self.__heightMap.setDefault()
        self.__smoothShading.set(True)

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        pass

    def makeMeshData(self) -> Tuple[List[float], List[float], List[float]]:
        xGridSize = self.__heightMap.getColumnSize()
        zGridSize = self.__heightMap.getRowSize()

        pointsMap = dcn.Array2D(glm.vec3, xGridSize, zGridSize)
        for x in range(xGridSize):
            for z in range(zGridSize):
                p = self.__makePointAt(x, z)
                pointsMap.setAt(p, x, z)

        texcoordsMap = dcn.Array2D(glm.vec2, xGridSize, zGridSize)
        for x in range(xGridSize):
            for z in range(zGridSize):
                p = self.__makeTexcoordsAt(x, z)
                texcoordsMap.setAt(p, x, z)

        normalsMap = dcn.Array2D(glm.vec3, xGridSize, zGridSize)
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

                vertexCache: List[glm.vec3] = []
                for xIndex, zIndex in triangulatedIndices:
                    vertexCache.append(pointsMap.getAt(xIndex, zIndex))

                triNormal1 = dmt.calcTriangleNormal(vertexCache[0], vertexCache[1], vertexCache[2])
                triNormal2 = dmt.calcTriangleNormal(vertexCache[3], vertexCache[4], vertexCache[5])

                for i, indices in enumerate(triangulatedIndices):
                    xIndex, zIndex = indices

                    thisVertex: glm.vec3 = vertexCache[i]
                    vertices.append(thisVertex.x)
                    vertices.append(thisVertex.y)
                    vertices.append(thisVertex.z)
                    del thisVertex

                    thisTexcoords = texcoordsMap.getAt(xIndex, zIndex)
                    texcoords.append(thisTexcoords.x)
                    texcoords.append(thisTexcoords.y)
                    del thisTexcoords

                    if self.__smoothShading:
                        thisNormal: glm.vec3 = normalsMap.getAt(xIndex, zIndex)
                    else:
                        thisNormal: glm.vec3 = triNormal1 if i < 3 else triNormal2
                    normals.append(thisNormal.x)
                    normals.append(thisNormal.y)
                    normals.append(thisNormal.z)
                    del thisNormal

        return vertices, texcoords, normals

    @property
    def m_heightMap(self):
        return self.__heightMap
    @property
    def m_xLen(self):
        return self.__xLen
    @m_xLen.setter
    def m_xLen(self, l: float):
        self.__xLen.set(l)
    @property
    def m_zLen(self):
        return self.__zLen
    @m_zLen.setter
    def m_zLen(self, l: float):
        self.__zLen.set(l)
    @property
    def m_smoothShading(self):
        return self.__smoothShading
    @m_smoothShading.setter
    def m_smoothShading(self, v: bool):
        self.__smoothShading = bool(v)

    def __checkValidity(self) -> bool:
        if self.__heightMap.getRowSize() < 2 or self.__heightMap.getColumnSize() < 2:
            return False

    def __makePointAt(self, xGrid: int , zGrid: int) -> glm.vec3:
        xGrid = int(xGrid); zGrid = int(zGrid)

        xGridCount = self.__heightMap.getColumnSize()
        zGridCount = self.__heightMap.getRowSize()

        xLeftInGlobal = -(self.__xLen.get() * 0.5)
        zFarInGlobal = -(self.__zLen.get() * 0.5)

        xPos = xLeftInGlobal + (self.__xLen.get() / (xGridCount - 1)) * xGrid
        zPos = zFarInGlobal + (self.__zLen.get() / (zGridCount - 1)) * zGrid
        yPos = self.__heightMap.getAt(zGrid, xGrid)

        return glm.vec3(xPos, yPos, zPos)

    def __makeTexcoordsAt(self, xGrid: int, zGrid: int) -> glm.vec2:
        xGridCount = self.__heightMap.getColumnSize()
        zGridCount = self.__heightMap.getRowSize()
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
    pass


if __name__ == '__main__':
    test()
