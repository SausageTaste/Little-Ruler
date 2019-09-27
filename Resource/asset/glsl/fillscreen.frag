uniform sampler2D uTexture;


in vec2 v_texCoord;

out vec4 f_color;


void main() { 
    const float GAMMA = 2.2;

    f_color = texture(uTexture, v_texCoord);
    f_color.xyz = pow(f_color.xyz, vec3(1.0 / GAMMA));
}
