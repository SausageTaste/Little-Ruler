uniform highp vec3 uViewPos;

uniform vec3 uBaseAmbient;
uniform highp int uDlightCount;
uniform int uPlightCount;

uniform float uShininess;
uniform float uSpecularStrength;
uniform float u_fogMaxPointInvSqr;
uniform vec3 u_fogColor;

uniform vec3  uPlightPoses[3];
uniform vec3  uPlightColors[3];
uniform float uPlightMaxDists[3];

uniform vec3      uDlightDirecs[3];
uniform vec3      uDlightColors[3];
uniform sampler2D uDlightDepthMap[3];  // TEX 1, 2, 3

uniform samplerCube u_environmentMap;


float _sampleDlightDepth(int index, vec2 coord) {

#ifdef GL_ES
    switch (index){
        case 0:
            return texture(uDlightDepthMap[0], coord).r;
        case 1:
            return texture(uDlightDepthMap[1], coord).r;
        case 2:
            return texture(uDlightDepthMap[2], coord).r;
        default:
            return 1.0;
    }
#else
    return texture(uDlightDepthMap[index], coord).r;
#endif

}

float _getShadowFactor_directional(int index, vec4 fragPosInDlight) {
    vec3 projCoords = fragPosInDlight.xyz / fragPosInDlight.w;
    if (projCoords.z > 1.0) return 1.0;

    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x > 1.0 || projCoords.x < 0.0) return 1.0;
    if (projCoords.y > 1.0 || projCoords.y < 0.0) return 1.0;

    float closestDepth = _sampleDlightDepth(index, projCoords.xy);
    float currentDepth = projCoords.z;

    //float bias = max(0.05 * (1.0 - dot(vNormalVec, -uDlightDirs[index])), 0.005);
    //float bias = 0.002;
    float bias = 0.0;
    float shadow = currentDepth - bias > closestDepth  ? 0.0 : 1.0;

    return shadow;
}

float _distanceDecreaser(float dist) {
    const float k_startDecrease = 15.0;
    const float k_endDecrea = 80.0;

    if (dist < k_startDecrease)
        return 1.0;
    else if (dist > k_endDecrea)
        return 0.0;
    else
        return (dist - k_endDecrea) / (k_startDecrease - k_endDecrea);
}

float _getLightFactor_directional(int index, vec3 viewDir, vec3 fragNormal) {
    vec3 lightLocDir = normalize(-uDlightDirecs[index]);

    float diff = max(dot(fragNormal, lightLocDir), 0.0);

    // Calculate specular lighting.
    if (uShininess <= 0.0) {
        return diff;
    }
    else {
        vec3 reflectDir = reflect(-lightLocDir, fragNormal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
        float specular = max(uSpecularStrength * spec, 0.0);

        return diff + specular;
    }
}

float getDlightFactor(int index, vec3 viewDir, vec3 fragNormal, vec4 fragPosInDlight) {
    return _getLightFactor_directional(index, viewDir, fragNormal) * _getShadowFactor_directional(index, fragPosInDlight);
}


float getLightFactor_point(int index, vec3 viewDir, vec3 fragNormal, vec3 fragPos) {
    float dist = distance(uViewPos, uPlightPoses[index]);

    vec3 lightDir = normalize(uPlightPoses[index] - fragPos);
    float diff = max(dot(lightDir, fragNormal), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, fragNormal);
    float spec = 0.0;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(fragNormal, halfwayDir), 0.0), uShininess);

    float specular = max(uSpecularStrength * spec, 0.0);

    return (diff + specular) * _distanceDecreaser(dist);
}


float _calcHeightFogFactor(vec3 fragPos) {
    float fragHeight = fragPos.y;
    const float lambda = 10.0;
    const float maxFactor = 0.1;
    float factor = -maxFactor * (fragHeight / lambda - 1.0);
    return clamp(factor, 0.0, maxFactor);
}

float _calcDistanceFogFactor(vec3 fragPos) {
    float fragDistance = distance(fragPos, uViewPos);
    float factor = fragDistance * u_fogMaxPointInvSqr * fragDistance;
    return clamp(factor, 0.0, 1.0);
}

float _calcFogFactor(vec3 fragPos) {
    float factor = _calcDistanceFogFactor(fragPos);
    factor += _calcHeightFogFactor(fragPos);
    return clamp(factor, 0.0, 1.0);
}

vec4 calcFogMixedColor(vec4 color, vec3 fragPos) {
    float factor = _calcFogFactor(fragPos);
    vec3 mixedColor = mix(color.xyz, u_fogColor, factor);
    return vec4(mixedColor, color.a);
}


vec3 getEnvColor(vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - uViewPos);
    vec3 R = reflect(I, fragNormal);
    return texture(u_environmentMap, R).rgb;
}

float getEnvFactor(void){
    float factor = (uSpecularStrength) * 0.3;
    return clamp(factor, 0.0, 1.0);
}
