layout (location = 0) in vec2 i_position;


uniform vec2 u_bottomLeft;
uniform vec2 u_rectSize;

uniform vec2 u_texOffset;
uniform vec2 u_texScale;

uniform bool u_yFlip_colorMap;
uniform bool u_yFlip_maskMap;


out vec2 v_texCoord_color;
out vec2 v_texCoord_mask;


void main(void) {
    v_texCoord_color = u_yFlip_colorMap ? vec2(i_position.x, 1.0 - i_position.y) : i_position;
    v_texCoord_mask  = u_yFlip_maskMap  ? vec2(i_position.x, 1.0 - i_position.y) : i_position;

    v_texCoord_color = v_texCoord_color * u_texScale + u_texOffset;
    v_texCoord_mask  = v_texCoord_mask  * u_texScale + u_texOffset;

    gl_Position = vec4( (u_bottomLeft + i_position * u_rectSize), 0.0, 1.0 );
}
