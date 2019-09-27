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
uniform int       u_slightCount;

uniform float uShininess;
uniform float uSpecularStrength;
uniform float u_envReflectivity;
uniform float u_fogMaxPointInvSqr;
uniform vec3  u_fogColor;

uniform vec3  uPlightPoses[3];
uniform vec3  uPlightColors[3];
uniform float uPlightMaxDists[3];

uniform vec3       uDlightDirecs[3];
uniform vec3       uDlightColors[3];
uniform sampler2D  uDlightDepthMap[3];
uniform highp mat4 uDlightProjViewMat[3];

uniform SpotLight u_slights[3];

uniform samplerCube u_environmentMap;


const float PI = 3.14159265;


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


float _mapShininess(void) {
    const float a = 0.1;  // minShin
    const float b = 512.0;  // maxShin
    const float c = 0.5;  // forMin
    const float d = 0.001;  // forMax

    const float alpha = (d - c) / (1.0/b - 1.0/a);
    const float beta = c - alpha / a;

    return alpha / uShininess + beta;  // Broken.
}

vec3 getEnvColor(vec3 fragPos, vec3 fragNormal) {
    vec3 I = normalize(fragPos - uViewPos);
    vec3 R = reflect(I, fragNormal);
    return texture(u_environmentMap, R).rgb * u_envReflectivity;
}

vec3 getEnvColorMulti(vec3 fragPos, vec3 fragNormal) {
    const int kSampleCount = 1;

    float kSampleDistFactor = _mapShininess();
    vec3 color = vec3(0.0);

    for (int i = -kSampleCount; i <= kSampleCount; i++) {
        for (int j = -kSampleCount; j <= kSampleCount; j++) {
            for (int k = -kSampleCount; k <= kSampleCount; k++) {
                vec3 newPos = fragPos + vec3(i, j, k) * kSampleDistFactor;
                vec3 I = normalize(newPos - uViewPos);
                vec3 R = reflect(I, fragNormal);
                color += texture(u_environmentMap, R).rgb;
            }
        }
    }

    const int sampledCount = (2 * kSampleCount + 1) * (2 * kSampleCount + 1) * (2 * kSampleCount + 1);

    return color / float(sampledCount);
}


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

float _distanceDecreaser(float dist) {
    const float k_startDecrease = 15.0;
    const float k_endDecrea = 80.0;

    if (dist < k_startDecrease)
        return 1.0;
    else if (dist > k_endDecrea)
        return 0.0;
    else
        return (dist - k_endDecrea) / (k_startDecrease - k_endDecrea);
}

float _getLightFactor_directional(int index, vec3 viewDir, vec3 fragNormal) {
    vec3 lightLocDir = normalize(-uDlightDirecs[index]);

    float diff = max(dot(fragNormal, lightLocDir), 0.0);

    // Calculate specular lighting.
    if (uShininess <= 0.0) {
        return diff;
    }
    else {
        vec3 reflectDir = reflect(-lightLocDir, fragNormal);
        float energyConservation = ( 2.0 + uShininess ) / ( 2.0 * PI );
        float spec = energyConservation * pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
        float specular = max(uSpecularStrength * spec, 0.0);

        return diff + specular;
    }
}

float _computeScattering(float lightDotView) {
    const float G_SCATTERING = 10.0;

    float result = 1.0 - G_SCATTERING * G_SCATTERING;
    result /= (4.0 * PI * pow(1.0 + G_SCATTERING * G_SCATTERING - (2.0 * G_SCATTERING) *  lightDotView, 1.5));
    return result;
}

vec3 getDlightColor(int i, vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos, vec4 fragPosInShadow) {
    float factor = _getLightFactor_directional(i, fragToViewDirec, fragNormal) * _getShadowFactor_directional(i, fragPosInShadow);
    return factor * uDlightColors[i];
}

vec3 calcDlightVolumeColor(int index, vec3 fragPos) {
    const int NUM_STEPS = 3;
    const float INTENSITY = 0.05;

    vec3 toFragFromView = fragPos - uViewPos;
    vec3 toFargDirec = normalize(toFragFromView);
    vec3 toLightDirec = normalize(-uDlightDirecs[index]);
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
    return accumFactor * uDlightColors[index];
}


float _calcDistAttenu(float fragDist, float constant, float linear, float quadratic) {
    return 1.0 / (constant + linear * fragDist + quadratic * (fragDist * fragDist));
}

vec2 _calcPlightFactors(int index, vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos) {
    vec3 fragToLightDirec = normalize(uPlightPoses[index] - fragPos);

    float diffuseFactor = max(dot(fragToLightDirec, fragNormal), 0.0);

    vec3 halfwayDir = normalize(fragToViewDirec + fragToLightDirec);
    float energyConservation = ( 8.0 + uShininess ) / ( 8.0 * PI );
    float spec = energyConservation * pow(max(dot(fragNormal, halfwayDir), 0.0), uShininess);
    float specular = max(uSpecularStrength * spec, 0.0);

    return vec2(diffuseFactor, specular);
}

vec3 getTotalPlightColors(vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos) {
    vec3 lightedColor = vec3(0.0);

    for (int i = 0; i < uPlightCount; i++) {
        vec2 diffNSpec = _calcPlightFactors(i, fragToViewDirec, fragNormal, fragPos);
        float attenFactor = _calcDistAttenu(distance(fragPos, uPlightPoses[i]), 1.0, 0.09, 0.032);
        lightedColor += (diffNSpec.x + diffNSpec.y) * attenFactor * uPlightColors[i];
    }

    return lightedColor;
}


vec2 _calcSlightFactors(int index, vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos) {
    vec3 fragToLightDirec = normalize(u_slights[index].m_pos - fragPos);

    float diffuseFactor = max(dot(fragToLightDirec, fragNormal), 0.0);

    vec3 halfwayDir = normalize(fragToViewDirec + fragToLightDirec);
    float energyConservation = ( 8.0 + uShininess ) / ( 8.0 * PI );
    float spec = energyConservation * pow(max(dot(fragNormal, halfwayDir), 0.0), uShininess);
    float specular = max(uSpecularStrength * spec, 0.0);

    vec2 factors = vec2(diffuseFactor, specular);
    float lightAngle = dot(-fragToLightDirec, u_slights[index].m_direc);

    if (lightAngle > u_slights[index].m_startFade) {
        return factors;
    }
    else if (lightAngle > u_slights[index].m_endFade) {
        float edgeCut = (lightAngle - u_slights[index].m_endFade) / (u_slights[index].m_startFade - u_slights[index].m_endFade);
        return factors * edgeCut;
    }
    else {
        return vec2(0.0);
    }
}

vec3 getTotalSlightColors(vec3 fragToViewDirec, vec3 fragNormal, vec3 fragPos) {
    vec3 lightedColor = vec3(0.0);

    for (int i = 0; i < u_slightCount; ++i) {
        vec2 diffNSpec = _calcSlightFactors(i, fragToViewDirec, fragNormal, fragPos);
        float attenFactor = _calcDistAttenu(distance(fragPos, u_slights[i].m_pos), 1.0, 0.09, 0.032);
        lightedColor += (diffNSpec.x + diffNSpec.y) * attenFactor * u_slights[i].m_color;
    }

    return lightedColor;
}

vec3 calcSlightVolumeColor(int index, vec3 fragPos) {
    const int NUM_STEPS = 3;
    const float INTENSITY = 0.1;

    vec3 toFragFromView = fragPos - uViewPos;
    //vec3 toFargDirec = normalize(toFragFromView);
    //vec3 toLightDirec = normalize(-uDlightDirecs[index]);
    vec3 rayStep = toFragFromView / float(NUM_STEPS);
    //float scatterFactor = _computeScattering(dot(toFargDirec, toLightDirec));

    vec3 curPos = uViewPos;
    float accumFactor = 0.0;

    for (int i = 0; i < NUM_STEPS; ++i) {
        vec3 lightToFragDirec = normalize(curPos - u_slights[index].m_pos);
        float lightAngle = dot(lightToFragDirec, u_slights[index].m_direc);

        if (lightAngle > u_slights[index].m_startFade) {
            accumFactor += 1.0;
        }
        else if (lightAngle > u_slights[index].m_endFade) {
            float edgeCut = (lightAngle - u_slights[index].m_endFade) / (u_slights[index].m_startFade - u_slights[index].m_endFade);
            accumFactor += edgeCut;
        }

        curPos += rayStep * _getDitherValue();
    }

    accumFactor *= INTENSITY / float(NUM_STEPS);
    return accumFactor * u_slights[index].m_color;
}


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

vec4 calcFogMixedColor(vec4 color, vec3 fragPos) {
    float factor = _calcFogFactor(fragPos);
    vec3 mixedColor = mix(color.xyz, u_fogColor, factor);
    return vec4(mixedColor, color.a);
}
