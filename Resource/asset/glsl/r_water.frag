#include <i_lighting.glsl>
#include <f_pbr.glsl>


uniform highp vec3 u_viewPos;
uniform highp mat4 u_projMat;
uniform highp mat4 u_viewMat;

uniform       sampler2D u_bansaImg;
uniform       sampler2D u_gooljulImg;
uniform       sampler2D u_dudvMap;
uniform       sampler2D u_normalMap;
uniform highp sampler2D u_depthMap;

uniform float u_dudvMoveFactor;
uniform float u_waveStrength;
uniform float u_darkestDepthPoint;
uniform float u_reflectance;
uniform vec3  u_deepColor;


in       vec3 v_fragPos;
in       vec2 v_texCoord;
in       vec4 v_fragPos_dlight[3];
in       vec4 v_fragPos_slight[3];
in highp vec4 v_clipSpace;
in       vec3 v_toCamera;

out vec4 f_color;


const float NEAR = 0.01;
const float FAR = 100.0;
const float WATER_SAMPLER_OFFSET = 0.003;
const float WATER_SAMPLER_OFFSET_INV = 1.0 - WATER_SAMPLER_OFFSET;


vec2 getDistortedCoords(void) {
    vec2 distoredTexCoords = texture(u_dudvMap, vec2(v_texCoord.x + u_dudvMoveFactor, v_texCoord.y)).rg * 0.1;
    return v_texCoord + vec2(distoredTexCoords.x, distoredTexCoords.y + u_dudvMoveFactor);
}

vec3 makeFragNormal(vec2 distortedCoords) {
    vec4 normalColor = texture(u_normalMap, distortedCoords);
    return normalize(vec3(
        normalColor.r * 2.0 - 1.0,
        normalColor.b * 6.0,
        normalColor.g * 2.0 - 1.0
    ));
}


float makeDepthFactor_linear(float waterDepth) {
    float factor = waterDepth / u_darkestDepthPoint * 1.5;
    return clamp(factor, 0.2, 1.0);
}

float makeDepthFactor_exponent(float waterDepth) {
    float factor = log(waterDepth + 1.0) / log(u_darkestDepthPoint + 1.0);
    return clamp(factor, 0.2, 1.0);
}


vec3 getWorldPosFromDepth(float depth, vec2 TexCoord) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverse(u_projMat) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(u_viewMat) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

float makeWaterDepth_actualDistance(vec2 refractionCoords) {
    float sampled = texture(u_depthMap, refractionCoords).r;
    vec3 refractionWorldPos = getWorldPosFromDepth(sampled, refractionCoords);
    float floorDistance = distance(u_viewPos, refractionWorldPos);
    float waterDistance = distance(u_viewPos, v_fragPos);

    return floorDistance - waterDistance;
}

float makeWaterDepth_planeDistance(vec2 refractionCoords) {
    highp float depth = texture(u_depthMap, refractionCoords).r;
    float floorDistance = 2.0 * NEAR * FAR / (FAR + NEAR - (2.0 * depth - 1.0) * (FAR - NEAR));
    depth = gl_FragCoord.z;
    float waterDistance = 2.0 * NEAR * FAR / (FAR + NEAR - (2.0 * depth - 1.0) * (FAR - NEAR));
    return floorDistance - waterDistance;
}


vec4 calculateWater(vec3 fragNormal, vec2 distortedCoords) {
    vec2 normalizedDeviceCoord = (v_clipSpace.xy / v_clipSpace.w) / 2.0 + 0.5;
    vec2 bansaCoord = vec2(normalizedDeviceCoord.x, -normalizedDeviceCoord.y);
    vec2 gooljulCoord = vec2(normalizedDeviceCoord.x, normalizedDeviceCoord.y);

#ifdef GL_ES
    float waterDepth = makeWaterDepth_planeDistance(gooljulCoord);
#else
    float waterDepth = makeWaterDepth_actualDistance(gooljulCoord);
#endif

    vec2 totalDistortion = (texture(u_dudvMap, distortedCoords).rg * 2.0 - 1.0) * u_waveStrength * clamp(waterDepth / 15.0, 0.0, 1.0);
    bansaCoord += totalDistortion;
    gooljulCoord += totalDistortion;

    gooljulCoord = clamp(gooljulCoord, WATER_SAMPLER_OFFSET, WATER_SAMPLER_OFFSET_INV);
    bansaCoord.x = clamp(bansaCoord.x, WATER_SAMPLER_OFFSET, WATER_SAMPLER_OFFSET_INV);
    bansaCoord.y = clamp(bansaCoord.y, -WATER_SAMPLER_OFFSET_INV, -WATER_SAMPLER_OFFSET);

#ifdef GL_ES
    waterDepth = makeWaterDepth_planeDistance(gooljulCoord);
#else
    waterDepth = makeWaterDepth_actualDistance(gooljulCoord);
#endif

    vec4 bansaColor = texture(u_bansaImg, bansaCoord);
    vec4 gooljulColor = texture(u_gooljulImg, gooljulCoord);
#ifdef GL_ES
    float depthFactor = makeDepthFactor_linear(waterDepth);
#else
    float depthFactor = makeDepthFactor_exponent(waterDepth);
#endif
    gooljulColor = mix(gooljulColor, vec4(u_deepColor, 1.0), depthFactor);

    vec3 viewVec = normalize(v_toCamera);
    float refractiveFactor = pow(max(dot(viewVec, fragNormal), 0.0), u_reflectance);

    vec4 outColor = mix(bansaColor, gooljulColor, refractiveFactor);
    outColor.a = clamp(waterDepth / 0.5, 0.0, 1.0);

    return outColor;
}


void main(void) {
    const float SPECULAR_INTENSITY = 100.0;

    // Needed values
    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec2 distoredTexCoords = getDistortedCoords();
    vec3 fragNormal = makeFragNormal(distoredTexCoords);

    // Water
    vec4 waterImage = calculateWater(fragNormal, distoredTexCoords);
    vec4 texColor = waterImage;

    // Lighting
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, texColor.rgb, u_metallic);
    vec3 pbrL = u_baseAmbient * texColor.rgb;
    for ( int i = 0; i < u_plightCount; ++i ) {
        vec3 radiance = calcRadiance_plight(i, v_fragPos);
        vec3 L        = calcToLight_plight(i, v_fragPos);

        pbrL += integratePBR(fragNormal, viewDir, F0, L, texColor.rgb, u_roughness, u_metallic, SPECULAR_INTENSITY) * radiance;
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        vec3 radiance   = calcRadiance_slight(i, v_fragPos);
        vec3 L          = calcToLight_slight(i, v_fragPos);
        bool isInShadow = isInShadow_slight(i, v_fragPos_slight[i]);

        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, texColor.rgb, u_roughness, u_metallic, SPECULAR_INTENSITY) * radiance;
    }
    for ( int i = 0; i < u_dlightCount; ++i ) {
        vec3 radiance   = calcRadiance_dlight(i);
        vec3 L          = calcToLight_dlight(i);
        bool isInShadow = isInShadow_dlight(i, v_fragPos_dlight[i]);

        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, texColor.rgb, u_roughness, u_metallic, SPECULAR_INTENSITY) * radiance;
    }

    // Final color
    f_color.rgb = mix(waterImage.rgb, pbrL, 0.5);
    f_color.a = 1.0;
}
