#include <i_lighting_forV.glsl>


layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;
layout (location = 3) in vec3 i_tangent;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

uniform vec4 u_clipPlane;

out vec3 v_fragPos;
out vec2 v_texCoord;
out vec3 v_normal;

#ifdef DAL_SHADOW_ON_WATER_IMAGE
out vec4 v_fragPos_dlight[3];
out vec4 v_fragPos_slight[3];
#endif

#ifdef DAL_ON_WATER_NORMAL_MAPPING
out mat3 v_tbn;
#endif

#ifdef GL_ES
out float v_clipDistance;
#endif


mat3 makeTBN(vec3 normal, mat4 modelMat) {
	vec3 tangentInWorld = normalize(vec3(modelMat * vec4(i_tangent, 0.0)));
	tangentInWorld = normalize(tangentInWorld - dot(tangentInWorld, normal) * normal);
	vec3 bitangent = cross(normal, tangentInWorld);
	return mat3(tangentInWorld, bitangent, normal);
}


void main(void) {
	vec4 worldPos = u_modelMat * vec4(i_position, 1.0);

#ifdef GL_ES
	v_clipDistance = dot(worldPos, u_clipPlane);
#else
	gl_ClipDistance[0] = dot(worldPos, u_clipPlane);
#endif

	gl_Position = u_projMat * u_viewMat * worldPos;
	v_fragPos = vec3(worldPos);
	v_texCoord = i_texCoord;
	v_normal = normalize(vec3(u_modelMat * vec4(i_normal, 0.0)));
#ifdef DAL_ON_WATER_NORMAL_MAPPING
	v_tbn = makeTBN(v_normal, u_modelMat);
#endif

#ifdef DAL_SHADOW_ON_WATER_IMAGE
	for (int i = 0; i < u_dlightCount; i++) {
		v_fragPos_dlight[i] = u_dlight_projViewMat[i] * worldPos;
	}
	for (int i = 0; i < u_slightCount; ++i) {
		v_fragPos_slight[i] = u_slight_projViewMat[i] * worldPos;
	}
#endif

}
