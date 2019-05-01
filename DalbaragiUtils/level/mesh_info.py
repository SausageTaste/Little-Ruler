import numpy as np

import level.primitive_data as pri


class VertexArray:
    def __init__(self, v: np.ndarray, t: np.ndarray, n: np.ndarray):
        if isinstance(v, np.ndarray): raise ValueError("v is " + type(v).__name__)
        if isinstance(t, np.ndarray): raise ValueError("t is " + type(t).__name__)
        if isinstance(n, np.ndarray): raise ValueError("n is " + type(n).__name__)

        self.__vertices = v
        self.__texcoords = t
        self.__normals = n


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
