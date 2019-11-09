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
uniform highp int u_slightCount;
uniform mat4 u_slightProjViewMat[3];


out vec3 v_fragPos;
out vec2 vTexCoord;
out vec3 vNormalVec;
out vec4 vFragPosInDlight[3];
out vec4 v_fragPosInSlight[3];
#ifdef GL_ES
out float v_clipDistance;
#endif


void main(void) {
    mat4 boneMat = u_jointTransforms[i_jointIDs[0]] * i_weights[0];
    for (int i = 1; i < 3; i++) {
        int jid = i_jointIDs[i];
        if (-1 == jid) break;
        boneMat += u_jointTransforms[jid] * i_weights[i];
    }

    vec4 worldPos = u_modelMat * boneMat * vec4(i_position, 1.0);

    if (u_doClip) {
#ifdef GL_ES
        v_clipDistance = dot(worldPos, u_clipPlane);
#else
        gl_ClipDistance[0] = dot(worldPos, u_clipPlane);
#endif
    }

    gl_Position = u_projMat * u_viewMat * worldPos;
    v_fragPos = vec3(worldPos);
    vTexCoord = vec2(i_texCoord.x * u_texScale.x, i_texCoord.y * u_texScale.y);
    vNormalVec = normalize(vec3(u_modelMat * boneMat * vec4(i_normal, 0.0)));

    for (int i = 0; i < uDlightCount; i++) {
        vFragPosInDlight[i] = uDlightProjViewMat[i] * worldPos;
    }
    for (int i = 0; i < u_slightCount; ++i) {
        v_fragPosInSlight[i] = u_slightProjViewMat[i] * worldPos;
    }
}