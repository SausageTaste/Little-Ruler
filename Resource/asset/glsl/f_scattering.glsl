#include <m_geometry.glsl>


const float RADIUS_EARTH = 6360000.0;
const float RADIUS_ATMOS = 6420000.0;

const float REFRACTIVE_INDEX = 1.00029;
const float REFRACTIVE_INDEX_SQR = REFRACTIVE_INDEX * REFRACTIVE_INDEX;
const float NUM_OF_MODELCULARS_AT_SEA_LEVEL = 2.504e25;

const vec3 SCATT_COEF_AT_SEA_LVL_RAYLEIGH = vec3(3.8e-6, 13.5e-6, 33.1e-6);
//const vec3 SCATT_COEF_AT_SEA_LVL_RAYLEIGH = vec3(0.0000051967317359, 0.0000121426979269, 0.0000296452586105);
//const float SCATT_COEF_AT_SEA_LVL_MIE = 21.0e-6;


float densityRatio_rayleigh(float height) {
    return exp(-height / 8500.0);
}

vec3 transmittance_rayleigh(vec3 a, vec3 b, vec3 earthCenter) {
    const int SAMPLE_COUNT = 10;

    vec3 deltaPoint = (b - a) / float(SAMPLE_COUNT);
    float opticalDepth = 0.0;
    for ( int i = 1; i <= SAMPLE_COUNT; ++i ) {
        vec3 samplePoint = a + deltaPoint * float(i);
        float sampleHeight = distance(samplePoint, earthCenter) - RADIUS_EARTH;
        opticalDepth += densityRatio_rayleigh(sampleHeight);
    }
    
    vec3 expoent = -SCATT_COEF_AT_SEA_LVL_RAYLEIGH * opticalDepth * distance(a, b);
    return vec3(exp(expoent.r), exp(expoent.g), exp(expoent.b));
}

float phase_rayleigh(float cosTheta) {
    const float PI = 3.14159265;
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

/*
float densityRatio_mie(float height) {
    return exp(-height / 1200.0);
}

float transmittance_mie(vec3 a, vec3 b, vec3 earthCenter) {
    const int SAMPLE_COUNT = 10;

    vec3 deltaPoint = (b - a) / float(SAMPLE_COUNT);
    float opticalDepth = 0.0;
    for ( int i = 1; i <= SAMPLE_COUNT; ++i ) {
        vec3 samplePoint = a + deltaPoint * float(i);
        float sampleHeight = distance(samplePoint, earthCenter) - RADIUS_EARTH;
        opticalDepth += densityRatio_mie(sampleHeight);
    }
    
    float expoent = -1.1 * SCATT_COEF_AT_SEA_LVL_MIE * opticalDepth * distance(a, b);
    return exp(expoent);
}

float phase_mie(float cosTheta, float g) {
    float numer = 3.0 * (1.0 - g*g) * (1.0 + cosTheta*cosTheta);
    float denom = 8.0 * 3.14159265 * (2.0 + g*g) * pow(1.0 + g*g - 2.0*g*cosTheta, 1.5);

    return numer / denom;
}
*/


vec3 calcSunLightPos(vec3 viewPos, vec3 samplePoint, vec3 direc) {
    Segment seg;
    seg.m_pos = samplePoint;
    seg.m_rel = direc * 500.0;

    Sphere atmos;
    atmos.m_center = vec3(viewPos.x, -RADIUS_EARTH, viewPos.z);
    atmos.m_radius = RADIUS_ATMOS;

    vec4 intersection = intersect_seg_sphere(seg, atmos);
    return intersection.xyz;
}

vec3 skyColor(vec3 viewPos, vec3 viewDirec, vec3 dlight_direc) {
    const vec3 DLIGHT_COLOR = vec3(200.0);
    const int SAMPLE_COUNT = 10;

    vec3 earthCenter = vec3(viewPos.x, -RADIUS_EARTH, viewPos.z);
    vec3 endpoint = calcSunLightPos(viewPos, viewPos, viewDirec);

    float dist = distance(viewPos, endpoint);
    float deltaDist = dist / float(SAMPLE_COUNT);
    vec3 sampleDirec_n = normalize(endpoint - viewPos);
    float cosTheta = dot(normalize(-dlight_direc), sampleDirec_n);

    vec3 result = vec3(0.0);

    for ( int i = 1 ; i <= SAMPLE_COUNT; ++i ) {
        vec3 samplePoint = viewPos + sampleDirec_n * deltaDist * float(i);
        float sampleHeight = distance(samplePoint, earthCenter) - RADIUS_EARTH;
        vec3 sampleDlightPos = calcSunLightPos(viewPos, samplePoint, -dlight_direc);

        vec3 light_rayleigh = DLIGHT_COLOR * transmittance_rayleigh(samplePoint, sampleDlightPos, earthCenter) * phase_rayleigh(cosTheta) * SCATT_COEF_AT_SEA_LVL_RAYLEIGH * densityRatio_rayleigh(sampleHeight);
        result += light_rayleigh * transmittance_rayleigh(samplePoint, viewPos, earthCenter) * deltaDist;

        //vec3 light_mie      = DLIGHT_COLOR * transmittance_mie(samplePoint, sampleDlightPos, earthCenter) * phase_mie(cosTheta, 0.7) * SCATT_COEF_AT_SEA_LVL_MIE * densityRatio_mie(sampleHeight);
        //result += light_mie * transmittance_mie(samplePoint, viewPos, earthCenter) * deltaDist;
    }

    return result;
}
