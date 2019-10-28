#include <inter_lighting.frag>
#include <inter_lightmaps.frag>


// Interf - PlaneClip
uniform bool u_doClip;


in vec3 vFragPos;
in vec2 vTexCoord;
in vec3 vNormalVec;
in vec4 vFragPosInDlight[3];
in vec4 v_fragPosInSlight[3];
#ifdef GL_ES
in float v_clipDistance;
#endif

out vec4 fColor;


void main(void) {
#ifdef GL_ES
    if (u_doClip && v_clipDistance < 0.0) {
        discard;
    }
#endif

    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 fragNormal = normalize(vNormalVec);
    vec4 texColor = texture(u_diffuseMap, vTexCoord);
    float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, vTexCoord).r : u_roughness;
    float metallic = u_hasMetallicMap ? texture(u_metallicMap, vTexCoord).r : u_metallic;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, texColor.rgb, u_metallic);
    vec3 pbrL = uBaseAmbient * texColor.rgb;
    for ( int i = 0; i < uPlightCount; ++i ) {
        vec3 radiance = calcPlightRadiance(i, vFragPos);
        vec3 L = normalize(u_plights[i].m_pos - vFragPos);
        pbrL += lightingIntegrateStep(fragNormal, viewDir, F0, L, texColor.rgb, roughness, metallic) * radiance;
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        vec3 radiance = calcSlightRadiance(i, vFragPos);
        vec3 L = normalize(u_slights[i].m_pos - vFragPos);
        bool isInShadow = isPointInSlightShadow(i, v_fragPosInSlight[i]);
        pbrL += isInShadow ? vec3(0.0) : lightingIntegrateStep(fragNormal, viewDir, F0, L, texColor.rgb, roughness, metallic) * radiance;
    }
    for ( int i = 0; i < uDlightCount; ++i ) {
        vec3 radiance = u_dlights[i].m_color;
        vec3 L = normalize(-u_dlights[i].m_direc);
        bool isInShadow = isPointInDlightShadow(i, vFragPosInDlight[i]);
        pbrL += isInShadow ? vec3(0.0) : lightingIntegrateStep(fragNormal, viewDir, F0, L, texColor.rgb, roughness, metallic) * radiance;
    }
    fColor.rgb = pbrL;
    fColor.a = texColor.a;

    fColor.rgb = calcFogMixedColor(fColor.rgb, vFragPos);
}
