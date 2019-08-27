import os
import json

import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.map.objectdef as ode
import dalutils.util.path as pth


class MapMetadata(inf.IDataBlock):
    def __init__(self):
        self.__binVersion = pri.IntData(1)

        super().__init__({
            "bin_version": self.__binVersion,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()
        data += self.__binVersion.getBinary()
        return data


class MapChunkBuilder(inf.IDataBlock):
    def __init__(self):
        self.__metadata = MapMetadata()
        self.__modelEmbedded = pri.UniformList(ode.ModelEmbedded)

        super().__init__({
            "metadata": self.__metadata,
            "model_embedded": self.__modelEmbedded,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__metadata.getBinary()
        data += self.__modelEmbedded.getBinary()

        return data

    @property
    def m_modelEmbedded(self):
        return self.__modelEmbedded


def exportJson(mapData: MapChunkBuilder, outputPath: str) -> None:
    pth.createFolderAlong(os.path.split(outputPath)[0])

    with open(outputPath, "w") as file:
        json.dump(mapData.getJson(), file, indent=4, sort_keys=False)
