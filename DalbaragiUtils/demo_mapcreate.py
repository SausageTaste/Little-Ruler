import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb
import level.datastruct.attrib_leaf as atl
import level.datastruct.attrib_complex as aco


def main():
    level = lvb.LevelBuilder("test_level")

    model = bfi.BuildInfo_ModelImported()

    actor = aco.ActorInfo()
    actor.setName("Yuri1")
    actor.getQuatHandle().rotate(180, (0, 1, 0))
    actor.getPosHandle().setX(2)
    actor.getPosHandle().setZ(4)
    model.addActor(actor)

    model.setModelID("asset::yuri.obj")
    level.add(model)

    ########

    light = bfi.BuildInfo_LightPoint()
    light.setName("center_light")
    light.getColorHandle().setXYZ(0.5, 0.2, 0.2)
    light.getPosHandle().setY(1.5)
    light.getPosHandle().setZ(1)
    level.add(light)

    ########

    light = bfi.BuildInfo_LightPoint()
    light.setName("center_light_2")
    light.getColorHandle().setXYZ(0.2, 0.2, 0.5)
    light.getPosHandle().setY(1.5)
    light.getPosHandle().setZ(6)
    level.add(light)

    ########

    model = bfi.BuildInfo_ModelImported()
    actor = aco.ActorInfo()
    actor.getPosHandle().setZ(5)
    #actor.getQuatHandle().rotate(-90, (0, 1, 0))
    model.addActor(actor)
    model.setModelID("test::academy.obj")
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("floor")
    model.getMeshHandle().setAABB(atl.Vec3(0, -1, 0), atl.Vec3(50, 0, 50))
    model.getMaterialHandle().setDiffuseMap("asset::0021di.png")
    model.getMaterialHandle().setTexScale(20, 20)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-10)
    actor.getPosHandle().setZ(-20)

    model.addActor(actor)
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("wall")
    model.getMeshHandle().setAABB(atl.Vec3(-4, 0, -10), atl.Vec3(4, 10, -1))
    model.getMaterialHandle().setDiffuseMap("asset::grass1.png")

    actor = aco.ActorInfo()
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setXYZ(30, -2, 3)
    model.addActor(actor)

    level.add(model)

    ########

    water = bfi.BuildInfo_WaterPlane()
    water.getPosHandle().setX(-10)
    water.getPosHandle().setY(0.5)
    water.getPosHandle().setZ(-20)
    water.setWidth(50)
    water.setHeight(50)
    level.add(water)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("box")
    model.getMeshHandle().setAABB(atl.Vec3(0, 0, 0), atl.Vec3(2, 2, 2))
    model.getMaterialHandle().setTexScale(2, 2)
    model.getMaterialHandle().setDiffuseMap("asset::grass1.png")

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(5)
    actor.getPosHandle().setZ(2)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(7)
    actor.getPosHandle().setY(-0.5)
    actor.getPosHandle().setZ(-1)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(2)
    actor.getPosHandle().setZ(-4)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-5)
    actor.getPosHandle().setZ(3)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.getPosHandle().setX(-3)
    actor.getPosHandle().setY(-1)
    actor.getPosHandle().setZ(6)
    model.addActor(actor)

    level.add(model)

    ########

    lvb.saveLevelJson(level)
    print("Saved: " + level.getLevelName() + ".json")
    print(level.getDataReport().getFormattedStr())


if __name__ == '__main__':
    main()
