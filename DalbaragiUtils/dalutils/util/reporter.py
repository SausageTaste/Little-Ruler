import enum
from typing import List, Tuple


class ErrorLevel(enum.Enum):
    INFO = 2
    WARN = 3
    ERRO = 4


class ReportFlags:
    def __init__(self):
        self.__flagEmptyNameWarn = False

    @property
    def m_flagEmptyNameWarn(self):
        return self.__flagEmptyNameWarn
    @m_flagEmptyNameWarn.setter
    def m_flagEmptyNameWarn(self, x: bool):
        self.__flagEmptyNameWarn = bool(x)


class ErrorNote:
    def __init__(self, msg: str = "", level = ErrorLevel.ERRO):
        self.__errLevel = ErrorLevel(level)
        self.__msg = str(msg)

    @property
    def m_errorLevel(self):
        return self.__errLevel
    @m_errorLevel.setter
    def m_errorLevel(self, level: ErrorLevel):
        if not isinstance(level, ErrorLevel):
            raise ValueError()
        self.__errLevel = level

    @property
    def m_msg(self):
        return self.__msg
    @m_msg.setter
    def m_msg(self, msg: str):
        self.__msg = str(msg)


class ErrorJournal:
    def __init__(self, title: str):
        self.__title = str(title)
        self.__children: List[ErrorJournal] = []
        self.__notes: List[ErrorNote] = []

    def addChildren(self, child: "ErrorJournal") -> None:
        if not isinstance(child, ErrorJournal):
            raise ValueError()
        self.__children.append(child)

    def addNote(self, note: ErrorNote) -> None:
        if not isinstance(note, ErrorNote):
            raise ValueError()
        self.__notes.append(note)

    # Returned value int is how many notes included.
    def makeReportText(self, indent: int = 0, logLevel: ErrorLevel = ErrorLevel.INFO) -> Tuple[str, int]:
        buffer = ""
        indentMore = indent + 1

        noteCount = 0

        if self.__title:
            buffer += "\t" * indent
            buffer += self.__title + '\n'

        for note in self.__notes:
            if note.m_errorLevel.value >= logLevel.value:
                buffer += "\t" * indentMore
                buffer += "[{}] {}\n".format(note.m_errorLevel.name, note.m_msg)
                noteCount += 1

        for child in self.__children:
            resMsg, resNoteCount = child.makeReportText(indentMore, logLevel)
            if resNoteCount:
                buffer += '\n'
                buffer += resMsg
                noteCount += resNoteCount

        return buffer, noteCount

    def isEmpty(self) -> bool:
        for child in self.__children:
            if not child.isEmpty():
                return False

        if len(self.__notes):
            return False
        else:
            return True

    def hasError(self) -> bool:
        for note in self.__notes:
            if note.m_errorLevel == ErrorLevel.ERRO:
                return True

        for child in self.__children:
            if child.hasError():
                return True

        return False


def test():
    parent = ErrorJournal("parent")

    note = ErrorNote()
    note.m_msg = "FUck this"
    note.m_errorLevel = ErrorLevel.ERRO
    parent.addNote(note)

    note = ErrorNote()
    note.m_msg = "Well good"
    note.m_errorLevel = ErrorLevel.INFO
    parent.addNote(note)

    child = ErrorJournal("child")

    note = ErrorNote()
    note.m_msg = "Well shit"
    note.m_errorLevel = ErrorLevel.WARN
    child.addNote(note)

    parent.addChildren(child)

    child = ErrorJournal("empty")
    parent.addChildren(child)

    msg, noteCount = parent.makeReportText(0, ErrorLevel.INFO)
    print(msg)
    print(noteCount)


if __name__ == '__main__':
    test()
