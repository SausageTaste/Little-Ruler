uniform mat4 u_projMat;
uniform mat4 u_viewMat;

uniform sampler2D u_texture;
uniform sampler2D u_depthMap;

in vec2 v_texCoord;

out vec4 f_color;


// https://stackoverflow.com/questions/32227283/getting-world-position-from-depth-buffer-value
vec3 WorldPosFromDepth() {
    float z = texture(u_depthMap, v_texCoord).r * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(v_texCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverse(u_projMat) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(u_viewMat) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

void main() { 
    const float GAMMA = 2.2;
    const float EXPOSURE = 1.0;

    vec3 texColor = texture(u_texture, v_texCoord).rgb;
    vec3 worldPos = WorldPosFromDepth();

    vec3 mapped = vec3(1.0) - exp(-texColor * EXPOSURE);
    //vec3 mapped = texColor / (texColor + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));

    f_color = vec4(mapped, 1.0);
}
