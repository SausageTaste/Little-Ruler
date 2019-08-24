import os
from types import Tuple


def split3(path: str) -> Tuple[str, str, str]:
    folderPath, fileNameExt = os.path.split(path)
    fileName, fileExt = os.path.splitext(fileNameExt)
    return folderPath, fileName, fileExt
