uniform sampler2D u_texture;

in vec2 v_texCoord;

out vec4 f_color;


void main() { 
    const float GAMMA = 2.2;
    const float EXPOSURE = 1.0;

    vec3 texColor = texture(u_texture, v_texCoord).rgb;

    vec3 mapped = vec3(1.0) - exp(-texColor * EXPOSURE);
    //vec3 mapped = texColor / (texColor + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));

    f_color = vec4(mapped, 1.0);
}
