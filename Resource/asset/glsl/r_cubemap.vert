layout (location = 0) in vec3 i_position;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;


out vec3 v_localPos;


void main() {
    v_localPos = i_position;
    vec4 clipPos = u_projMat * u_viewMat * vec4(v_localPos, 1.0);
    gl_Position = clipPos.xyww;
}
