import json

import level.build_info as bfi
import level.level_builder as lvb


def main():
    level = lvb.LevelBuilder("test_level")

    model = bfi.BuildInfo_ModelImported()
    #model.m_model_name = "yuri.obj"
    model.m_actor.m_pos.z = 2
    level.add(model)

    model = bfi.Build_ModelDefined()
    level.add(model)

    level.throwIfNotIntegral()

    with open(level.m_levelName + ".json", "w", encoding="utf8") as file:
        json.dump(level.getJson(), file, indent=4, sort_keys=True)


if __name__ == '__main__':
    main()
