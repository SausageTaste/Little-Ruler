import math
from typing import Tuple

import glm


def calcTriangleArea(p1: glm.vec3, p2: glm.vec3, p3: glm.vec3):
    a = glm.length(p2 - p1)
    b = glm.length(p3 - p2)
    c = glm.length(p1 - p3)

    s = (a + b + c) * 0.5
    return math.sqrt(s * (s - a) * (s - b) * (s - c))

def calcTriangleNormal(p1: glm.vec3, p2: glm.vec3, p3: glm.vec3):
    edge1 = p3 - p2
    edge2 = p1 - p2
    return glm.normalize(glm.cross(edge1, edge2))

def _angleAxis(radian: float, selector: Tuple[float, float, float]):
    result = glm.quat()

    a = float(radian)
    s = math.sin(a * 0.5)

    result.w = math.cos(a * 0.5)
    result.x = selector[0] * s
    result.y = selector[1] * s
    result.z = selector[2] * s

    return result

def _normalizeQuat(q: glm.quat):
    d = math.sqrt(q.w**2 + q.x**2 + q.y**2 + q.z**2)
    return glm.quat(q.w / d, q.x / d, q.y / d, q.z / d)

def rotateQuat(q: glm.quat, degree: float, selector: Tuple[float, float, float]) -> glm.quat:
    rotator = _angleAxis(glm.radians(degree), selector)
    rotated: glm.quat = rotator * q
    normalized = _normalizeQuat(rotated)
    return normalized
