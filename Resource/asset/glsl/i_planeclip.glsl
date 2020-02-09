uniform vec4 u_clipPlane;


float calcClipDist(vec3 worldPos) {
    return dot(vec4(worldPos, 1.0), u_clipPlane);
}
