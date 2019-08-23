import dalutils.map.interface as inf
import dalutils.map.primitives as pri
import dalutils.map.objectdef as ode


class MapChunkBuilder(inf.IDataBlock):
    def __init__(self):
        self.__modelEmbedded = pri.UniformList(ode.ModelEmbedded)

        super().__init__({
            "model_embedded": self.__modelEmbedded,
        })

    def getBinary(self) -> bytearray:
        data = bytearray()

        data += self.__modelEmbedded.getBinary()

        return data

    @property
    def m_modelEmbedded(self):
        return self.__modelEmbedded
