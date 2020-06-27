#include <m_geometry.glsl>


uniform vec3 u_envmapPos;

uniform samplerCube u_envmap;
uniform samplerCube u_irradianceMap;
uniform samplerCube u_prefilterMap;
uniform sampler2D u_brdfLUT;
uniform bool u_hasEnvmap;

uniform int u_numPlanes;
uniform vec4 u_planes[16];


const uint MAX_MIP_LVL = 4u;


vec3 intersect_seg_envVolume(Segment seg) {
    vec3 colpoint;
    float coldist = 999999999999.99;

    for ( int i = 0; i < u_numPlanes; ++i ) {
        Plane plane;
        plane.m_normal = u_planes[i].xyz;
        plane.m_d = u_planes[i].w;

        vec4 result = intersect_seg_plane(seg, plane);
        if ( 0.0 != result.w ) {
            float dist = distance(result.xyz, seg.m_pos);
            if ( dist < coldist ) {
                coldist = dist;
                colpoint = result.xyz;
            }
        }
    }

    return colpoint;
}

vec3 calcEnvSampleDirec(vec3 viewPos, vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, fragNormal);
    return R;
}

vec3 getEnvColor(vec3 viewPos, vec3 fragPos, vec3 fragNormal) {
    vec3 R = calcEnvSampleDirec(viewPos, fragPos, fragNormal);

    if ( 0 == u_numPlanes ) {
        return texture(u_envmap, R).rgb;
    }
    else {
        Segment ray;
        ray.m_pos = fragPos;
        ray.m_rel = R * 100.0;

        vec3 intersection = intersect_seg_envVolume(ray);
        vec3 sampleRay = intersection - u_envmapPos;
        return texture(u_envmap, sampleRay).rgb;
    }
}
