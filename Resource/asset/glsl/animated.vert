// Interf - Geometry
layout (location = 0) in vec3 i_position;

layout (location = 1) in vec2 iTexCoord;
layout (location = 2) in vec3 iNormal;
layout (location = 3) in ivec3 i_jointIDs;
layout (location = 4) in vec3 i_weights;

// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

// From Master
uniform highp int uDlightCount;
uniform bool u_doClip;
uniform vec4 u_clipPlane;

// From Light
uniform mat4 uDlightProjViewMat[3];

// From Material
uniform float uTexScaleX;
uniform float uTexScaleY;

// From animation
uniform mat4 u_poses[30];


out vec3 vFragPos;
out vec2 vTexCoord;
out vec3 vNormalVec;
out vec4 vFragPosInDlight[3];


void main(void) {
    mat4 boneMat = u_poses[i_jointIDs[0]] * i_weights[0];
    for (int i = 1; i < 3; i++) {
        int jid = i_jointIDs[i];
        if (-1 == jid) break;
        boneMat += u_poses[jid] * i_weights[i];
    }

    vec4 worldPos = u_modelMat * boneMat * vec4(i_position, 1.0);

#ifndef GL_ES
    if (u_doClip) {
        gl_ClipDistance[0] = dot(worldPos, u_clipPlane);
    }
#endif

    gl_Position = u_projMat * u_viewMat * worldPos;
    vFragPos = vec3(worldPos);
    vTexCoord = vec2(iTexCoord.x * uTexScaleX, -iTexCoord.y * uTexScaleY);
    vNormalVec = normalize(vec3(u_modelMat * boneMat * vec4(iNormal, 0.0)));

    for (int i = 0; i < uDlightCount; i++) {
        vFragPosInDlight[i] = uDlightProjViewMat[i] * worldPos;
    }
}