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

out vec4 f_color;


void main(void) {
    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec3 fragNormal = normalize(v_normal);

    vec4 albedo = texture(u_diffuseMap, v_texCoord);
    float roughness = u_hasRoughnessMap ? texture(u_roughnessMap, v_texCoord).r : u_roughness;
    float metallic = u_hasMetallicMap ? texture(u_metallicMap, v_texCoord).r : u_metallic;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, u_metallic);
    vec3 pbrL = u_baseAmbient * albedo.rgb;

    for ( int i = 0; i < u_plightCount; ++i ) {
        vec3 radiance = calcRadiance_plight(i, v_fragPos);
        vec3 L        = calcToLight_plight(i, v_fragPos);

        pbrL += integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
    }
    for ( int i = 0; i < u_slightCount; ++i ) {
        vec3 radiance   = calcRadiance_slight(i, v_fragPos);
        vec3 L          = calcToLight_slight(i, v_fragPos);
        bool isInShadow = isInShadow_slight(i, v_fragPos_slight[i]);

        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
    }
    for ( int i = 0; i < u_dlightCount; ++i ) {
        vec3 radiance   = calcRadiance_dlight(i);
        vec3 L          = calcToLight_dlight(i);
        bool isInShadow = isInShadow_dlight(i, v_fragPos_dlight[i]);

        pbrL += isInShadow ? vec3(0.0) : integratePBR(fragNormal, viewDir, F0, L, albedo.rgb, roughness, metallic) * radiance;
    }
    f_color.rgb = pbrL;
    f_color.a = albedo.a;

    if ( u_hasEnvmap ) {
        vec3 envcolor = getEnvColor(u_viewPos, v_fragPos, fragNormal);
        f_color.xyz = mix(envcolor, f_color.xyz, roughness * 0.5 + 0.5);
    }
}
