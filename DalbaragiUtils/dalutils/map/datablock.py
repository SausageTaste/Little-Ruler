import math
from typing import Generator

import glm
import numpy as np

import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.map.collider as col


class Material(inf.IDataBlock):
    def __init__(self):
        self.__diffuseColor = pri.Vec3()
        self.__shininess = pri.FloatData()
        self.__specStreng = pri.FloatData()
        self.__texScaleX = pri.FloatData()
        self.__texScaleY = pri.FloatData()
        self.__diffuseMap = pri.StrData()
        self.__specularMap = pri.StrData()
        self.__flagAlphaBlend = pri.BoolValue()

        self.setDefault()

        super().__init__({
            "base_color": self.__diffuseColor,
            "shininess": self.__shininess,
            "spec_strength": self.__specStreng,
            "tex_scale_x": self.__texScaleX,
            "tex_scale_y": self.__texScaleY,
            "diffuse_map": self.__diffuseMap,
            "specular_map": self.__specularMap,
            "alpha_blend": self.__flagAlphaBlend,
        })

    def setDefault(self) -> None:
        self.__diffuseColor.setXYZ(1, 1, 1)
        self.__shininess.set(32)
        self.__specStreng.set(1)
        self.__texScaleX.set(1)
        self.__texScaleY.set(1)
        self.__diffuseMap.set("")
        self.__specularMap.set("")
        self.__flagAlphaBlend.set(False)

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__diffuseColor.getBinary()
        data += self.__shininess.getBinary()
        data += self.__specStreng.getBinary()
        data += self.__texScaleX.getBinary()
        data += self.__texScaleY.getBinary()
        data += self.__diffuseMap.getBinary()
        data += self.__specularMap.getBinary()
        data += self.__flagAlphaBlend.getBinary()
        return data

    @property
    def m_diffuseColor(self):
        return self.__diffuseColor
    @property
    def m_shininess(self):
        return self.__shininess
    @property
    def m_specStreng(self):
        return self.__specStreng
    @property
    def m_texScaleX(self):
        return self.__texScaleX
    @property
    def m_texScaleY(self):
        return self.__texScaleY
    @property
    def m_diffuseMap(self):
        return self.__diffuseMap
    @property
    def m_specularMap(self):
        return self.__specularMap


class Mesh(inf.IDataBlock):
    def __init__(self):
        self.__vertices = pri.FloatArray()
        self.__texcoords = pri.FloatArray()
        self.__normals = pri.FloatArray()

        self.setDefault()

        super().__init__({
            "vertices": self.__vertices,
            "texcoords": self.__texcoords,
            "normals": self.__normals
        })

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__vertices.getBinary()
        data += self.__texcoords.getBinary()
        data += self.__normals.getBinary()

        return data

    def setDefault(self) -> None:
        self.__vertices.clear()
        self.__texcoords.clear()
        self.__normals.clear()

    def makeAABB(self) -> col.AABB:
        self.assertValidity()

        aabb = col.AABB()

        for vertex in self.__genVertices():
            aabb.makeItCover(*vertex.getXYZ())

        if aabb.calcVolume() <= 0.0:
            ValueError("AABB volume is 0.")

        return aabb

    def makeSphere(self) -> col.Sphere:
        self.assertValidity()

        vertCount = 0
        vertSum = pri.Vec3(0, 0, 0)
        for vertex in self.__genVertices():
            vertSum += vertex
            vertCount += 1
        avrgVert = vertSum / vertCount

        mostDistanceSqr = 0.0
        for vertex in self.__genVertices():
            distanceSqr = (avrgVert - vertex).getLenSqr()
            if distanceSqr > mostDistanceSqr:
                mostDistanceSqr = distanceSqr

        return col.Sphere(avrgVert.getX(), avrgVert.getY(), avrgVert.getZ(), math.sqrt(mostDistanceSqr))

    def makeTriangleSoup(self) -> col.TriangleSoup:
        self.assertValidity()

        soup = col.TriangleSoup()
        it = self.__genVertices()

        while True:
            tri = col.Triangle()
            try:
                tri.m_p1.set(next(it))
            except StopIteration:
                return soup
            else:
                tri.m_p2.set(next(it))
                tri.m_p3.set(next(it))
                soup.m_triangles.pushBack(tri)

    def assertValidity(self) -> None:
        if (self.__vertices.getSize() % 3) != 0:
            raise ValueError("Size of Mesh::m_vertices is not multiple of 3.")

        numVert = int(self.__vertices.getSize() / 3)
        if self.__texcoords.getSize() != (2 * numVert):
            raise ValueError("Numbers of vertices and texcoords are invalid.")

        if self.__normals.getSize() != self.__vertices.getSize():
            raise ValueError("Vertices and normals have different num of floats.")

    # Point coordinates follow OpenGL's texture coordinate system.
    # From upper left corner, rotate counter clock wise.
    # Also the faces are winded in ccw.
    def buildIn_rect(self, p01: pri.Vec3, p00: pri.Vec3, p10: pri.Vec3, p11: pri.Vec3):
        vertices: np.ndarray = np.array((
            *p01.getXYZ(),
            *p00.getXYZ(),
            *p10.getXYZ(),
            *p01.getXYZ(),
            *p10.getXYZ(),
            *p11.getXYZ(),
        ), dtype=np.float32)
        assert len(vertices) == 18

        texcoords: np.ndarray = np.array((
            0.0, 1.0,
            0.0, 0.0,
            1.0, 0.0,
            0.0, 1.0,
            1.0, 0.0,
            1.0, 1.0,
        ), dtype=np.float32)
        assert len(texcoords) == 12

        a = p00 - p01
        b = p10 - p01
        c = p11 - p01

        normal1 = glm.cross(a.getVec(), b.getVec())
        normal2 = glm.cross(b.getVec(), c.getVec())

        normals: np.ndarray = np.array((
            normal1.x, normal1.y, normal1.z,
            normal1.x, normal1.y, normal1.z,
            normal1.x, normal1.y, normal1.z,
            normal2.x, normal2.y, normal2.z,
            normal2.x, normal2.y, normal2.z,
            normal2.x, normal2.y, normal2.z,
        ), dtype=np.float32)
        assert len(normals) == 18

        self.__vertices.append(vertices)
        self.__texcoords.append(texcoords)
        self.__normals.append(normals)

    def __genVertices(self) -> Generator[pri.Vec3, None, None]:
        for i in range(int(self.__vertices.getSize() / 3)):
            x = self.__vertices[i]
            y = self.__vertices[i + 1]
            z = self.__vertices[i + 2]
            yield pri.Vec3(x, y, z)


class RenderUnit(inf.IDataBlock):
    def __init__(self):
        self.__mesh = Mesh()
        self.__material = Material()

        self.setDefault()

        super().__init__({
            "mesh": self.__mesh,
            "material": self.__material,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__mesh.getBinary()
        data += self.__material.getBinary()
        return data

    def setDefault(self) -> None:
        self.__mesh.setDefault()
        self.__material.setDefault()

    @property
    def m_mesh(self):
        return self.__mesh
    @property
    def m_material(self):
        return self.__material


class Transform(inf.IDataBlock):
    def __init__(self):
        self.__pos = pri.Vec3()
        self.__quat = pri.Quat()
        self.__scale = pri.FloatData()

        self.setDefault()

        super().__init__({
            "pos": self.__pos,
            "quat": self.__quat,
            "scale": self.__scale,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__pos.getBinary()
        data += self.__quat.getBinary()
        data += self.__scale.getBinary()
        return data

    def setDefault(self) -> None:
        self.__pos.setXYZ(0, 0, 0)
        self.__quat.setDefault()
        self.__scale.set(1)

    @property
    def m_pos(self):
        return self.__pos
    @property
    def m_quat(self):
        return self.__quat
    @property
    def m_scale(self):
        return self.__scale


class StaticActor(inf.IDataBlock):
    def __init__(self):
        self.__actorName = pri.StrData()
        self.__transform = Transform()

        self.setDefault()

        super().__init__({
            "name": self.__actorName,
            "transform": self.__transform,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__actorName.getBinary()
        data += self.__transform.getBinary()
        return data

    def setDefault(self) -> None:
        self.__actorName.set("")
        self.__transform.setDefault()

    @property
    def m_name(self):
        return self.__actorName
    @property
    def m_transform(self):
        return self.__transform
