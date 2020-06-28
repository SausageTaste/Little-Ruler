#include <m_geometry.glsl>


uniform vec3 u_envmapPos;

uniform samplerCube u_irradianceMap;
uniform samplerCube u_prefilterMap;
uniform sampler2D u_brdfLUT;
uniform bool u_hasEnvmap;

uniform int u_numPlanes;
uniform vec4 u_planes[16];


const uint MAX_MIP_LVL = 3u;
const float MAX_ENV_SAMPLE_DIST = 200.0;


// 999999.99 on not intersecting
float intersect_ray_plane(vec3 point, vec3 direc, vec4 plane) {
    vec3  endPoint          = point + direc;
    float dist_point        = dot(plane, vec4(point, 1.0));
    float dist_endPoint     = dot(plane, vec4(endPoint, 1.0));
    float abs_dist_point    = abs(dist_point);
    float abs_dist_endPoint = abs(dist_endPoint);
    float rayLength         = length(direc);

    if ( dist_point * dist_endPoint < 0.0 ) {
        float dist_along = (abs_dist_point * rayLength) / (abs_dist_point + abs_dist_endPoint);
        return dist_along;
    }
    else if ( abs_dist_endPoint < abs_dist_point ) {
        float dist_along = (abs_dist_point * rayLength) / (abs_dist_point - abs_dist_endPoint);
        return dist_along;
    }
    else {
        return MAX_ENV_SAMPLE_DIST + 1.0;
    }
}

vec3 intersect_seg_envVolume(vec3 point, vec3 direc) {
    vec3 col_point = vec3(0.0, 0.0, 0.0);
    float col_dist = MAX_ENV_SAMPLE_DIST;
    vec3 direc_n = normalize(direc);

    int total = 0;
    int inter = 0;

    for ( int i = 0; i < u_numPlanes; ++i ) {
        float dist = intersect_ray_plane(point, direc_n, u_planes[i]);
       
        if ( dist >= 0.01 && dist < col_dist ) {
            col_dist = dist;
            col_point = point + direc_n * dist;
        }
    }

    return col_point;
}

vec3 calcEnvSampleDirec(vec3 viewPos, vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, fragNormal);

#ifdef DAL_PARALLAX_CORRECT_CUBE_MAP
    if ( u_numPlanes != 0 ) {
        vec3 intersection = intersect_seg_envVolume(fragPos, R);
        return normalize(intersection - u_envmapPos);
    }
#endif

    return R;
}
