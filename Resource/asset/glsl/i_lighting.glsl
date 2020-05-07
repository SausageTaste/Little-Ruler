uniform       vec3      u_baseAmbient;

uniform       float     u_roughness;
uniform       float     u_metallic;

uniform highp int       u_dlightCount;
uniform       int       u_plightCount;
uniform highp int       u_slightCount;

uniform       vec3      u_plight_poses[3];
uniform       vec3      u_plight_colors[3];
uniform       float     u_plight_maxDist[3];

uniform       vec3      u_dlight_direcs[3];
uniform       vec3      u_dlight_colors[3];
uniform highp mat4      u_dlight_projViewMat[3];
uniform highp sampler2D u_dlight_shadowmap[3];

uniform       vec3      u_slight_poses[3];
uniform       vec3      u_slight_direcs[3];
uniform       vec3      u_slight_colors[3];
uniform       float     u_slight_maxDists[3];
uniform       float     u_slight_fadeStart[3];
uniform       float     u_slight_fadeEnd[3];
uniform highp mat4      u_slight_projViewMat[3];
uniform       sampler2D u_slight_shadowmap[3];


// General

float _calcDistAttenu(float fragDist, float constant, float linear, float quadratic) {
    return 1.0 / (constant + linear * fragDist + quadratic * (fragDist * fragDist));
}

float _calcHalfDistAttenu(float fragDist, float halfIntensityDist) {
    float quadratic = 1.0 / (halfIntensityDist * halfIntensityDist);
    return _calcDistAttenu(fragDist, 1.0, 0.0, quadratic);
}

float _calcMaxDistFactor(float fragDist, float maxDist) {
    // Light starts to disapear from (maxDist - BLEND_LEN).
    const float BLEND_LEN = 0.5;
    return clamp( (maxDist - fragDist) / BLEND_LEN, 0.0, 1.0 );
}

float _calcTotalAttenu(float fragDist, float maxDist) {
    return _calcMaxDistFactor(fragDist, maxDist) * _calcDistAttenu(fragDist, 1.0, 0.0, 1.0);
}


// Directional

float _sampleDlightShadowmap(int index, vec2 coord) {

#ifdef GL_ES
    switch (index) {

    case 0:
        return texture(u_dlight_shadowmap[0], coord).r;
    case 1:
        return texture(u_dlight_shadowmap[1], coord).r;
    case 2:
        return texture(u_dlight_shadowmap[2], coord).r;
    default:
        return 1.0;

    }
#else
    return texture(u_dlight_shadowmap[index], coord).r;
#endif

}

float _sampleDlightDepth(int index, vec2 coord) {
    if (coord.x > 1.0 || coord.x < 0.0) return 1.0;
    if (coord.y > 1.0 || coord.y < 0.0) return 1.0;
    return _sampleDlightShadowmap(index, coord);
}

bool isInShadow_dlight(int i, vec4 fragPosInDlight) {
    vec3 projCoords = fragPosInDlight.xyz / fragPosInDlight.w;
    if (projCoords.z > 1.0)
    return false;

    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = _sampleDlightDepth(i, projCoords.xy);
    float currentDepth = projCoords.z;

    return currentDepth > closestDepth;
}

vec3 calcRadiance_dlight(int i) {
    return u_dlight_colors[i];
}

vec3 calcToLight_dlight(int i) {
    return -u_dlight_direcs[i];
}


// Point

vec3 calcRadiance_plight(int i, vec3 fragPos) {
    float attenFactor = _calcTotalAttenu(distance(fragPos, u_plight_poses[i]), u_plight_maxDist[i]);
    return u_plight_colors[i] * attenFactor;
}

vec3 calcToLight_plight(int i, vec3 fragPos) {
    return normalize(u_plight_poses[i] - fragPos);
}


// Spot

float _calcSlightAtten(int i, vec3 fragPos) {
    vec3 fragToLight_n = normalize(u_slight_poses[i] - fragPos);
    float theta        = dot(-fragToLight_n, u_slight_direcs[i]);
    float epsilon      = u_slight_fadeStart[i] - u_slight_fadeEnd[i];
    float attenFactor  = _calcTotalAttenu(distance(fragPos, u_slight_poses[i]), u_slight_maxDists[i]);

    return clamp((theta - u_slight_fadeEnd[i]) / epsilon * attenFactor, 0.0, 1.0);
}

float _sampleSlightTexture(int index, vec2 coord) {

#ifdef GL_ES
    switch (index) {

    case 0:
        return texture(u_slight_shadowmap[0], coord).r;
    case 1:
        return texture(u_slight_shadowmap[1], coord).r;
    case 2:
        return texture(u_slight_shadowmap[2], coord).r;
    default:
        return 1.0;

    }
#else
    return texture(u_slight_shadowmap[index], coord).r;
#endif

}

bool isInShadow_slight(int i, vec4 fragPosInSlight) {
    vec3 projCoords = fragPosInSlight.xyz / fragPosInSlight.w;
    if (projCoords.z > 1.0)
    return false;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x > 1.0 || projCoords.x < 0.0 || projCoords.y > 1.0 || projCoords.y < 0.0)
    return false;

    float closestDepth = _sampleSlightTexture(i, projCoords.xy);
    float currentDepth = projCoords.z;

    const float BIAS = 0.0;
    return (currentDepth - BIAS) > closestDepth ? true : false;
}

vec3 calcRadiance_slight(int i, vec3 fragPos) {
    return u_slight_colors[i] * _calcSlightAtten(i, fragPos);
}

vec3 calcToLight_slight(int i, vec3 fragPos) {
    return normalize(u_slight_poses[i] - fragPos);
}
