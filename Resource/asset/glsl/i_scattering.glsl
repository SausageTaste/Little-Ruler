
const vec3 BETA_RAYLEIGH = vec3(0.0000051967317359, 0.0000121426979269, 0.0000296452586105);
const float RHO_RAYLEIGH = 0.9;
const float ANISOTROPY = 0.85;



float phase_mie(float cosTheta) {
    const float PI = 3.14;

    float numer = 3.0 * (1.0 - ANISOTROPY*ANISOTROPY) * (1.0 + cosTheta*cosTheta);
    float denom = 8.0*PI * (2.0 + ANISOTROPY*ANISOTROPY) * (1.0 + ANISOTROPY*ANISOTROPY - 2.0*ANISOTROPY*cosTheta);
    return numer / denom;
}

float phase_rayleigh(float cosTheta) {
    const float PI = 3.14;

    float numer = 3.0 * (1.0 + cosTheta*cosTheta);
    float denom = 16.0 * PI;
    return numer / denom;
}

vec3 transmittance_rayleigh(float dist) {
    vec3 exponent = -BETA_RAYLEIGH * RHO_RAYLEIGH * dist;
    return vec3(exp(exponent.x), exp(exponent.y), exp(exponent.z));
}
