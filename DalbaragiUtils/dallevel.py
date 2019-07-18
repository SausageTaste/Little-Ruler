import os
import sys
import json
from typing import List, Dict, Callable, Tuple

import level.datastruct.level_builder as lvb


cmdFunc_t = Callable[[List[str]], int]


def pathSplit(path: str) -> Tuple[str, str, str]:
    forderPath, fileNameExt = os.path.split(path)
    fileName, ext = os.path.splitext(fileNameExt)
    return forderPath, fileName, ext


CMDNAME_COMPILE = "compile"
def cmdCompile(args: List[str]) -> int:
    for filePath in args[2:]:
        print("="*20)
        print("Start compiling: " + filePath)
        folpath, filename, fileext = pathSplit(filePath)
        level = lvb.LevelBuilder(filename)
        with open(filePath, "r", encoding="utf8") as file:
            data = json.load(file)
            level.setJson(data)
        savedPath: str = lvb.saveLevelBinary(level, folpath + "/")
        print("Compile done: " + savedPath)

    return 0


CMD_MAP: Dict[str, cmdFunc_t] = {
    CMDNAME_COMPILE : cmdCompile,
}


def printCommands() -> None:
    pass


def main(args: List[str]) -> int:
    print("Arguments : {}".format(args))

    if len(args) < 2:
        printCommands()
        return 0

    commandName: str = args[1]
    if commandName not in CMD_MAP.keys():
        print("Invalid command: " + commandName)
        return -1
    else:
        return CMD_MAP[commandName](args)


if __name__ == '__main__':
    exit(main(sys.argv))
