#include <inter_lighting.frag>

// Interf - Mesh
uniform sampler2D u_diffuseMap;  // TEX 0

// Interf - PlaneClip
uniform bool u_doClip;
uniform vec4 u_clipPlane;


in vec3 vFragPos;
in vec2 vTexCoord;
in vec3 vNormalVec;
in vec4 vFragPosInDlight[3];

out vec4 fColor;


void main(void) {
#ifdef GL_ES
    if (u_doClip) {
        if ( dot(vec4(vFragPos, 1.0), u_clipPlane) < 0.0 ) discard;
    }
#endif

    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 lightedColor = uBaseAmbient;
    vec3 fragNormal = vNormalVec;

    int i;
    for (i = 0; i < uDlightCount; i++) {
        lightedColor += getDlightFactor(i, viewDir, fragNormal, vFragPosInDlight[i]) * uDlightColors[i];
    }
    for (i = 0; i < uPlightCount; i++) {
        lightedColor += getLightFactor_point(i, viewDir, fragNormal, vFragPos) * uPlightColors[i];
    }

    vec4 texColor = texture(u_diffuseMap, vTexCoord);
    if (texColor.a == 0.0) discard;
    fColor = texColor * vec4(lightedColor, 1.0);
}