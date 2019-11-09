// Interf - Geometry
layout (location = 0) in vec3 i_position;

// Interf - Anime
layout (location = 3) in ivec3 i_jointIDs;
layout (location = 4) in vec3 i_weights;


// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

// Interf - Anime
uniform mat4 u_jointTransforms[30];


void main() {
    mat4 boneMat = u_jointTransforms[i_jointIDs[0]] * i_weights[0];
    for (int i = 1; i < 3; i++) {
        int jid = i_jointIDs[i];
        if (-1 == jid) break;
        boneMat += u_jointTransforms[jid] * i_weights[i];
    }

    gl_Position = u_projMat * u_viewMat * u_modelMat * boneMat * vec4(i_position, 1.0);
}
