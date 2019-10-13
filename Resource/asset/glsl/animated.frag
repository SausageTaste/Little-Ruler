#include <inter_lighting.frag>


// Interf - PlaneClip
uniform bool u_doClip;

uniform sampler2D u_diffuseMap;  // TEX 0


in vec3 v_fragPos;
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

    vec3 viewDir = normalize(uViewPos - v_fragPos);
    vec3 fragNormal = normalize(vNormalVec);
    vec4 texColor = texture(u_diffuseMap, vTexCoord);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, texColor.rgb, u_metallic);
    vec3 pbrL = uBaseAmbient * texColor.rgb;
    for ( int i = 0; i < uPlightCount; ++i ) {
        vec3 radiance = calcPlightRadiance(i, v_fragPos);
        vec3 L = normalize(u_plights[i].m_pos - v_fragPos);
        pbrL += lightingIntegrateStep(fragNormal, viewDir, F0, L, texColor.rgb) * radiance;
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        vec3 radiance = calcSlightRadiance(i, v_fragPos);
        vec3 L = normalize(u_slights[i].m_pos - v_fragPos);
        bool isInShadow = isPointInSlightShadow(i, v_fragPosInSlight[i]);
        pbrL += isInShadow ? vec3(0.0) : lightingIntegrateStep(fragNormal, viewDir, F0, L, texColor.rgb) * radiance;
    }
    for ( int i = 0; i < uDlightCount; ++i ) {
        vec3 radiance = u_dlights[i].m_color;
        vec3 L = normalize(-u_dlights[i].m_direc);
        bool isInShadow = isPointInDlightShadow(i, vFragPosInDlight[i]);
        pbrL += isInShadow ? vec3(0.0) : lightingIntegrateStep(fragNormal, viewDir, F0, L, texColor.rgb) * radiance;
    }
    fColor.rgb = pbrL;
    fColor.a = texColor.a;

    fColor.rgb = calcFogMixedColor(fColor.rgb, v_fragPos);
}
