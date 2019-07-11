import random

import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb
import level.datastruct.attrib_leaf as atl
import level.datastruct.attrib_complex as aco


def main():
    level = lvb.LevelBuilder("hroom")

    ########

    water = bfi.BuildInfo_WaterPlane()
    water.getPosHandle().setXYZ(-150, -1, -150)
    water.setWidth(300)
    water.setHeight(300)

    water.setSpecStreng(3)
    water.setDarkestDepthPoint(40)
    water.setWaveStreng(0.01)
    water.getDepthColorHandle().setXYZ(0.07, 0.07, 0.2)
    water.setReflectivity(0.6)

    level.add(water)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("image1")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(3.0, 4.2, 0))
    model.getMaterialHandle().setDiffuseMap("hentai::sagiri(1).png")

    actor = aco.ActorInfo()
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("image2")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(2.997 * 2, 4, 0))
    model.getMaterialHandle().setDiffuseMap("hentai::_MIO4690.png")

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(4, 0, 0)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("image3")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(1.526 * 2, 3.482 * 2, 0))
    model.getMaterialHandle().setDiffuseMap("hentai::kiri00_2_0403.png")

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(11, 0, 0)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("image4")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(7.36/2, 9/2, 0))
    model.getMaterialHandle().setDiffuseMap("hentai::1556371108.png")

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(15, 0, 0)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("image4")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(1.447*2, 2.047*2, 0))
    model.getMaterialHandle().setDiffuseMap("hentai::72068248_p1.png")

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(19, 0, 0)
    model.addActor(actor)

    level.add(model)

    ########

    lvb.saveLevelJson(level)
    print("Saved: " + level.getLevelName() + ".json")
    print(level.getDataReport().getFormattedStr())


if __name__ == '__main__':
    main()
