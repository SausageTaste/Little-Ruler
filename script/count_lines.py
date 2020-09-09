import os
from typing import Tuple, List, Generator


def walk_source_dir():
    target_dirs = [
        "./../engine",
        "./../LittleRulerAndroid/app/src/main",
    ]

    for target_dir in target_dirs:
        for folder_path, folders, files in os.walk(target_dir):
            rest_path, folder_name = os.path.split(folder_path)

            if folder_path.count("out"):
                continue
            if folder_name.count('.'):
                continue

            yield os.path.normpath(folder_path), folders, files

def get_source_file_pathes() -> Generator[str, None, None]:
    for folder_path, folders, files in walk_source_dir():
        for file_name in files:
            if file_name.endswith(".cpp"):
                yield os.path.join(folder_path, file_name)
            elif file_name.endswith(".h"):
                yield os.path.join(folder_path, file_name)
            elif file_name.endswith(".txt"):
                yield os.path.join(folder_path, file_name)
            elif file_name.endswith(".java"):
                yield os.path.join(folder_path, file_name)

def count_lines(path: str) -> int:
    with open(path, "r", encoding="utf8", errors="replace") as file:
        return len(file.readlines())


def main():
    accum_lines = 0
    database: List[Tuple[int, str]] = []

    for file_path in get_source_file_pathes():
        lines = count_lines(file_path)
        accum_lines += lines
        database.append((lines, file_path))

    database.sort()

    for lines, file_path in database:
        print("{:>8}   {}".format(lines, file_path))
    print("{:>8}   total".format(accum_lines))
    print("{:>8}   file path".format("lines"))

    input("Press any key to exit...")


if __name__ == '__main__':
    main()
