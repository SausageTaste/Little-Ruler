layout (location = 0) in vec2 i_position;


uniform vec2 u_bottomLeft;
uniform vec2 u_rectSize;

uniform vec2 u_texOffset;
uniform vec2 u_texScale;


out vec2 v_uvCoord;


void main(void) {
    v_uvCoord = i_position * u_texScale + u_texOffset;
    gl_Position = vec4( (u_bottomLeft + i_position * u_rectSize), 0.0, 1.0 );
}
