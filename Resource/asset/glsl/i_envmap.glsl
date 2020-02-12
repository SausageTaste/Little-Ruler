#include <m_geometry.glsl>


uniform vec3 u_envmapPos;
uniform samplerCube u_envmap;
uniform bool u_hasEnvmap;


vec3 getEnvColor(vec3 viewPos, vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, fragNormal);
    return texture(u_envmap, R).rgb;
}

vec3 getEnvColor_test(vec3 viewPos, vec3 fragPos, vec3 fragNormal, AABB boundingVolume) {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, fragNormal);

    Segment ray;
    ray.m_pos = fragPos;
    ray.m_rel = R * 100.0;

    vec4 intersection = interdect_seg_aabb(ray, boundingVolume);
    vec3 sampleRay = intersection.xyz - u_envmapPos;
    return texture(u_envmap, sampleRay).rgb;
}
