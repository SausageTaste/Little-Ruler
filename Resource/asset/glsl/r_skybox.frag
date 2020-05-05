#include <i_lighting.glsl>
#include <f_scattering.glsl>
#include <m_geometry.glsl>


uniform vec3 u_viewPos;
uniform vec3 u_viewPosActual;
uniform samplerCube u_skyboxTex;

uniform vec3 u_dlight_direc;


in vec3 v_worldPos;

out vec4 f_color;


void main() {
    vec3 texCoord = normalize(v_worldPos - u_viewPos);
    vec3 texColor = texture(u_skyboxTex, texCoord).xyz;
    f_color = vec4(texColor, 1.0);

#ifdef DAL_VOLUMETRIC_LIGHT
    for ( int i = 0; i < u_dlightCount; ++i ) {
        f_color.xyz += calcScatterColor_dlight(i, u_viewPosActual + texCoord * 30.0, u_viewPosActual);
    }
#endif

    Segment seg;
    seg.m_pos = u_viewPos;
    seg.m_rel = normalize(v_worldPos) * 50.0;

    Sphere atmos;
    atmos.m_center = vec3(u_viewPos.x, -RADIUS_EARTH, u_viewPos.z);
    atmos.m_radius = RADIUS_ATMOS;

    vec4 intersection = intersect_seg_sphere(seg, atmos);
    f_color.xyz = skyColor(u_viewPos, intersection.xyz, u_dlight_direc);
}
