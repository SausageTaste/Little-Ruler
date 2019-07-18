import os
import sys
import json
from typing import List, Dict, Callable, Tuple

import level.datastruct.level_builder as lvb


cmdFunc_t = Callable[[List[str]], int]


def printErr(text: str) -> None:
    sys.stdout.flush()
    print(text, file=sys.stderr)
    sys.stderr.flush()

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

        try:
            file = open(filePath, "r", encoding="utf8")
        except FileNotFoundError:
            printErr("[ERROR] input file not found: " + filePath)
            continue

        try:
            data = json.load(file)  # Just let it throw exception which is great hint for user.
        except json.decoder.JSONDecodeError as e:
            printErr("[ERROR] JSONDecodeError: " + str(e))
            continue

        file.close()
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
