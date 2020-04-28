uniform vec3 u_viewPos;
uniform samplerCube u_skyboxTex;


in vec3 v_worldPos;

out vec4 f_color;


void main() {
    vec3 texCoord = normalize(v_worldPos - u_viewPos);
    vec3 texColor = texture(u_skyboxTex, texCoord).xyz;
    f_color = vec4(texColor, 1.0);
}
