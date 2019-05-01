import math



class Vec3:
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0):
        self.__x: float = x
        self.__y: float = y
        self.__z: float = z

    def getLength(self) -> float:
        return math.sqrt(self.getLengthSquare())

    def getLengthSquare(self) -> float:
        return self.__x**2 + self.__y**2 + self.__z**2

    def normalize(self) -> None:
        length = self.getLength()
        self.__x /= length
        self.__y /= length
        self.__z /= length

    def overrideFromJson(self, data: dict) -> None:
        self.__x = data[0]
        self.__y = data[1]
        self.__z = data[2]

    def getJson(self) -> list:
        return [self.__x, self.__y, self.__z]

    @property
    def x(self):
        return self.__x
    @x.setter
    def x(self, v: float):
        self.__x = float(v)

    @property
    def y(self):
        return self.__y
    @y.setter
    def y(self, v: float):
        self.__y = float(v)

    @property
    def z(self):
        return self.__x
    @z.setter
    def z(self, v: float):
        self.__z = float(v)


class Vec4(Vec3):
    def __init__(self, x:float=0.0, y:float=0.0, z:float=0.0, w:float=0.0):
        super().__init__(x, y, z)
        self.__w: float = float(w)

    def getLength(self) -> float:
        return math.sqrt(self.getLengthSquare())

    def getLengthSquare(self) -> float:
        return self.x ** 2 + self.y ** 2 + self.z ** 2 + self.__w ** 2

    def normalize(self) -> None:
        length = self.getLength()
        self.x /= length
        self.y /= length
        self.z /= length
        self.__w /= length

    def overrideFromJson(self, data: dict) -> None:
        self.x = data[0]
        self.y = data[1]
        self.z = data[2]
        self.__w = data[3]

    def getJson(self) -> list:
        a = super().getJson()
        a.append(self.__w)
        return a

    @property
    def w(self):
        return self.__w
    @w.setter
    def w(self, v: float):
        self.__w = float(v)

