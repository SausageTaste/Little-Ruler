layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec2 iTexCoord;
layout (location = 2) in vec3 iNormal;
layout (location = 3) in ivec3 i_jointIDs;
layout (location = 4) in vec3 i_weights;

uniform mat4 uProjectMat;
uniform mat4 uViewMat;
uniform mat4 uModelMat;

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
out vec4 v_worldPos;


void main(void) {
    mat4 boneMat = u_poses[i_jointIDs[0]] * i_weights[0];
    for (int i = 1; i < 3; i++) {
        int jid = i_jointIDs[i];
        if (-1 == jid) break;
        boneMat += u_poses[jid] * i_weights[i];
    }

    v_worldPos = uModelMat * boneMat * vec4(iPosition, 1.0);

#ifndef GL_ES
    if (u_doClip) {
        gl_ClipDistance[0] = dot(v_worldPos, u_clipPlane);
    }
#endif

    gl_Position = uProjectMat * uViewMat * v_worldPos;
    vFragPos = vec3(uModelMat * vec4(iPosition, 1.0));
    vTexCoord = vec2(iTexCoord.x * uTexScaleX, -iTexCoord.y * uTexScaleY);
    vNormalVec = normalize(vec3(uModelMat * vec4(iNormal, 0.0)));

    for (int i = 0; i < uDlightCount; i++) {
        vFragPosInDlight[i] = uDlightProjViewMat[i] * vec4(vFragPos, 1.0);
    }
}