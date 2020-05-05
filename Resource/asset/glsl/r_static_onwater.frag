#include <i_lighting.glsl>
#include <i_lightmap.glsl>
#include <f_pbr.glsl>


uniform highp vec3 u_viewPos;

in vec3 v_fragPos;
in vec2 v_texCoord;
in vec3 v_normal;

#ifdef DAL_SHADOW_ON_WATER_IMAGE
in vec4 v_fragPos_dlight[3];
in vec4 v_fragPos_slight[3];
#endif

#ifdef DAL_ON_WATER_NORMAL_MAPPING
in mat3 v_tbn;
#endif

#ifdef GL_ES
in float v_clipDistance;
#endif

out vec4 f_color;


vec3 makeNormal() {

#ifdef DAL_ON_WATER_NORMAL_MAPPING
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
#ifdef GL_ES
    if ( v_clipDistance < 0.0 ) {
        discard;
    }
#endif

    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec3 fragNormal = makeNormal();

    vec4 albedo = texture(u_diffuseMap, v_texCoord);
    float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, v_texCoord).r : u_roughness;
    float metallic = u_hasMetallicMap ? texture(u_metallicMap, v_texCoord).r : u_metallic;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, u_metallic);
    vec3 pbrL = u_baseAmbient * albedo.rgb;

#ifdef DAL_ON_WATER_POSITION_LIGHT
    for ( int i = 0; i < u_plightCount; ++i ) {
        vec3 radiance = calcRadiance_plight(i, v_fragPos);
        vec3 L        = calcToLight_plight(i, v_fragPos);

        pbrL += integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        vec3 radiance   = calcRadiance_slight(i, v_fragPos);
        vec3 L          = calcToLight_slight(i, v_fragPos);

# ifdef DAL_SHADOW_ON_WATER_IMAGE
        bool isInShadow = isInShadow_slight(i, v_fragPos_slight[i]);
        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
# else
        pbrL += integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
# endif
    }
#endif

    for ( int i = 0; i < u_dlightCount; ++i ) {
        vec3 radiance   = calcRadiance_dlight(i);
        vec3 L          = calcToLight_dlight(i);

#ifdef DAL_SHADOW_ON_WATER_IMAGE
        bool isInShadow = isInShadow_dlight(i, v_fragPos_dlight[i]);
        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
#else
        pbrL += integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
#endif

    }

    f_color.rgb = pbrL;
    f_color.a = albedo.a;
}
