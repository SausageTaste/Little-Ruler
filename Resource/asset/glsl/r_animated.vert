#include <i_lighting_forV.glsl>
#include <i_skeleton.glsl>


layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;
layout (location = 3) in vec3 i_tangent;
layout (location = 4) in ivec3 i_jointIDs;
layout (location = 5) in vec3 i_weights;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;


out vec3 v_fragPos;
out vec2 v_texCoord;
out vec3 v_normal;
out vec4 v_fragPos_dlight[3];
out vec4 v_fragPos_slight[3];

#ifdef DAL_NORMAL_MAPPING
out mat3 v_tbn;
#endif


mat3 makeTBN(vec3 normal, mat4 modelMat) {
	vec3 tangentInWorld = normalize(vec3(modelMat * vec4(i_tangent, 0.0)));
	tangentInWorld = normalize(tangentInWorld - dot(tangentInWorld, normal) * normal);
	vec3 bitangent = cross(normal, tangentInWorld);
	return mat3(tangentInWorld, bitangent, normal);
}


void main(void) {
	mat4 modelJointMat = u_modelMat * makeJointTransform(i_jointIDs, i_weights);
	vec4 worldPos = modelJointMat * vec4(i_position, 1.0);

	gl_Position = u_projMat * u_viewMat * worldPos;
	v_fragPos = vec3(worldPos);
	v_texCoord = i_texCoord;
	v_normal = normalize(vec3(modelJointMat * vec4(i_normal, 0.0)));
#ifdef DAL_NORMAL_MAPPING
	v_tbn = makeTBN(v_normal, modelJointMat);
#endif

	for (int i = 0; i < u_dlightCount; i++) {
		v_fragPos_dlight[i] = u_dlight_projViewMat[i] * worldPos;
	}
	for (int i = 0; i < u_slightCount; ++i) {
		v_fragPos_slight[i] = u_slight_projViewMat[i] * worldPos;
	}
}
