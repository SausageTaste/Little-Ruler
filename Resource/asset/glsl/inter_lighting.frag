struct PointLight {
    vec3 m_pos;
    vec3 m_color;
};

struct DirecLight {
    vec3 m_direc;
    vec3 m_color;
};

struct SpotLight {
    vec3 m_pos;
    vec3 m_direc;
    vec3 m_color;
    float m_startFade;
    float m_endFade;
};


uniform highp vec3 uViewPos;

uniform vec3      uBaseAmbient;
uniform highp int uDlightCount;
uniform int       uPlightCount;
uniform highp int u_slightCount;

uniform float uShininess;
uniform float uSpecularStrength;
uniform float u_envReflectivity;
uniform float u_fogMaxPointInvSqr;
uniform vec3  u_fogColor;

uniform PointLight u_plights[3];

uniform DirecLight u_dlights[3];
uniform sampler2D  uDlightDepthMap[3];
uniform highp mat4 uDlightProjViewMat[3];

uniform SpotLight  u_slights[3];
uniform sampler2D  u_slightDepthMap[3];
uniform highp mat4 u_slightProjViewMat[3];

uniform samplerCube u_environmentMap;


const float PI = 3.14159265;
const float EPSILON = 0.000000001;


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

vec3 getEnvColor(vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - uViewPos);
    vec3 R = reflect(I, fragNormal);
    return texture(u_environmentMap, R).rgb * u_envReflectivity;
}


// Simple light

float _computeScattering(float lightDotView) {
    const float G_SCATTERING = 10.0;

    float result = 1.0 - G_SCATTERING * G_SCATTERING;
    result /= (4.0 * PI * pow(1.0 + G_SCATTERING * G_SCATTERING - (2.0 * G_SCATTERING) *  lightDotView, 1.5));
    return result;
}

float _calcDistAttenu(float fragDist, float constant, float linear, float quadratic) {
    return 1.0 / (constant + linear * fragDist + quadratic * (fragDist * fragDist));
}

float _calcDiffuse(vec3 fragToLight_n, vec3 fragNormal_n) {
    return max(dot(fragToLight_n, fragNormal_n), 0.0);
}

float _calcSpecular(vec3 fragToLight_n, vec3 fragNormal_n, vec3 fragToView_n) {
    vec3 halfwayDir = normalize(fragToView_n + fragToLight_n);
    float energyConservation = ( 8.0 + uShininess ) / ( 8.0 * PI );
    float spec = energyConservation * pow(max(dot(fragNormal_n, halfwayDir), 0.0), uShininess);
    return max(uSpecularStrength * spec, 0.0);
}


// Directional Light

float _sampleDlightTexture(int index, vec2 coord) {

#ifdef GL_ES
    switch (index){
        case 0:
            return texture(uDlightDepthMap[0], coord).r;
        case 1:
            return texture(uDlightDepthMap[1], coord).r;
        case 2:
            return texture(uDlightDepthMap[2], coord).r;
        default:
            return 1.0;
    }
#else
    return texture(uDlightDepthMap[index], coord).r;
#endif

}

float _sampleDlightDepth(int index, vec2 coord) {
    if (coord.x > 1.0 || coord.x < 0.0) return 1.0;
    if (coord.y > 1.0 || coord.y < 0.0) return 1.0;
    return _sampleDlightTexture(index, coord);
}

float _getShadowFactor_directional(int index, vec4 fragPosInDlight) {
    vec3 projCoords = fragPosInDlight.xyz / fragPosInDlight.w;
    if (projCoords.z > 1.0) return 1.0;

    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = _sampleDlightDepth(index, projCoords.xy);
    float currentDepth = projCoords.z;

    //float bias = max(0.05 * (1.0 - dot(vNormalVec, -uDlightDirs[index])), 0.005);
    //float bias = 0.002;
    float bias = 0.0;
    float shadow = (currentDepth - bias) > closestDepth ? 0.0 : 1.0;

    return shadow;
}

float _getLightFactor_directional(int index, vec3 viewDir, vec3 fragNormal) {
    vec3 lightLocDir = normalize(-u_dlights[index].m_direc);

    float diffuse = _calcDiffuse(lightLocDir, fragNormal);
    float specular = _calcSpecular(lightLocDir, fragNormal, viewDir);

    return diffuse + specular;
}

vec3 getDlightColor(int i, vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos, vec4 fragPosInShadow) {
    float factor = _getLightFactor_directional(i, fragToViewDirec, fragNormal) * _getShadowFactor_directional(i, fragPosInShadow);
    return factor * u_dlights[i].m_color;
}

vec3 calcDlightVolumeColor(int index, vec3 fragPos) {
    const int NUM_STEPS = 3;
    const float INTENSITY = 0.05;

    vec3 toFragFromView = fragPos - uViewPos;
    vec3 toFargDirec = normalize(toFragFromView);
    vec3 toLightDirec = normalize(-u_dlights[index].m_direc);
    vec3 rayStep = toFragFromView / float(NUM_STEPS);
    float scatterFactor = _computeScattering(dot(toFargDirec, toLightDirec));

    vec3 curPos = uViewPos;
    float accumFactor = 0.0;

    for (int i = 0; i < NUM_STEPS; ++i) {
        vec4 curPosInDlight = uDlightProjViewMat[index] * vec4(curPos, 1.0);
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
    return accumFactor * u_dlights[index].m_color;
}


// Point Light

vec2 _calcPlightFactors(int index, vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos) {
    vec3 fragToLightDirec = normalize(u_plights[index].m_pos - fragPos);

    float diffuse = _calcDiffuse(fragToLightDirec, fragNormal);
    float specular = _calcSpecular(fragToLightDirec, fragNormal, fragToViewDirec);

    return vec2(diffuse, specular);
}

vec3 calcPlightRadiance(int i, vec3 fragPos) {
    float attenFactor = _calcDistAttenu(distance(fragPos, u_plights[i].m_pos), 1.0, 0.09, 0.032);
    return u_plights[i].m_color * attenFactor;
}

vec3 getTotalPlightColors(vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos) {
    vec3 lightedColor = vec3(0.0);

    for (int i = 0; i < uPlightCount; i++) {
        vec2 diffNSpec = _calcPlightFactors(i, fragToViewDirec, fragNormal, fragPos);
        lightedColor += (diffNSpec.x + diffNSpec.y) * calcPlightRadiance(i, fragPos);
    }

    return lightedColor;
}


// Spot Light

vec2 _calcSlightFactors(int index, vec3 fragToView_n, vec3 fragNormal, vec3 fragPos) {
    vec3 fragToLight_n = normalize(u_slights[index].m_pos - fragPos);
    float diffuse = _calcDiffuse(fragToLight_n, fragNormal);
    float specular = _calcSpecular(fragToLight_n, fragNormal, fragToView_n);
    return vec2(diffuse, specular);
}

float _calcSlightAtten(int i, vec3 fragPos) {
    vec3 fragToLight_n = normalize(u_slights[i].m_pos - fragPos);
    float theta        = dot(-fragToLight_n, u_slights[i].m_direc);
    float epsilon      = u_slights[i].m_startFade - u_slights[i].m_endFade;
    return clamp((theta - u_slights[i].m_endFade) / epsilon, 0.0, 1.0);
}

float _sampleSlightTexture(int index, vec2 coord) {

#ifdef GL_ES
    switch (index){

    case 0:
        return texture(u_slightDepthMap[0], coord).r;
    case 1:
        return texture(u_slightDepthMap[1], coord).r;
    case 2:
        return texture(u_slightDepthMap[2], coord).r;
    default:
        return 1.0;

    }
#else
    return texture(u_slightDepthMap[index], coord).r;
#endif

}

bool isPointInSlightShadow(int i, vec4 fragPosInSlight) {
    vec3 projCoords = fragPosInSlight.xyz / fragPosInSlight.w;
    if (projCoords.z > 1.0)
        return false;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.x > 1.0 || projCoords.x < 0.0 || projCoords.y > 1.0 || projCoords.y < 0.0)
        return false;

    float closestDepth = _sampleSlightTexture(i, projCoords.xy);
    float currentDepth = projCoords.z;

    const float BIAS = 0.0;
    return (currentDepth - BIAS) > closestDepth ? true : false;
}

vec3 calcSlightRadiance(int i, vec3 fragPos) {
    return u_slights[i].m_color * _calcSlightAtten(i, fragPos);
}

vec3 getSlightColor(int i, vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos, vec4 fragPosInShadow) {
    vec2 diffNSpec = _calcSlightFactors(i, fragToViewDirec, fragNormal, fragPos);
    bool isInShadow = isPointInSlightShadow(i, fragPosInShadow);
    return isInShadow ? vec3(0.0) : (diffNSpec.x + diffNSpec.y) * calcSlightRadiance(i, fragPos);
}

vec3 calcSlightVolumeColor(int index, vec3 fragPos) {
    const int NUM_STEPS = 3;
    const float INTENSITY = 0.3;

    vec3 toFragFromView = fragPos - uViewPos;
    //vec3 toFargDirec = normalize(toFragFromView);
    //vec3 toLightDirec = normalize(-u_dlights[index].m_direc);
    vec3 rayStep = toFragFromView / float(NUM_STEPS);
    //float scatterFactor = _computeScattering(dot(toFargDirec, toLightDirec));

    vec3 curPos = uViewPos;
    float accumFactor = 0.0;

    for (int i = 0; i < NUM_STEPS; ++i) {
        vec3 lightToFragDirec = normalize(curPos - u_slights[index].m_pos);
        float lightAngle = dot(lightToFragDirec, u_slights[index].m_direc);

        vec4 curPosInDlight = u_slightProjViewMat[index] * vec4(curPos, 1.0);
        bool isInShadow = isPointInSlightShadow(index, curPosInDlight);
        accumFactor += isInShadow ? 0.0 : _calcSlightAtten(index, fragPos);

        curPos += rayStep * _getDitherValue();
    }

    accumFactor *= INTENSITY / float(NUM_STEPS);
    return accumFactor * u_slights[index].m_color;
}


// Fog

float _calcDistanceFogFactor(vec3 fragPos) {
    vec3 fromViewToFrag = fragPos - uViewPos;
    float fragDistanceSqr = dot(fromViewToFrag, fromViewToFrag);
    float factor = fragDistanceSqr * (u_fogMaxPointInvSqr);
    return min(factor, 1.0);
}

float _calcFogFactor(vec3 fragPos) {
    float factor = _calcDistanceFogFactor(fragPos);
    return clamp(factor, 0.0, 1.0);
}

vec3 calcFogMixedColor(vec3 color, vec3 fragPos) {
    float factor = _calcFogFactor(fragPos);
    vec3 mixedColor = mix(color, u_fogColor, factor);
    return mixedColor;
}


// PBR

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, EPSILON); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 lightingIntergrateStep(vec3 N, vec3 V, vec3 F0, vec3 L, vec3 albedo, vec3 radiance) {
    const float metallic = 0.0;
    const float roughness = 0.7;

    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = nominator / max(denominator, EPSILON);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}
