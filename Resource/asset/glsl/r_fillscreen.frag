#include <i_lighting.glsl>
#include <i_scattering.glsl>


uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform vec3 u_viewPos;

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

vec3 calcScattering(int index, vec3 albedo, vec3 worldPos) {
    const int SAMPLE_COUNT = 10;
    const float MAX_SAMPLE_DIST = 100.0;
    const float DLIGHT_DISTANCE = 10000.0;

    vec3 toFrag = worldPos - u_viewPos;
    float toFragDist = length(toFrag);
    vec3 toFrag_n = toFrag / toFragDist;
    vec3 sampleVec = toFragDist > MAX_SAMPLE_DIST ? toFrag_n * MAX_SAMPLE_DIST : toFrag;

    float cosTheta = dot(sampleVec, -u_dlight_direcs[index]);
    vec3 sampleStep = sampleVec / float(SAMPLE_COUNT + 1);
    vec3 accum = vec3(0.0);
    vec3 curPos = u_viewPos;

    for ( int i = 0; i < SAMPLE_COUNT; ++i ) {
        curPos += sampleStep * _getDitherValue();;

        vec4 curPosInDlight = u_dlight_projViewMat[index] * vec4(curPos, 1.0);
        vec3 projCoords = curPosInDlight.xyz / curPosInDlight.w;
        projCoords = projCoords * 0.5 + 0.5;
        float depthFromMap = _sampleDlightDepth(index, projCoords.xy);
        float curDepth = projCoords.z;
        float inShadow = depthFromMap > curDepth ? 1.0 : 0.0;

        accum += inShadow * transmittance_rayleigh(DLIGHT_DISTANCE + distance(curPos, u_viewPos));
    }

    vec3 inScatter = u_dlight_colors[index] * phase_rayleigh(cosTheta) * BETA_RAYLEIGH * RHO_RAYLEIGH * accum * length(sampleVec);
    vec3 materialColor = transmittance_rayleigh(DLIGHT_DISTANCE + toFragDist) * albedo;

    return inScatter + materialColor;
}

vec3 fixColor(vec3 color) {
    const float GAMMA = 2.2;
    const float EXPOSURE = 1.0;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));
    return mapped;
}


void main() {
    vec3 texColor = texture(u_texture, v_texCoord).rgb;
    vec3 worldPos = WorldPosFromDepth();

    f_color = vec4(0.0, 0.0, 0.0, 1.0);

#ifdef DAL_VOLUMETRIC_LIGHT
    for ( int i = 0; i < u_dlightCount; ++i ) {
        f_color.xyz += calcScattering(i, texColor, worldPos);
    }
#endif

    f_color.xyz = fixColor(f_color.xyz);
}
