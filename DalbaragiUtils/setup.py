import os
from typing import List, Tuple
from importlib.util import find_spec


class LibInfo:
    def __init__(self, importName: str, installName: str):
        self.__importName = str(importName)
        self.__installName = str(installName)

    def checkAvailable(self) -> bool:
        spam_spec = find_spec(self.__importName)
        return spam_spec is not None

    def install(self) -> None:
        cmd = "pip install " + self.__installName
        res = os.system(cmd)
        if 0 != res:
            print("Failed to install lib: {}, cmd: {}".format(self.__importName, cmd))


def main():
    res = os.system("python -m pip install --upgrade pip")
    if 0 != res:
        print("Failed to update pip.")

    libs: List[LibInfo] = (
        LibInfo("numpy", "numpy"),
        LibInfo("glm", "pyglm"),
    )

    for lib in libs:
        if not lib.checkAvailable():
            lib.install()

    input("All done successfully!")


if __name__ == '__main__':
    main()
