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

    // Lighting
    vec3 lightedColor = uBaseAmbient;

    for ( int i = 0; i < uDlightCount; ++i ) {
        lightedColor += getDlightColor(i, viewDir, fragNormal, v_fragPos, vFragPosInDlight[i]);
    }
    lightedColor += getTotalPlightColors(viewDir, fragNormal, v_fragPos);
    for ( int i = 0; i < u_slightCount; ++i ) {
        lightedColor += getSlightColor(i, viewDir, fragNormal, v_fragPos, v_fragPosInSlight[i]);
    }
    lightedColor += getEnvColor(v_fragPos, fragNormal);

    // Final
    fColor.rgb = texColor.rgb * lightedColor;
    fColor.rgb = calcFogMixedColor(fColor.rgb, v_fragPos);
    fColor.a = texColor.a;
}
