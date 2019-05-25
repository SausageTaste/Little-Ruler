

layout (location = 0) in vec3 iPosition;

uniform mat4 uProjViewMat;
uniform mat4 uModelMat;


void main() {
    gl_Position = uProjViewMat * uModelMat * vec4(iPosition, 1.0);
}
