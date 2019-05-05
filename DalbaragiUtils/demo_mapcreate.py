import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb
import level.datastruct.attrib_leaf as atl
import level.datastruct.attrib_complex as aco


def main():
    level = lvb.LevelBuilder("test_level")

    model = bfi.BuildInfo_ModelImported()
    model.addActor(aco.ActorInfo())
    model.setModelID("yuri.obj")
    level.add(model)

    model = bfi.BuildInfo_ModelDefined()
    model.setModelID("floor")
    model.getMeshHandle().setAABB(atl.Vec3(-50, -1, -50), atl.Vec3(50, 0, 50))
    model.getMaterialHandle().setDiffuseMap("grass.png")
    model.addActor(aco.ActorInfo())
    level.add(model)

    lvb.saveLevelJson(level)


if __name__ == '__main__':
    main()
