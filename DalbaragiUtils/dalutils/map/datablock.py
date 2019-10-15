import math
from typing import Generator, Optional

import glm
import numpy as np

import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.map.collider as col
import dalutils.map.meshbuilder as mes
import dalutils.util.reporter as rep


class Material(inf.IDataBlock):
    def __init__(self):
        self.__shininess = pri.FloatValue()
        self.__specStreng = pri.FloatValue()
        self.__reflectivity = pri.FloatValue()

        self.__roughness = pri.FloatValue()
        self.__metallic = pri.FloatValue()

        self.__texScaleX = pri.FloatValue()
        self.__texScaleY = pri.FloatValue()

        self.__diffuseMap = pri.StringValue()
        self.__roughnessMap = pri.StringValue()
        self.__metallicMap = pri.StringValue()

        self.setDefault()

        super().__init__({
            "shininess": self.__shininess,
            "spec_strength": self.__specStreng,
            "reflectivity": self.__reflectivity,

            "roughness": self.__roughness,
            "metallic": self.__metallic,

            "tex_scale_x": self.__texScaleX,
            "tex_scale_y": self.__texScaleY,

            "diffuse_map": self.__diffuseMap,
            "roughness_map": self.__roughnessMap,
            "metallic_map": self.__metallicMap,
        })

    def getBinary(self) -> bytearray:
        return self._makeBinaryAsListed()

    def setDefault(self) -> None:
        self.__shininess.set(32)
        self.__specStreng.set(1)
        self.__reflectivity.set(0.1)

        self.__roughness.set(0.3)
        self.__metallic.set(0)

        self.__texScaleX.set(1)
        self.__texScaleY.set(1)

        self.__diffuseMap.set("")
        self.__roughnessMap.set("")
        self.__metallicMap.set("")

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        if self.__texScaleX.get() == 0.0:
            note = rep.ErrorNote("tex_scale_x -> It mustn't be zero.", rep.ErrorLevel.ERRO)
            journal.addNote(note)
        if self.__texScaleY.get() == 0.0:
            note = rep.ErrorNote("tex_scale_y -> It mustn't be zero.", rep.ErrorLevel.ERRO)
            journal.addNote(note)

        if self.__diffuseMap.get() == "":
            journal.addNote(rep.ErrorNote("diffuse_map -> It must be defined.", rep.ErrorLevel.ERRO))

    @property
    def m_shininess(self):
        return self.__shininess
    @property
    def m_specStreng(self):
        return self.__specStreng
    @property
    def m_reflectivity(self):
        return self.__reflectivity

    @property
    def m_roughness(self):
        return self.__roughness
    @property
    def m_metallic(self):
        return self.__metallic

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
    def m_roughnessMap(self):
        return self.__roughnessMap
    @property
    def m_metallicMap(self):
        return self.__metallicMap


class Mesh(inf.IDataBlock):
    def __init__(self):
        self.__vertices = pri.FloatList()
        self.__texcoords = pri.FloatList()
        self.__normals = pri.FloatList()

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

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        if (self.__vertices.getSize() % 9) != 0:
            journal.addNote(rep.ErrorNote("vertices -> Size is not multiple of 9.", rep.ErrorLevel.ERRO))

        numVert = int(self.__vertices.getSize() / 3)
        if self.__texcoords.getSize() != (2 * numVert):
            journal.addNote(rep.ErrorNote("texcoords -> Size of texcoords does not match that of vertices.", rep.ErrorLevel.ERRO))

        if self.__normals.getSize() != self.__vertices.getSize():
            journal.addNote(rep.ErrorNote("normals -> Sizes of Normals and vertices are different.", rep.ErrorLevel.ERRO))

    def makeAABB(self) -> col.AABB:
        self.assertValidity()

        aabb = col.AABB()
        for vertex in self.__genVertices():
            aabb.makeItCover(*vertex.getXYZ())

        journal = rep.ErrorJournal("tmp")
        aabb.fillErrReport(journal)
        journal.assertNoErr()

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

        sphere = col.Sphere(avrgVert.getX(), avrgVert.getY(), avrgVert.getZ(), math.sqrt(mostDistanceSqr))
        journal = rep.ErrorJournal("tmp")
        sphere.fillErrReport(journal)
        journal.assertNoErr()

        return sphere

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

                journal = rep.ErrorJournal("tmp")
                tri.fillErrReport(journal)
                journal.assertNoErr()

                soup.m_triangles.pushBack(tri)

    def assertValidity(self) -> None:
        journal = rep.ErrorJournal("tmp")
        self.fillErrReport(journal)
        if journal.hasError():
            raise ValueError("\n" + journal.makeReportText()[0])

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

        normal1 = glm.normalize(glm.cross(a.getVec(), b.getVec()))
        normal2 = glm.normalize(glm.cross(b.getVec(), c.getVec()))
        normalAvrg = (normal1 + normal2) * 0.5

        normals: np.ndarray = np.array((
            normalAvrg.x, normalAvrg.y, normalAvrg.z,
            normal1.x, normal1.y, normal1.z,
            normalAvrg.x, normalAvrg.y, normalAvrg.z,
            normalAvrg.x, normalAvrg.y, normalAvrg.z,
            normalAvrg.x, normalAvrg.y, normalAvrg.z,
            normal2.x, normal2.y, normal2.z,
        ), dtype=np.float32)
        assert len(normals) == 18

        self.__vertices.append(vertices)
        self.__texcoords.append(texcoords)
        self.__normals.append(normals)

    def buildIn(self, builder: mes.IMeshBuilder):
        vertices, texcoords, normals = builder.makeMeshData()

        self.__vertices.append(vertices)
        self.__texcoords.append(texcoords)
        self.__normals.append(normals)

    def __genVertices(self) -> Generator[pri.Vec3, None, None]:
        assert (self.__vertices.getSize() % 3) == 0

        numVertices = self.__vertices.getSize() // 3
        for i in range(numVertices):
            x = self.__vertices[3 * i]
            y = self.__vertices[3 * i + 1]
            z = self.__vertices[3 * i + 2]
            yield pri.Vec3(x, y, z)


class RenderUnit(inf.IDataBlock):
    def __init__(self):
        self.__mesh = pri.Variant(mes.Rect, mes.HeightGrid)
        self.__material = Material()

        self.setDefault()

        super().__init__({
            "mesh": self.__mesh,
            "material": self.__material,
        })

    def getBinary(self) -> bytearray:
        return self._makeBinaryAsListed()

    def setDefault(self) -> None:
        self.__mesh.clear()
        self.__material.setDefault()

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        if not self.__mesh.isValid():
            journal.addNote(rep.ErrorNote("Mesh hasn't been defined."))

        try:
            child = rep.ErrorJournal("mesh")
            self.__mesh.get().fillErrReport(child)
        except AttributeError:
            pass
        else:
            journal.addChildren(child)

        child = rep.ErrorJournal("material")
        self.__material.fillErrReport(child)
        journal.addChildren(child)

    @property
    def m_mesh(self):
        return self.__mesh
    @m_mesh.setter
    def m_mesh(self, meshBuilder: mes.IMeshBuilder):
        self.__mesh.set(meshBuilder)
    @property
    def m_material(self):
        return self.__material


class Transform(inf.IDataBlock):
    def __init__(self):
        self.__pos = pri.Vec3()
        self.__quat = pri.Quat()
        self.__scale = pri.FloatValue()

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

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        if self.__scale.get() == 0.0:
            journal.addNote(rep.ErrorNote("scale -> Scale factor is zero."))

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
        self.__actorName = pri.StringValue()
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

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        child = rep.ErrorJournal("transform")
        self.__transform.fillErrReport(child)
        journal.addChildren(child)

    @property
    def m_name(self):
        return self.__actorName
    @property
    def m_transform(self):
        return self.__transform
