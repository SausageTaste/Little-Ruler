import random

import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb
import level.datastruct.attrib_leaf as atl
import level.datastruct.attrib_complex as aco


def main():
    level = lvb.LevelBuilder("vast_citylike")

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("floor_upper")
    model.getMeshHandle().setAABB(atl.Vec3(-150, -1, -150), atl.Vec3(150, 0, 150))
    model.getMaterialHandle().setDiffuseMap("asset::0021di.png")
    model.getMaterialHandle().setTexScale(75, 75)

    actor = aco.ActorInfo()
    model.addActor(actor)

    level.add(model)

    ########

    water = bfi.BuildInfo_WaterPlane()
    water.getPosHandle().setXYZ(-150, 1, -150)
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
    model.setModelID("building")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(4, 30, 4))
    model.getMaterialHandle().setTexScale(1, 30/4)
    model.getMaterialHandle().setDiffuseMap("asset::missing_no.png")

    for _ in range(200):
        x = random.randint(-80, 80)
        y = random.randint(-20, 0)
        z = random.randint(-80, 80)

        actor = aco.ActorInfo()
        actor.getPosHandle().setXYZ(x, y, z)
        model.addActor(actor)

    level.add(model)

    ########

    lvb.saveLevelJson(level)
    print("Saved: " + level.getLevelName() + ".json")
    print(level.getDataReport().getFormattedStr())


if __name__ == '__main__':
    main()
