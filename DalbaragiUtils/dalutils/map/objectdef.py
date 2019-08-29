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

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__name.getBinary()
        data += self.__renderUnits.getBinary()
        data += self.__staticActors.getBinary()

        bounding = self.__buildBoundingBox()
        data += but.get2BytesInt(bounding.getTypeCode())
        data += bounding.getBinary()

        if not self.__flagDetailedCollider.get():
            data += but.get2BytesInt(0)
        else:
            soup = self.__buildDetailedCol()
            data += but.get2BytesInt(soup.getTypeCode())
            data += soup.getBinary()

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

    @property
    def m_name(self):
        return self.__name
    @property
    def m_renderUnits(self):
        return self.__renderUnits
    @property
    def m_staticActors(self):
        return self.__staticActors
    @property
    def m_flagDetailedCollider(self):
        return self.__flagDetailedCollider

    def __hasRotatingActor(self) -> bool:
        for sactor in self.__staticActors:
            sactor: blk.StaticActor
            if not sactor.m_transform.m_quat.isDefaultValue():
                return False

        return True

    def __buildBoundingBox(self) -> col.ICollider:
        if self.__hasRotatingActor():
            bounding = col.Sphere()
            for unit in self.__renderUnits:
                unit: blk.RenderUnit
                bounding = bounding.makeContaining(unit.m_mesh.makeSphere())
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
