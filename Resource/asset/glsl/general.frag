#include <inter_lighting.frag>


// Interf - PlaneClip
uniform bool u_doClip;

uniform sampler2D u_diffuseMap;  // TEX 0


in vec3 vFragPos;
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

    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 lightedColor = uBaseAmbient;
    vec3 fragNormal = vNormalVec;

    for (int i = 0; i < uDlightCount; ++i) {
        lightedColor += getDlightColor(i, viewDir, fragNormal, vFragPos, vFragPosInDlight[i]);
    }
    lightedColor += getTotalPlightColors(viewDir, fragNormal, vFragPos);
    lightedColor += getTotalSlightColors(viewDir, fragNormal, vFragPos);
    lightedColor += getEnvColor(vFragPos, fragNormal);

    vec4 texColor = texture(u_diffuseMap, vTexCoord);
    if (texColor.a < 0.5) {
        discard;
    }

    fColor = vec4(texColor.rgb * lightedColor, 1.0);
    fColor.xyz += calcDlightVolumeColor(0, vFragPos);
    fColor.xyz += calcSlightVolumeColor(0, vFragPos);
    fColor = calcFogMixedColor(fColor, vFragPos);
}
