uniform sampler2D u_colorMap;
uniform bool      u_hasColorMap;
uniform vec4      u_colorDefault;

uniform sampler2D u_maskMap;
uniform bool      u_hasMaskMap;


in vec2 v_uvCoord;

out vec4 f_color;


void main() {
    f_color   = u_hasColorMap ? texture(u_colorMap, v_uvCoord)   : u_colorDefault;
    f_color.a = u_hasMaskMap  ? texture(u_maskMap,  v_uvCoord).r : f_color.a;
}
