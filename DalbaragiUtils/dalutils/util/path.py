import os
from typing import Tuple, List, Generator


def split3(path: str) -> Tuple[str, str, str]:
    folderPath, fileNameExt = os.path.split(path)
    fileName, fileExt = os.path.splitext(fileNameExt)
    return folderPath, fileName, fileExt


def splitAll(path: str) -> List[str]:
    return path.replace('\\', '/').split('/')


def stepInsidePath(path: str) -> Generator[str, None, None]:
    pathList = splitAll(path)
    if len(pathList) <= 1:
        yield path
        return

    pathAccum = ""
    if pathList[0].endswith(':'):
        pathAccum += pathList[0]
        pathList = pathList[1:]

    for fol in pathList:
        pathAccum += fol + '/'
        yield pathAccum[:-1]


# File name shouldn't be included in path.
def createFolderAlong(path: str) -> int:
    createdCounter = 0

    for folPath in stepInsidePath(path):
        if os.path.isfile(folPath):
            raise FileExistsError("'{}' exists as as file so can't create folder of the name.".format(os.path.abspath(folPath)))
        elif os.path.isdir(folPath):
            continue
        else:
            os.mkdir(folPath)
            createdCounter += 1

    return createdCounter
