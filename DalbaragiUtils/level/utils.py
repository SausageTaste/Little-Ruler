

def isValidStrID(text: str) -> bool:
    if 0 != text.count(" "): return False
    elif 0 != text.count("\n"): return False
    elif 0 != text.count("\t"): return False
    elif text[0].isnumeric(): return False

    return True

def throwIfNotValidStrId(text: str) -> None:
    if not isValidStrID(text): raise ValueError("Invalid for object name: " + text)