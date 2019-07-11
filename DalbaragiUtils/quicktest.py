import os
import shutil

import dlbcompile as dlb
from demo import water_bowl, vast_city_like


INTERM_FOLDER: str = "./intermediates/"
OUTPUT_FOLDER: str = "./outputs/"
ASSET_FOLDER: str = "./../Resource/asset/map/"


def copyOutputf() -> None:
    for dlbFile in os.listdir(OUTPUT_FOLDER):
        shutil.copy(OUTPUT_FOLDER + dlbFile, ASSET_FOLDER + dlbFile)

def genIntermediates():
    for i in os.listdir(INTERM_FOLDER):
        yield i


def main():
    water_bowl.main()
    vast_city_like.main()

    args = [""] + [INTERM_FOLDER + i for i in genIntermediates()]
    dlb.main(args)
    copyOutputf()


if __name__ == '__main__':
    main()
