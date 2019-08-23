import os
from typing import List
from importlib.util import find_spec


class LibInstaller:
    def __init__(self, importName: str, installName: str):
        self.__importName = str(importName)
        self.__installName = str(installName)

    def tryInstall(self) -> bool:
        if self.__checkAvailable():
            print("Library already exists: {}".format(self.__importName))
            return True
        else:
            return self.__install()

    def getImportName(self) -> str:
        return self.__importName

    def __checkAvailable(self) -> bool:
        spam_spec = find_spec(self.__importName)
        return spam_spec is not None

    def __install(self) -> bool:
        cmd = self.__makeCmdStr()
        exitCode = os.system(cmd)
        if 0 != exitCode:
            print("Failed to install lib: {}, cmd: {}, exit code".format(self.__importName, cmd, exitCode))
            return False
        else:
            return True

    def __makeCmdStr(self) -> str:
        return "pip install " + self.__installName


def main():
    libs: List[LibInstaller] = [
        LibInstaller("numpy", "numpy"),
        LibInstaller("glm", "pyglm"),
    ]

    successCount: int = 0
    failedList: List[str] = []

    for lib in libs:
        result = lib.tryInstall()
        if result:
            successCount += 1
        else:
            failedList.append(lib.getImportName())

    print("=" * 40)
    print("{} out of {} libs are available.".format(successCount, len(libs)))
    if len(failedList):
        print("Failed libs : {}".format(failedList))
    input("Press a key to continue...")


if __name__ == '__main__':
    main()
