#include <i_lighting.glsl>


uniform vec3 u_viewPos;
uniform vec3 u_viewPosActual;
uniform samplerCube u_skyboxTex;


in vec3 v_worldPos;

out vec4 f_color;


void main() {
    vec3 texCoord = normalize(v_worldPos - u_viewPos);
    vec3 texColor = texture(u_skyboxTex, texCoord).xyz;
    f_color = vec4(texColor, 1.0);

    for ( int i = 0; i < u_dlightCount; ++i ) {
        f_color.xyz += calcScatterColor_dlight(i, u_viewPosActual + texCoord * 30.0, u_viewPosActual);
    }
}
