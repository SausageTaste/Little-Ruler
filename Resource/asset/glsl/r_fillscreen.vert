layout (location = 0) in vec2 i_position;
layout (location = 1) in vec2 i_texCoord;


out vec2 v_texCoord;


void main() {
    gl_Position = vec4(i_position.x, i_position.y, 0.0, 1.0); 
    v_texCoord = i_texCoord;
}
