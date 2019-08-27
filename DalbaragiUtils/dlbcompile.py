import os
import sys
import json
from typing import List

import dalutils.map.mapbuilder as mbu
import dalutils.util.exportfile as exf


def printHelp():
    text = """
Usage : dlbcompile.py <options> <json source files>
Options are
  --compile, --c
    compile json files into dlb files.
    ex) dlbcompile.py --compile "my_map.json" "subfolder/my_second.json"
"""
    print(text)


class WorkList:
    def __init__(self):
        self.__toCompile: List[str] = []

    @property
    def m_toCompile(self) -> List[str]:
        return self.__toCompile


class CmdArgParser:
    def __init__(self):
        self.__cmdFuncMap = {
            "compile" : self.__cmdType_compile,
            "c"       : self.__cmdType_compile,
        }
        self.__result = WorkList()

    @property
    def m_workList(self) -> WorkList:
        return self.__result

    def start(self, args: List[str]) -> bool:
        if not len(args):
            printHelp()
            return False

        currentFunc = self.__cmdType_null

        for arg in args:
            if arg.startswith("--"):
                currentFunc = self.__dispatchCmdType(arg)
            else:
                currentFunc(arg)

        return True

    def __dispatchCmdType(self, cmd: str):
        cmd = cmd.strip("--")
        try:
            found = self.__cmdFuncMap[cmd]
        except KeyError:
            raise ValueError("Invalid command: " + cmd)
        else:
            return found

    def __cmdType_null(self, arg: str) -> None:
        raise ValueError("Need command type specified.")

    def __cmdType_compile(self, arg: str) -> None:
        self.__result.m_toCompile.append(arg)


def workCompile(path: str):
    with open(str(path), "r", encoding="utf8") as file:
        data = json.load(file)

    mapData = mbu.MapChunkBuilder()
    mapData.setJson(data)

    folderFileName, fileExt = os.path.splitext(path)
    binData = mapData.getBinary()
    zippedData = exf.compressWithSizeInt32(binData)
    with open(folderFileName + ".dlb", "wb") as file:
        file.write(zippedData)


def main(args: list):
    print("dlbcompile started with: {}".format(args))

    parser = CmdArgParser()
    if not parser.start(args[1:]):
        return

    for x in parser.m_workList.m_toCompile:
        workCompile(x)


if __name__ == '__main__':
    main(sys.argv)
