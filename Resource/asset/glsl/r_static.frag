#include <i_lighting.glsl>
#include <i_lightmap.glsl>
#include <f_pbr.glsl>
#include <i_envmap.glsl>


uniform highp vec3 u_viewPos;

in vec3 v_fragPos;
in vec2 v_texCoord;
in vec3 v_normal;
in vec4 v_fragPos_dlight[3];
in vec4 v_fragPos_slight[3];

#ifdef DAL_NORMAL_MAPPING
in mat3 v_tbn;
#endif

out vec4 f_color;


vec3 makeNormal() {

#ifdef DAL_NORMAL_MAPPING
    if ( u_hasNormalMap ) {
        vec3 normal = texture(u_normalMap, v_texCoord).rgb;
        normal = normal * 2.0 - 1.0;   
        return normalize(v_tbn * normal); 
    }
    else {
        return v_normal;
    }
#else
    return v_normal;
#endif

}


void main(void) {
    vec3  viewDir    = normalize(u_viewPos - v_fragPos);
    vec3  fragNormal = makeNormal();
    vec4  albedo     = texture(u_diffuseMap, v_texCoord);
    float roughness  = u_hasRoughnessMap ? texture(u_roughnessMap, v_texCoord).r : u_roughness;
    float metallic   = u_hasMetallicMap ? texture(u_metallicMap, v_texCoord).r : u_metallic;
    vec3  ambient    = u_hasEnvmap ? texture(u_irradianceMap, fragNormal).xyz : u_baseAmbient;
    vec3  F0         = mix(vec3(0.04), albedo.rgb, u_metallic);

    vec3 pbrL = vec3(0.0);

    // Env mapping
    {
        vec3 kS = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - dot(fragNormal, viewDir), 5.0);
        pbrL += (1.0 - kS) * ambient * albedo.xyz;
        if ( u_hasEnvmap ) {
            vec3  envSampleVec = calcEnvSampleDirec(u_viewPos, v_fragPos, fragNormal);
            float mip = float(MAX_MIP_LVL) * roughness;
            vec3  radiance = textureLod(u_prefilterMap, envSampleVec, mip).rgb;
            pbrL += kS * radiance;
        }
    }

    for ( int i = 0; i < u_plightCount; ++i ) {
        vec3 radiance = calcRadiance_plight(i, v_fragPos);
        vec3 L        = calcToLight_plight(i, v_fragPos);

        pbrL += integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic, 1.0) * radiance;
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        vec3 radiance   = calcRadiance_slight(i, v_fragPos);
        vec3 L          = calcToLight_slight(i, v_fragPos);
        bool isInShadow = isInShadow_slight(i, v_fragPos_slight[i]);

        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic, 1.0) * radiance;
    }
    for ( int i = 0; i < u_dlightCount; ++i ) {
        vec3 radiance   = calcRadiance_dlight(i);
        vec3 L          = calcToLight_dlight(i);
        bool isInShadow = isInShadow_dlight(i, v_fragPos_dlight[i]);

        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic, 1.0) * radiance;
    }

    f_color.rgb = pbrL;
    f_color.a = albedo.a;
}
