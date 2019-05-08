import os
import shutil

import dlbcompile as dlb
import demo_mapcreate as mcr


OUTPUT_FOLDER: str = "./outputs/"
ASSET_FOLDER: str = "./../Resources/assets/maps/"


def copyOutputf() -> None:
    for dlbFile in os.listdir(OUTPUT_FOLDER):
        shutil.copy(OUTPUT_FOLDER + dlbFile, ASSET_FOLDER + dlbFile)


def main():
    mcr.main()
    dlb.main(["", "intermediates/test_level.json"])
    copyOutputf()


if __name__ == '__main__':
    main()
