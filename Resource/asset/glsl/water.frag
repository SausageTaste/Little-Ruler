#include <inter_lighting.frag>


// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;

uniform sampler2D u_bansaTex;
uniform sampler2D u_gooljulTex;
uniform sampler2D u_dudvMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_depthMap;

uniform float u_dudvMoveFactor;
uniform float u_waveStrength;
uniform float u_darkestDepthPoint;
uniform float u_reflectivity;
uniform vec3 u_deepColor; // 0.07, 0.07, 0.15


in vec3 vFragPos;
in vec2 vTexCoord;
in vec3 vNormalVec;
in vec4 vFragPosInDlight[3];
in vec4 v_clipSpace;
in vec3 v_toCamera;

out vec4 fColor;


const float NEAR = 0.01;
const float FAR = 100.0;
const float WATER_SAMPLER_OFFSET = 0.003;
const float WATER_SAMPLER_OFFSET_INV = 1.0 - WATER_SAMPLER_OFFSET;


vec3 getWorldPosFromDepth(float depth, vec2 TexCoord) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverse(u_projMat) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(u_viewMat) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec2 getDistortedCoords(void) {
    vec2 distoredTexCoords = texture(u_dudvMap, vec2(vTexCoord.x + u_dudvMoveFactor, vTexCoord.y)).rg * 0.1;
    return vTexCoord + vec2(distoredTexCoords.x, distoredTexCoords.y + u_dudvMoveFactor);
}

float makeDepthFactor1(float waterDepth) {
    float factor = waterDepth / u_darkestDepthPoint;
    return clamp(factor, 0.2, 1.0);
}

float makeDepthFactor(float waterDepth) {
    float factor = log(waterDepth + 1.0) / log(u_darkestDepthPoint + 1.0);
    return clamp(factor, 0.2, 1.0);
}

vec3 makeFragNormal(vec2 distortedCoords) {
    vec4 normalColor = texture(u_normalMap, distortedCoords);
    return normalize(vec3(
        normalColor.r * 2.0 - 1.0,
        normalColor.b * 6.0,
        normalColor.g * 2.0 - 1.0
    ));
}

float makeWaterDepth1(vec2 refractionCoords) {
    float depth = texture(u_depthMap, refractionCoords).r;
    float floorDistance = 2.0 * NEAR * FAR / (FAR + NEAR - (2.0 * depth - 1.0) * (FAR - NEAR));
    depth = gl_FragCoord.z;
    float waterDistance = 2.0 * NEAR * FAR / (FAR + NEAR - (2.0 * depth - 1.0) * (FAR - NEAR));
    return floorDistance - waterDistance;
}

float makeWaterDepth(vec2 refractionCoords) {
    float sampled = texture(u_depthMap, refractionCoords).r;
    vec3 refractionWorldPos = getWorldPosFromDepth(sampled, refractionCoords);
    float floorDistance = distance(uViewPos, refractionWorldPos);
    float waterDistance = distance(uViewPos, vFragPos);

    return floorDistance - waterDistance;
}


vec4 calculateWater(vec3 fragNormal, vec2 distortedCoords) {
    vec2 normalizedDeviceCoord = (v_clipSpace.xy / v_clipSpace.w) / 2.0 + 0.5;
    vec2 bansaCoord = vec2(normalizedDeviceCoord.x, -normalizedDeviceCoord.y);
    vec2 gooljulCoord = vec2(normalizedDeviceCoord.x, normalizedDeviceCoord.y);

    vec2 totalDistortion = (texture(u_dudvMap, distortedCoords).rg * 2.0 - 1.0) * u_waveStrength;// * clamp(makeWaterDepth(gooljulCoord) / 1.0, 0.0, 1.0);
    bansaCoord += totalDistortion;
    gooljulCoord += totalDistortion;

    gooljulCoord = clamp(gooljulCoord, WATER_SAMPLER_OFFSET, WATER_SAMPLER_OFFSET_INV);
    bansaCoord.x = clamp(bansaCoord.x, WATER_SAMPLER_OFFSET, WATER_SAMPLER_OFFSET_INV);
    bansaCoord.y = clamp(bansaCoord.y, -WATER_SAMPLER_OFFSET_INV, -WATER_SAMPLER_OFFSET);

    float waterDepth = makeWaterDepth(gooljulCoord);

    vec4 bansaColor = texture(u_bansaTex, bansaCoord);
    vec4 gooljulColor = texture(u_gooljulTex, gooljulCoord);
    gooljulColor = mix(gooljulColor, vec4(u_deepColor, 1.0), makeDepthFactor(waterDepth));

    vec3 viewVec = normalize(v_toCamera);
    float refractiveFactor = pow(dot(viewVec, fragNormal), u_reflectivity);

    vec4 outColor = mix(bansaColor, gooljulColor, refractiveFactor);
    outColor.a = clamp(waterDepth / 0.1, 0.0, 1.0);

    //outColor = vec4(vec3(waterDepth / 60.0), 1.0);

    return outColor;
}


void main(void) {
    // Needed values
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec2 distoredTexCoords = getDistortedCoords();
    vec3 fragNormal = makeFragNormal(distoredTexCoords);

    // Lighting
    int i;
    vec3 lightedColor = uBaseAmbient;
    for (i = 0; i < uDlightCount; i++) {
        vec4 fragPosInLight = vec4(vec3(vFragPosInDlight[i]) + fragNormal * u_waveStrength, 1.0);
        fragPosInLight.y = vFragPosInDlight[i].y;
        lightedColor += getDlightFactor(i, viewDir, fragNormal, fragPosInLight) * uDlightColors[i];
    }
    for (i = 0; i < uPlightCount; i++) {
        lightedColor += getLightFactor_point(i, viewDir, fragNormal, vFragPos) * uPlightColors[i];
    }

    // Water
    vec4 waterImage = calculateWater(fragNormal, distoredTexCoords);

    // Final color
    fColor = 0.5 * waterImage * (vec4(lightedColor, 1.0) + 1.0);
    fColor = calcFogMixedColor(fColor, vFragPos);
    //fColor += vec4(0.05, 0.05, 0.1, 0.0);
    //fColor = waterImage;
}