// Interf - Geometry
layout (location = 0) in vec3 i_position;

// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;


out vec3 v_texCoords;


void main() {
    v_texCoords = i_position;
    vec4 pos = u_projMat * u_viewMat * vec4(i_position, 1.0);
    gl_Position = pos.xyww;
}
