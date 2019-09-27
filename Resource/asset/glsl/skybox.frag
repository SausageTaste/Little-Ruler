uniform vec3  u_fogColor;

uniform samplerCube u_skyboxTex;


in vec3 v_texCoords;

out vec4 f_color;


const float PI = 3.14159265;


void main() {
    const float CUTOFF_ANGLE = cos(5.0);
    const float lambda = 0.5;
    const float SHIFT = 0.0;

    float cosTheta = dot(normalize(v_texCoords), vec3(0.0, 1.0, 0.0)) + SHIFT;
    if (cosTheta < 0.0) {
        f_color = vec4(u_fogColor, 1.0);
    }
    else {
        float factor = 1.0 - clamp(sqrt(abs(cosTheta) / lambda), 0.0, 1.0);
        vec3 texColor = texture(u_skyboxTex, v_texCoords).xyz;
        f_color = vec4(mix(texColor, u_fogColor, factor), 1.0);
    }
}
