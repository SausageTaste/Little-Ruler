

layout (location = 0) in vec2 i_position;


uniform vec2 u_bottLeft;
uniform vec2 u_rectSize;

uniform bool u_upsideDown_diffuseMap;
uniform bool u_upsideDown_maskMap;

uniform vec2 u_texOffset;
uniform vec2 u_texScale;

out vec2 v_texCoord_diffuse;
out vec2 v_texCoord_mask;


void main(void) {
    v_texCoord_diffuse = u_upsideDown_diffuseMap ? vec2(i_position.x, 1.0 - i_position.y) : i_position;
    v_texCoord_mask    = u_upsideDown_maskMap    ? vec2(i_position.x, 1.0 - i_position.y) : i_position;

    v_texCoord_diffuse = v_texCoord_diffuse * u_texScale + u_texOffset;
    v_texCoord_mask    = v_texCoord_mask    * u_texScale + u_texOffset;

    gl_Position = vec4( (u_bottLeft + i_position * u_rectSize), 0.0, 1.0 );
}
