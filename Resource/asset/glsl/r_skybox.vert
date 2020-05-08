layout (location = 0) in vec3 i_position;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;


out vec3 v_worldPos;


void main() {
    vec4 worldPos = u_modelMat * vec4(i_position, 1.0);
    v_worldPos = worldPos.xyz;
    vec4 pos = u_projMat * u_viewMat * worldPos;
    gl_Position = pos.xyww;
}
