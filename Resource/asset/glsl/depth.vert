
// Interf - Geometry
layout (location = 0) in vec3 i_position;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;


void main() {
    gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(i_position, 1.0);
}
