

uniform vec4 u_color;

uniform sampler2D u_diffuseMap;
uniform bool u_hasDiffuseMap;

uniform sampler2D u_maskMap;
uniform bool u_hasMaskMap;


in vec2 v_texCoord_diffuse;
in vec2 v_texCoord_mask;

out vec4 f_color;


void main() {
    f_color = u_hasDiffuseMap ? texture(u_diffuseMap, v_texCoord_diffuse) : u_color;
    f_color.a = u_hasMaskMap  ? texture(u_maskMap   , v_texCoord_mask).r  : f_color.a;
}
