import os
from typing import Tuple, List, Generator

import local_tools.path_tools as ptt


PRINT_EXCLUDED_ONES = False

FOLDERS_TO_EXCLUDE = [
    ".cxx",
    ".gradle",
    "gradle",
    "app/build",
    "app/src/test",
    "app/src/androidTest",
    "app/src/main/res",
    "out",
    "license"
]

FILES_TO_EXCLUDE = [
    "vulkan_wrapper",
]

TARGET_DIRS = [
    os.path.join(ptt.find_repo_root_path(), "engine"),
    os.path.join(ptt.find_repo_root_path(), "LittleRulerAndroid"),
    os.path.join(ptt.find_repo_root_path(), "Resource/asset/glsl"),
]

TARGET_EXTENSIONS = [
    "cpp",
    "h",
    "txt",
    "java",
    "vert",
    "frag",
]


def is_folder_to_be_excluded(folder_path: str) -> bool:
    for x in FOLDERS_TO_EXCLUDE:
        if os.path.normpath(x) in folder_path:
            return True

    return False

def is_file_to_be_excluded(file_name: str) -> bool:
    for x in FILES_TO_EXCLUDE:
        if x in file_name:
            return True

    return False

def is_ext_to_be_included(file_name: str) -> bool:
    for x in TARGET_EXTENSIONS:
        if file_name.endswith(x):
            return True

    return False

def walk_source_dir():
    for target_dir in TARGET_DIRS:
        target_dir = os.path.normpath(target_dir)

        for folder_path, folders, files in os.walk(target_dir):
            folder_path = os.path.normpath(folder_path)
            rest_path, folder_name = os.path.split(folder_path)

            if is_folder_to_be_excluded(folder_path):
                if PRINT_EXCLUDED_ONES:
                    print("Excluded folder:", folder_path)
                continue

            yield folder_path, folders, files

def get_source_file_pathes() -> Generator[str, None, None]:
    for folder_path, folders, files in walk_source_dir():
        for file_name in files:
            if is_file_to_be_excluded(file_name):
                if PRINT_EXCLUDED_ONES:
                    print("Excluded file:", os.path.join(folder_path, file_name))
                continue

            if is_ext_to_be_included(file_name):
                yield os.path.join(folder_path, file_name)

def count_lines(path: str) -> int:
    with open(path, "r", encoding="utf8", errors="replace") as file:
        return len(file.readlines())


def main():
    database: List[Tuple[int, str]] = []

    for file_path in get_source_file_pathes():
        lines = count_lines(file_path)
        database.append((lines, file_path))

    database.sort(key=lambda data_tuple: data_tuple[1])

    accum_lines = 0
    for lines, file_path in database:
        accum_lines += lines
        print("{:>8} | {:>8} | {}".format(lines, accum_lines, file_path))
    print("---------|----------|---------------------------------------------------")
    print("{:>8} | {:>8} | file path".format("lines", "accum"))

    #input("Press any key to exit...")


if __name__ == '__main__':
    main()
