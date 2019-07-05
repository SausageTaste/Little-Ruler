// Interf - Geometry
layout (location = 0) in vec3 i_position;

// Interf - Mesh
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;

// Interf - Anime
layout (location = 3) in ivec3 i_jointIDs;
layout (location = 4) in vec3 i_weights;


// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

// Interf - Mesh
uniform vec2 u_texScale;

// Interf - Anime
uniform mat4 u_jointTransforms[30];

// Interf - PlaneClip
uniform bool u_doClip;
uniform vec4 u_clipPlane;

uniform highp int uDlightCount;

uniform mat4 uDlightProjViewMat[3];


out vec3 vFragPos;
out vec2 vTexCoord;
out vec3 vNormalVec;
out vec4 vFragPosInDlight[3];


void main(void) {
    mat4 boneMat = u_jointTransforms[i_jointIDs[0]] * i_weights[0];
    for (int i = 1; i < 3; i++) {
        int jid = i_jointIDs[i];
        if (-1 == jid) break;
        boneMat += u_jointTransforms[jid] * i_weights[i];
    }

    vec4 worldPos = u_modelMat * boneMat * vec4(i_position, 1.0);

#ifndef GL_ES
    if (u_doClip) {
        gl_ClipDistance[0] = dot(worldPos, u_clipPlane);
    }
#endif

    gl_Position = u_projMat * u_viewMat * worldPos;
    vFragPos = vec3(worldPos);
    vTexCoord = vec2(i_texCoord.x * u_texScale.x, -i_texCoord.y * u_texScale.y);
    vNormalVec = normalize(vec3(u_modelMat * boneMat * vec4(i_normal, 0.0)));

    for (int i = 0; i < uDlightCount; i++) {
        vFragPosInDlight[i] = uDlightProjViewMat[i] * worldPos;
    }
}