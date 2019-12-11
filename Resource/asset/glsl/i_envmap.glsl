#include <m_geometry.glsl>


uniform samplerCube u_envmap;


vec3 getEnvColor(vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - uViewPos);
    vec3 R = reflect(I, fragNormal);
    return texture(u_envmap, R).rgb;
}

vec3 getEnvColor_test(vec3 fragPos, vec3 fragNormal, AABB boundingVolume) {
    vec3 I = normalize(fragPos - uViewPos);
    vec3 R = reflect(I, fragNormal);

    Segment ray;
    ray.m_pos = fragPos;
    ray.m_rel = R * 100.0;

    vec4 result = interdect_seg_aabb(ray, boundingVolume);
    vec3 sampleray = result.xyz - vec3(6.0, 2.0, -5.0);

    return texture(u_envmap, sampleray).rgb;
}
