import os
import shutil

import dlbcompile as dlb
import demo_mapcreate as mcr


OUTPUT_FOLDER: str = "./outputs/"
ASSET_FOLDER: str = "./../Resource/asset/map/"


def copyOutputf() -> None:
    for dlbFile in os.listdir(OUTPUT_FOLDER):
        shutil.copy(OUTPUT_FOLDER + dlbFile, ASSET_FOLDER + dlbFile)


def main():
    mcr.main()
    dlb.main(["", "intermediates/water_bowl.json"])
    copyOutputf()


if __name__ == '__main__':
    main()
