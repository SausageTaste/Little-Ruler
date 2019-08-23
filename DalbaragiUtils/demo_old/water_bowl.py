import random

import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb
import level.datastruct.attrib_leaf as atl
import level.datastruct.attrib_complex as aco


def main():
    level = lvb.LevelBuilder("water_bowl")

    ########

    model = bfi.BuildInfo_ModelImported()

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(2, 1, 5)
    model.addActor(actor)

    model.setModelID("asset::sheeptangball.dae")
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelImported()
    actor = aco.ActorInfo()
    actor.getPosHandle().setZ(5)
    model.addActor(actor)
    model.setModelID("asset::yuri_cso1.obj")
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelImported()
    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-2)
    actor.getPosHandle().setZ(5)
    model.addActor(actor)
    model.setModelID("asset::yuri_cso2.obj")
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelImported()
    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(0, 0, 17)
    actor.setScale(3)
    model.addActor(actor)
    model.setModelID("asset::63building.obj")
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("floor_upper")
    model.getMeshHandle().setAABB(atl.Vec3(0, -1, 0), atl.Vec3(20, 0, 50))
    model.getMaterialHandle().setDiffuseMap("asset::0021di.png")
    model.getMaterialHandle().setTexScale(10, 25)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-10)
    actor.getPosHandle().setZ(-20)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("floor_below")
    model.getMeshHandle().setAABB(atl.Vec3(0, -1, 0), atl.Vec3(50, 0, 50))
    model.getMaterialHandle().setDiffuseMap("asset::0021di.png")
    model.getMaterialHandle().setTexScale(25, 25)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-10)
    actor.getPosHandle().setY(-30)
    actor.getPosHandle().setZ(-20)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("wallX")
    model.getMeshHandle().setAABB(atl.Vec3(0, -50, 0), atl.Vec3(1, 10, 50))
    model.getMaterialHandle().setDiffuseMap("asset::0021di.png")
    model.getMaterialHandle().setTexScale(25/2, 30/2)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-11)
    actor.getPosHandle().setZ(-20)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-11 + 51)
    actor.getPosHandle().setZ(-20)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("wallY")
    model.getMeshHandle().setAABB(atl.Vec3(0, -50, 0), atl.Vec3(50, 10, 1))
    model.getMaterialHandle().setDiffuseMap("asset::0021di.png")
    model.getMaterialHandle().setTexScale(25/2, 30/2)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-10)
    actor.getPosHandle().setZ(-21)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-10)
    actor.getPosHandle().setZ(-21 + 51)
    model.addActor(actor)

    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("tall")
    model.getMeshHandle().setAABB(atl.Vec3(-4, -50, -9), atl.Vec3(4, 50, -1))
    model.getMaterialHandle().setTexScale(2, 25)
    model.getMaterialHandle().setDiffuseMap("asset::grass1.png")

    actor = aco.ActorInfo()
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(30, -50, 3)
    model.addActor(actor)

    level.add(model)

    ########

    water = bfi.BuildInfo_WaterPlane()
    water.getPosHandle().setXYZ(-11, 0.3, -21)
    water.setWidth(52)
    water.setHeight(52)

    water.setSpecStreng(3)
    water.setDarkestDepthPoint(40)
    water.setWaveStreng(0.01)
    water.getDepthColorHandle().setXYZ(0.07, 0.07, 0.2)
    water.setReflectivity(0.3)

    level.add(water)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("box")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(2, 2, 2))
    model.getMaterialHandle().setTexScale(1, 1)
    model.getMaterialHandle().setDiffuseMap("asset::missing_no.png")

    for _ in range(30):
        x = random.randint(-8, 38)
        y = random.randint(-30, 5)
        z = random.randint(-17, 22)

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
