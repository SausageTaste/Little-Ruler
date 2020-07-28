
const float PI = 3.14159265;
const float EPSILON = 0.0001;


float _DistributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, EPSILON); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float _GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float _GeometrySmith(float NdotV, float NdotL, float roughness) {
    float ggx2 = _GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = _GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 _fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 integratePBR(vec3 N, vec3 V, vec3 F0, vec3 L, vec3 albedo, float roughness, float metallic, float specularIntensity) {
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    float NDF = _DistributionGGX(NdotH, roughness);
    float G   = _GeometrySmith(NdotV, NdotL, roughness);
    vec3  F   = _fresnelSchlick(NdotH, F0);

    vec3  nominator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL;
    vec3  specular    = specularIntensity * nominator / max(denominator, EPSILON);

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    return (kD * albedo / PI + specular) * NdotL;
}
