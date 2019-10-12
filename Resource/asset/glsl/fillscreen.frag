uniform sampler2D uTexture;


in vec2 v_texCoord;

out vec4 f_color;


void main() { 
    const float GAMMA = 2.2;
    const float EXPOSURE = 0.5;

    vec3 texColor = texture(uTexture, v_texCoord).rgb;

    //vec3 mapped = vec3(1.0) - exp(-texColor * EXPOSURE);
    vec3 mapped = texColor / (texColor + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));

    f_color = vec4(mapped, 1.0);
}
