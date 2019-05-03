import json

import level.build_info as bfi
import level.mesh_info as mes
import level.level_builder as lvb
import level.primitive_data as pri


def main():
    level = lvb.LevelBuilder("test_level")

    model = bfi.BuildInfo_ModelImported()
    model.m_model_name = "yuri.obj"
    model.m_actor.m_pos.z = 2
    level.add(model)

    model = bfi.BuildInfo_ModelDefined()
    model.setMesh(mes.generateAABBMesh(pri.Vec3(0, 0, 0), pri.Vec3(1, 1, 1)))
    level.add(model)

    level.throwIfNotIntegral()

    jsonData = level.getJson()
    print(jsonData)
    with open(level.m_levelName + ".json", "w", encoding="utf8") as file:
        json.dump(jsonData, file, indent=4, sort_keys=True)


if __name__ == '__main__':
    main()
