import os
from typing import List, Tuple
from importlib.util import find_spec;


class LibInfo:
    def __init__(self, importName, installName):
        self.__importName = ""
        self.__installName = ""

    def checkAvailable(self):
        spam_spec = find_spec(self.__importName)
        return spam_spec is not None

    def install(self):
        os.system("pip install " + self.__installName);


def main():
    os.system("python -m pip install --upgrade pip");

    libs: List[LibInfo] = (
        LibInfo("numpy", "numpy"),
        LibInfo("glm", "pyglm"),
    );

    for lib in libs:
        if not lib.checkAvailable():
            lib.install()

    input("All done successfully!");


if __name__ == '__main__':
    main();
