import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb
import level.datastruct.attrib_leaf as atl
import level.datastruct.attrib_complex as aco


def main():
    level = lvb.LevelBuilder("test_level")

    model = bfi.BuildInfo_ModelImported()
    model.addActor(aco.ActorInfo())
    level.add(model)

    model = bfi.BuildInfo_ModelDefined()
    model.getMeshHandle().setAABB(atl.Vec3(-50, -1, -50), atl.Vec3(50, 0, 50))
    model.addActor(aco.ActorInfo())
    level.add(model)

    level.saveToFile()


if __name__ == '__main__':
    main()
