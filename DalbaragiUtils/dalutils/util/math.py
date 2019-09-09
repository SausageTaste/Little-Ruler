import math

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