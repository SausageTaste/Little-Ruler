#include <inter_lighting.frag>


// Interf - PlaneClip
uniform bool u_doClip;

uniform sampler2D u_diffuseMap;  // TEX 0


in vec3 v_fragPos;
in vec2 vTexCoord;
in vec3 vNormalVec;
in vec4 vFragPosInDlight[3];
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
    vec3 lightedColor = uBaseAmbient;
    vec3 fragNormal = vNormalVec;

    for (int i = 0; i < uDlightCount; ++i) {
        lightedColor += getDlightColor(i, viewDir, fragNormal, v_fragPos, vFragPosInDlight[i]);
    }
    lightedColor += getTotalPlightColors(viewDir, fragNormal, v_fragPos);
    lightedColor += getTotalSlightColors(viewDir, fragNormal, v_fragPos);
    lightedColor += getEnvColor(v_fragPos, fragNormal);

    vec4 texColor = texture(u_diffuseMap, vTexCoord);
    if (texColor.a < 0.5) {
        discard;
    }

    fColor = vec4(texColor.rgb * lightedColor, 1.0);
    fColor.xyz += calcDlightVolumeColor(0, v_fragPos);
    fColor.xyz += calcSlightVolumeColor(0, v_fragPos);
    fColor = calcFogMixedColor(fColor, v_fragPos);
}
