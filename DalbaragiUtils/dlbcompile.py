import sys
from typing import List, Optional

import level.build_info as bif


class CompileWorkReciepe:
    def __init__(self):
        self.m_fileNames:List[str] = []


def getWorkProperties() -> Optional[CompileWorkReciepe]:
    work = CompileWorkReciepe()

    for arg in sys.argv[1:]:
        if arg.endswith(".json"):
            work.m_fileNames.append(arg)
        else:
            print("Invalid parameter: " + arg)
            return None

    return work


def main():
    work = getWorkProperties()
    if work is None:
        print("\nTerminate due to error during parsing args.")
        return

    what = bif.BuildInfo_ModelImported()


if __name__ == '__main__':
    main()
