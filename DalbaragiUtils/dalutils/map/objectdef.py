import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.map.datablock as blk
import dalutils.map.collider as col
import dalutils.util.binutil as but
import dalutils.util.reporter as rep


class ModelEmbedded(inf.IDataBlock):
    def __init__(self):
        self.__name = pri.StringValue()
        self.__renderUnits = pri.UniformList(blk.RenderUnit)
        self.__staticActors = pri.UniformList(blk.StaticActor)
        self.__flagDetailedCollider = pri.BoolValue()

        self.setDefault()

        super().__init__({
            "name": self.__name,
            "render_units": self.__renderUnits,
            "static_actors": self.__staticActors,
            "detailed_collider": self.__flagDetailedCollider,
        })

    def __str__(self):
        return "{}{{ name={}, size={} }}".format(type(self).__name__, self.__name.get(), self.__renderUnits.__len__())

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__name.getBinary()
        data += self.__renderUnits.getBinary()
        data += self.__staticActors.getBinary()
        data += self.__flagDetailedCollider.getBinary()
        data += pri.BoolValue(self.__hasRotatingActor()).getBinary()

        return data

    def setDefault(self) -> None:
        self.__name.set("")
        self.__renderUnits.clear()
        self.__staticActors.clear()
        self.__flagDetailedCollider.set(False)

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        for i, unit in enumerate(self.__renderUnits):
            child = rep.ErrorJournal("render unit [{}]".format(i))
            unit.fillErrReport(child)
            journal.addChildren(child)

        for i, actor in enumerate(self.__staticActors):
            child = rep.ErrorJournal("static actor [{}]".format(i))
            actor.fillErrReport(child)
            journal.addChildren(child)

    def newRenderUnit(self) -> blk.RenderUnit:
        unit = blk.RenderUnit()
        self.__renderUnits.pushBack(unit)
        return unit

    def newStaticActor(self) -> blk.StaticActor:
        actor = blk.StaticActor()
        self.__staticActors.pushBack(actor)
        return actor

    @property
    def m_name(self):
        return self.__name
    @property
    def m_flagDetailedCollider(self):
        return self.__flagDetailedCollider

    def __hasRotatingActor(self) -> bool:
        for sactor in self.__staticActors:
            sactor: blk.StaticActor
            if not sactor.m_transform.m_quat.isDefaultValue():
                return True

        return False

    def __buildBoundingBox(self) -> col.ICollider:
        if self.__hasRotatingActor():
            bounding = col.Sphere()
            for unit in self.__renderUnits:
                unit: blk.RenderUnit
                bounding = bounding.makeContaining(unit.m_mesh.makeSphere())
            bounding.setCenterOrigin()
            return bounding
        else:
            bounding = col.AABB()
            for unit in self.__renderUnits:
                unit: blk.RenderUnit
                bounding = bounding.makeContaining(unit.m_mesh.makeAABB())
            return bounding

    def __buildDetailedCol(self) -> col.TriangleSoup:
        soup = col.TriangleSoup()

        for unit in self.__renderUnits:
            unit: blk.RenderUnit
            soup += unit.m_mesh.makeTriangleSoup()

        return soup


class ModelImported(inf.IDataBlock):
    def __init__(self):
        self.__resourceID = pri.StringValue()
        self.__staticActors = pri.UniformList(blk.StaticActor)
        self.__flagDetailedCollider = pri.BoolValue()

        self.setDefault()

        super().__init__({
            "resource_id": self.__resourceID,
            "static_actors": self.__staticActors,
            "detailed_collider": self.__flagDetailedCollider,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__resourceID.getBinary()
        data += self.__staticActors.getBinary()
        data += self.__flagDetailedCollider.getBinary()

        return data

    def setDefault(self) -> None:
        self.__resourceID.set("")
        self.__staticActors.clear()
        self.__flagDetailedCollider.set(False)

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        pass

    def newStaticActor(self) -> blk.StaticActor:
        actor = blk.StaticActor()
        self.__staticActors.pushBack(actor)
        return actor

    @property
    def m_resourceID(self):
        return self.__resourceID
    @property
    def m_flagDetailedCollider(self):
        return self.__flagDetailedCollider


class WaterPlane(inf.IDataBlock):
    def __init__(self):
        self.__centerPos = pri.Vec3()
        self.__width = pri.FloatValue()
        self.__height = pri.FloatValue()
        self.__flowSpeed = pri.FloatValue()
        self.__waveStrength = pri.FloatValue()
        self.__darkestDepth = pri.FloatValue()
        self.__deepColor = pri.Vec3()
        self.__reflectivity = pri.FloatValue()

        self.setDefault()

        super().__init__({
            "center_pos" : self.__centerPos,
            "width" : self.__width,
            "height" : self.__height,
            "flow_speed" : self.__flowSpeed,
            "wave_strength" : self.__waveStrength,
            "darkest_depth" : self.__darkestDepth,
            "deep_color" : self.__deepColor,
            "reflectivity" : self.__reflectivity,
        })

    def getBinary(self) -> bytearray:
        return self._makeBinaryAsListed()

    def setDefault(self) -> None:
        self.__centerPos.setXYZ(0, 0, 0)
        self.__width.set(1)
        self.__height.set(1)
        self.__flowSpeed.set(0.03)
        self.__waveStrength.set(0.02)
        self.__darkestDepth.set(5)
        self.__deepColor.setXYZ(0.07, 0.07, 0.15)
        self.__reflectivity.set(0.05)

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        if self.__width.get() == 0.0:
            journal.addNote(rep.ErrorNote("width -> It must not be 0."))
        if self.__height.get() == 0.0:
            journal.addNote(rep.ErrorNote("height -> It must not be 0."))

    @property
    def m_centerPos(self):
        return self.__centerPos
    @property
    def m_width(self):
        return self.__width
    @property
    def m_height(self):
        return self.__height
    @property
    def m_flowSpeed(self):
        return self.__flowSpeed
    @property
    def m_waveStrength(self):
        return self.__waveStrength
    @property
    def m_darkestDepth(self):
        return self.__darkestDepth
    @property
    def m_deepColor(self):
        return self.__deepColor
    @property
    def m_reflectivity(self):
        return self.__reflectivity


class LightPoint(inf.IDataBlock):
    def __init__(self):
        self.__color = pri.Vec3()
        self.__pos = pri.Vec3()
        self.__maxDistance = pri.FloatValue()

        self.setDefault()

        super().__init__({
            "color" : self.__color,
            "pos" : self.__pos,
            "max_distance" : self.__maxDistance,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__color.getBinary()
        data += self.__pos.getBinary()
        data += self.__maxDistance.getBinary()

        return data

    def setDefault(self) -> None:
        self.__color.setXYZ(1, 1, 1)
        self.__pos.setXYZ(0, 0, 0)
        self.__maxDistance.set(5)

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        pass

    @property
    def m_color(self):
        return self.__color
    @property
    def m_pos(self):
        return self.__pos
    @property
    def m_maxDistance(self):
        return self.__maxDistance
