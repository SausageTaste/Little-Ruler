import os
import json

import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.map.objectdef as ode
import dalutils.util.path as pth
import dalutils.util.reporter as rep


class MapMetadata(inf.IDataBlock):
    def __init__(self):
        self.__binVersion = pri.IntValue()

        self.setDefault()

        super().__init__({
            "bin_version": self.__binVersion,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__binVersion.getBinary()
        return data

    def setDefault(self) -> None:
        self.__binVersion.set(1)

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        pass


class MapChunkBuilder(inf.IDataBlock):
    def __init__(self):
        self.__metadata = MapMetadata()
        self.__embeddedModels = pri.UniformList(ode.ModelEmbedded)
        self.__importedModels = pri.UniformList(ode.ModelImported)
        self.__waterPlanes = pri.UniformList(ode.WaterPlane)
        self.__plights = pri.UniformList(ode.LightPoint)

        self.setDefault()

        super().__init__({
            "metadata": self.__metadata,
            "embedded_models": self.__embeddedModels,
            "imported_models": self.__importedModels,
            "water_planes": self.__waterPlanes,
            "point_lights": self.__plights,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__metadata.getBinary()
        data += self.__embeddedModels.getBinary()
        data += self.__importedModels.getBinary()
        data += self.__waterPlanes.getBinary()
        data += self.__plights.getBinary()

        return data

    def setDefault(self) -> None:
        self.__metadata.setDefault()
        self.__embeddedModels.clear()
        self.__importedModels.clear()
        self.__waterPlanes.clear()
        self.__plights.clear()

    def fillErrReport(self, journal: rep.ErrorJournal) -> None:
        child = rep.ErrorJournal("metadata")
        self.__metadata.fillErrReport(child)
        journal.addChildren(child)

        for i, modelEmbed in enumerate(self.__embeddedModels):
            child = rep.ErrorJournal("model embeded [{}]".format(i))
            modelEmbed.fillErrReport(child)
            journal.addChildren(child)

        for i, modelEmbed in enumerate(self.__importedModels):
            child = rep.ErrorJournal("model imported [{}]".format(i))
            modelEmbed.fillErrReport(child)
            journal.addChildren(child)

        for i, water in enumerate(self.__waterPlanes):
            child = rep.ErrorJournal("water plane [{}]".format(i))
            water.fillErrReport(child)
            journal.addChildren(child)

        for i, light in enumerate(self.__plights):
            child = rep.ErrorJournal("point light [{}]".format(i))
            light.fillErrReport(child)
            journal.addChildren(child)

    def newEmbeddedModel(self) -> ode.ModelEmbedded:
        obj = ode.ModelEmbedded()
        self.__embeddedModels.pushBack(obj)
        return obj

    def newImportedModel(self) -> ode.ModelImported:
        obj = ode.ModelImported()
        self.__importedModels.pushBack(obj)
        return obj

    def newWaterPlane(self) -> ode.WaterPlane:
        obj = ode.WaterPlane()
        self.__waterPlanes.pushBack(obj)
        return obj

    def newPointLight(self) -> ode.LightPoint:
        obj = ode.LightPoint()
        self.__plights.pushBack(obj)
        return obj


def exportJson(mapData: MapChunkBuilder, outputPath: str) -> None:
    journal = rep.ErrorJournal(outputPath)
    mapData.fillErrReport(journal)
    journal.assertNoErr()

    pth.createFolderAlong(os.path.split(outputPath)[0])

    with open(outputPath, "w") as file:
        json.dump(mapData.getJson(), file, indent=4, sort_keys=False)
