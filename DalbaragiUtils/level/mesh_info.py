import numpy as np
import base64

import level.primitive_data as pri
import level.base_info as bas


class VertexArray(bas.BuildInfo):
    s_field_self = "vertex_array"

    s_field_vertices = "vertices"
    s_field_texcoords = "texcoords"
    s_field_normals = "normals"

    def __init__(self, v: np.ndarray, t: np.ndarray, n: np.ndarray):
        if not isinstance(v, np.ndarray): raise ValueError("v is " + type(v).__name__)
        if not isinstance(t, np.ndarray): raise ValueError("t is " + type(t).__name__)
        if not isinstance(n, np.ndarray): raise ValueError("n is " + type(n).__name__)

        self.__vertices = v
        self.__texcoords = t
        self.__normals = n

    def getIntegrityReport(self) -> bas.IntegrityReport:
        report = bas.IntegrityReport(self.s_field_self)

        if self.__vertices.size == 0:
            report.emplaceBack(self.s_field_vertices, "Empty")

        if self.__texcoords.size == 0:
            report.emplaceBack(self.s_field_texcoords, "Empty")

        if self.__normals.size == 0:
            report.emplaceBack(self.s_field_normals, "Empty")


        vertSize: int = self.__vertices.size
        texSzie: int = self.__texcoords.size
        normSize: int = self.__normals.size

        if (vertSize != normSize) or (2*vertSize != 3*texSzie):
            errMsg = "Incorrect size of vertices: vert({}), tex({}), nor({})".format(
                self.__vertices.size, self.__texcoords.size, self.__normals.size
            )
            report.emplaceBack("Data error", errMsg)

        return report

    def getJson(self) -> dict:
        a = base64.encodebytes(self.__vertices.tobytes())
        print(a)
        return {
            self.s_field_vertices : base64.encodebytes(self.__vertices.tobytes()).decode(),
            self.s_field_texcoords : base64.encodebytes(self.__texcoords.tobytes()).decode(),
            self.s_field_normals : base64.encodebytes(self.__normals.tobytes()).decode(),
        }



def generateAABBMesh(p1: pri.Vec3, p2: pri.Vec3) -> VertexArray:
    if p1.x < p2.x:
        xOne = p1.x; xTwo = p2.x
    else:
        xOne = p2.x; xTwo = p1.x

    if p1.y < p2.y:
        yOne = p1.y; yTwo = p2.y
    else:
        yOne = p2.y; yTwo = p1.y

    if p1.z < p2.z:
        zOne = p1.z; zTwo = p2.z
    else:
        zOne = p2.z; zTwo = p1.z

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

    return VertexArray(vertices, texCoords, normals)
