import numpy as np

import level.primitive_data as pri
import level.element_interface as ein


class VertexArray(ein.ILevelAttrib):
    __s_field_vertices = "vertices"
    __s_field_texcoords = "texcoords"
    __s_field_normals = "normals"

    def __init__(self):
        self.__vertices = pri.FloatArray()
        self.__texcoords = pri.FloatArray()
        self.__normals = pri.FloatArray()

        super().__init__({
            self.__s_field_vertices : self.__vertices,
            self.__s_field_texcoords : self.__texcoords,
            self.__s_field_normals : self.__normals
        })

    def setArray(self, v: np.ndarray, t: np.ndarray, n: np.ndarray):
        if not isinstance(v, np.ndarray) or not isinstance(t, np.ndarray) or not isinstance(n, np.ndarray):
            raise ValueError()

        self.__vertices = v
        self.__texcoords = t
        self.__normals = n


def generateAABBMesh(p1: pri.Vec3, p2: pri.Vec3) -> VertexArray:
    x1, y1, z1 = p1.getXYZ()
    x2, y2, z2 = p2.getXYZ()

    if x1 < x2:
        xOne = x1; xTwo = x2
    else:
        xOne = x2; xTwo = x1

    if y1 < y2:
        yOne = y1; yTwo = y2
    else:
        yOne = y2; yTwo = y1

    if z1 < z2:
        zOne = z1; zTwo = z2
    else:
        zOne = z2; zTwo = z1

    del x1, y1, z1
    del x2, y2, z2

    vertices = np.array([
        xOne, yTwo, zTwo,
        xOne, yOne, zTwo,
        xTwo, yOne, zTwo,
        xOne, yTwo, zTwo,
        xTwo, yOne, zTwo,
        xTwo, yTwo, zTwo,

        xTwo, yTwo, zTwo,
        xTwo, yOne, zTwo,
        xTwo, yOne, zOne,
        xTwo, yTwo, zTwo,
        xTwo, yOne, zOne,
        xTwo, yTwo, zOne,

        xTwo, yTwo, zOne,
        xTwo, yOne, zOne,
        xOne, yOne, zOne,
        xTwo, yTwo, zOne,
        xOne, yOne, zOne,
        xOne, yTwo, zOne,

        xOne, yTwo, zOne,
        xOne, yOne, zOne,
        xOne, yOne, zTwo,
        xOne, yTwo, zOne,
        xOne, yOne, zTwo,
        xOne, yTwo, zTwo,

        xOne, yTwo, zOne,
        xOne, yTwo, zTwo,
        xTwo, yTwo, zTwo,
        xOne, yTwo, zOne,
        xTwo, yTwo, zTwo,
        xTwo, yTwo, zOne,

        xOne, yOne, zTwo,
        xOne, yOne, zOne,
        xTwo, yOne, zOne,
        xOne, yOne, zTwo,
        xTwo, yOne, zOne,
        xTwo, yOne, zTwo,
    ], np.float32)
    texCoords = np.array([
        0, 1,
        0, 0,
        1, 0,
        0, 1,
        1, 0,
        1, 1,

        0, 1,
        0, 0,
        1, 0,
        0, 1,
        1, 0,
        1, 1,

        0, 1,
        0, 0,
        1, 0,
        0, 1,
        1, 0,
        1, 1,

        0, 1,
        0, 0,
        1, 0,
        0, 1,
        1, 0,
        1, 1,

        0, 1,
        0, 0,
        1, 0,
        0, 1,
        1, 0,
        1, 1,

        0, 1,
        0, 0,
        1, 0,
        0, 1,
        1, 0,
        1, 1,
    ], np.float32)
    normals = np.array([
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,

        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,

        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,

        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,

        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,

        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
    ], np.float32)

    va = VertexArray()
    va.setArray(vertices, texCoords, normals)
    return va