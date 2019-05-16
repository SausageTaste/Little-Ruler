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
    actor.getPosHandle().setZ(-5)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.setName("Yuri2")
    actor.getQuatHandle().rotate(-90, (0, 1, 0))
    actor.getPosHandle().setX(5)
    model.addActor(actor)

    actor = aco.ActorInfo()
    actor.setName("Yuri2")
    actor.getQuatHandle().rotate(90, (0, 1, 0))
    actor.getPosHandle().setX(-5)
    model.addActor(actor)

    model.setModelID("yuri.obj")
    level.add(model)

    ########

    light = bfi.BuildInfo_LightSpot()
    light.setName("center_light")
    light.getPosHandle().setY(3)
    level.add(light)

    ########

    model = bfi.BuildInfo_ModelImported()
    model.addActor(aco.ActorInfo())
    model.setModelID("palanquin.obj")
    level.add(model)

    ########

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("floor")
    model.getMeshHandle().setAABB(atl.Vec3(-50, -1, -50), atl.Vec3(50, -0, 50))
    model.getMaterialHandle().setDiffuseMap("grass1.png")
    model.addActor(aco.ActorInfo())
    level.add(model)

    lvb.saveLevelJson(level)
    print("Saved: " + level.getLevelName() + ".json")
    print(level.getDataReport().getFormattedStr())


if __name__ == '__main__':
    main()
