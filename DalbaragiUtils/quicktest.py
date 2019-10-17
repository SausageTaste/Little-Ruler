import os
import shutil


def main():
    os.system("python .\\demo\\water_n_slope.py")
    os.system("dlbcompile.py --c demo/intermediates/water_n_slope.json")
    shutil.copyfile("./demo/intermediates/water_n_slope.dlb", "./../Resource/asset/map/water_n_slope.dlb")


if __name__ == '__main__':
    main()
