import zlib


def main():
    content = "Hello world mates!".encode(encoding="utf8")
    compressed = zlib.compress(content, level=zlib.Z_BEST_COMPRESSION)
    print(len(content))
    print(len(compressed))

    decom = zlib.decompress(compressed)
    with open("test.tst", "wb") as file:
        file.write(compressed)


if __name__ == '__main__':
    main()
