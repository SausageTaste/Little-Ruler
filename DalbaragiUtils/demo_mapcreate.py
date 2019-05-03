import json
import os

import level.datastruct.item_builder as bfi
import level.datastruct.level_builder as lvb


def saveToFile(level: lvb.LevelBuilder):
    OUTPUT_FOLDER_NAME: str = "intermediates"
    report = level.getIntegrityReport()
    if report.isFatal():
        raise RuntimeError(report.getFormattedStr())

    jsonData = level.getJson()

    if not os.path.isdir(OUTPUT_FOLDER_NAME):
        os.mkdir(OUTPUT_FOLDER_NAME)

    filePath: str = "{}/{}.json".format(OUTPUT_FOLDER_NAME, level.getLevelName())
    with open(filePath, "w", encoding="utf8") as file:
        json.dump(jsonData, file, indent=4, sort_keys=True)


def main():
    level = lvb.LevelBuilder("test_level")

    model = bfi.BuildInfo_ModelImported()
    level.add(model)

    model = bfi.BuildInfo_ModelDefined()
    level.add(model)

    saveToFile(level)




if __name__ == '__main__':
    main()
