uniform samplerCube u_skyboxTex;


in vec3 v_texCoords;

out vec4 f_color;


void main() {
    f_color = texture(u_skyboxTex, v_texCoords);
}
