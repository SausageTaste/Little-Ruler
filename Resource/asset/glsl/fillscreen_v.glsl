

layout (location = 0) in vec2 iPosition;
layout (location = 1) in vec2 iTexCoord;


out vec2 vTexCoord;


void main() {
	gl_Position = vec4(iPosition.x, iPosition.y, 0.0, 1.0); 
    vTexCoord = iTexCoord;
}