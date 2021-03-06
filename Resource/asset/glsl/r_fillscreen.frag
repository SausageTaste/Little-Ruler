#include <i_lighting.glsl>
#include <i_scattering.glsl>


uniform highp mat4 u_projMat;
uniform highp mat4 u_viewMat;
uniform vec3 u_viewPos;

uniform sampler2D u_texture;
uniform highp sampler2D u_depthMap;

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

float _getDitherValue(void) {
    float ditherPattern[16] = float[](
        0.0   , 0.5   , 0.125 , 0.625 ,
        0.75  , 0.22  , 0.875 , 0.375 ,
        0.1875, 0.6875, 0.0625, 0.5625,
        0.9375, 0.4375, 0.8125, 0.3125
    );

    int i = int(gl_FragCoord.x) % 4;
    int j = int(gl_FragCoord.y) % 4;

    int index = 4 * i + j;
    return ditherPattern[index];
}

vec3 calcScattering(int index, vec3 fragPos, vec3 viewPos) {
    const int NUM_STEPS = 5;
    const float INTENSITY = 0.5;
    const float MAX_SAMPLE_DIST = 100.0;

    vec3 toFrag = fragPos - viewPos;
    float toFragDist = length(toFrag);
    vec3 toFarg_n = toFrag / toFragDist;
    vec3 sampleVec = toFragDist > MAX_SAMPLE_DIST ? toFarg_n * MAX_SAMPLE_DIST : toFrag;
    vec3 rayStep = sampleVec / float(NUM_STEPS);

    vec3 curPos = viewPos;
    float accumFactor = 0.0;

    for (int i = 0; i < NUM_STEPS; ++i) {
        vec4 curPosInDlight = u_dlight_projViewMat[index] * vec4(curPos, 1.0);
        vec3 projCoords = curPosInDlight.xyz / curPosInDlight.w;
        projCoords = projCoords * 0.5 + 0.5;

        float depthFromMap = _sampleDlightDepth(index, projCoords.xy);
        float curDepth = projCoords.z;

        if (depthFromMap > curDepth) {
            accumFactor += 1.0;
        }

        curPos += rayStep * _getDitherValue();
    }

    accumFactor *= INTENSITY / float(NUM_STEPS);
    return accumFactor * u_dlight_colors[index] * phase_mie(dot(toFarg_n, normalize(-u_dlight_direcs[index])));
}

vec3 calcScattering_slight(int index, vec3 fragPos, vec3 viewPos) {
    const int NUM_STEPS = 5;
    const float INTENSITY = 0.3;
    const float MAX_SAMPLE_DIST = 5.0;

    vec3 toFrag = fragPos - viewPos;
    float toFragDist = length(toFrag);
    vec3 toFarg_n = toFrag / toFragDist;
    vec3 sampleVec = toFragDist > MAX_SAMPLE_DIST ? toFarg_n * MAX_SAMPLE_DIST : toFrag;
    vec3 rayStep = sampleVec / float(NUM_STEPS);

    vec3 curPos = viewPos;
    float accumFactor = 0.0;

    for (int i = 0; i < NUM_STEPS; ++i) {
        vec4 curPosInLight = u_slight_projViewMat[index] * vec4(curPos, 1.0);
        bool isInShadow = isInShadow_slight(index, curPosInLight);
        accumFactor += isInShadow ? 0.0 : _calcSlightAtten(index, fragPos);

        curPos += rayStep * _getDitherValue();
    }

    accumFactor *= INTENSITY / float(NUM_STEPS);
    return accumFactor * u_slight_colors[index];
}

vec3 fixColor(vec3 color) {
    const float GAMMA = 2.2;
    const float EXPOSURE = 0.8;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));
    return mapped;
}


void main() {
    vec3 texColor = texture(u_texture, v_texCoord).rgb;
    vec3 worldPos = WorldPosFromDepth();

    f_color = vec4(texColor, 1.0);

#ifdef DAL_VOLUMETRIC_LIGHT
    for ( int i = 0; i < u_dlightCount; ++i ) {
        f_color.xyz += calcScattering(i, worldPos, u_viewPos);
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        //f_color.xyz += calcScattering_slight(i, worldPos, u_viewPos);
    }
#endif

    f_color.xyz = fixColor(f_color.xyz);
}
