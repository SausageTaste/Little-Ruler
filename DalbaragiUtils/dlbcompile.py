import os
import sys
import json
from typing import List, Optional

import level.datastruct.level_builder as lvb


class CompileWorkReciepe:
    def __init__(self):
        self.m_fileNames: List[str] = []


def getOnlyFileName(path: str) -> str:
    return os.path.split(os.path.splitext(path)[0])[1]

def getWorkProperties(args: list) -> Optional[CompileWorkReciepe]:
    work = CompileWorkReciepe()
    errOnce = False

    for arg in args[1:]:
        if arg.endswith(".json"):
            if os.path.isfile(arg):
                work.m_fileNames.append(arg)
            else:
                print("File not exist: " + arg)
                errOnce = True
        else:
            print("Invalid parameter: " + arg)
            errOnce = True

    if errOnce:
        return None
    else:
        return work


def main(args: list):
    work = getWorkProperties(args)
    if work is None:
        print("\nTerminate due to error during parsing args.")
        return

    for filePath in work.m_fileNames:
        fileName = getOnlyFileName(filePath)
        with open(filePath, "r", encoding="utf8") as file:
            data = json.load(file)
            level = lvb.LevelBuilder(fileName)
            level.setJson(data)
            lvb.saveLevelBinary(level)

            print("Compile done: " + fileName + ".dlb")


if __name__ == '__main__':
    main(sys.argv)
