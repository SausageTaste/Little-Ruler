#include <i_lighting_forV.glsl>
#include <i_skeleton.glsl>


layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;
layout (location = 3) in ivec3 i_jointIDs;
layout (location = 4) in vec3 i_weights;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

uniform vec4 u_clipPlane;

out vec3 v_fragPos;
out vec2 v_texCoord;
out vec3 v_normal;
out vec4 v_fragPos_dlight[3];
out vec4 v_fragPos_slight[3];

#ifdef GL_ES
out float v_clipDistance;
#endif


void main(void) {
	mat4 boneMat = makeJointTransform(i_jointIDs, i_weights);
	vec4 worldPos = u_modelMat * boneMat * vec4(i_position, 1.0);

#ifdef GL_ES
	v_clipDistance = dot(worldPos, u_clipPlane);
#else
	gl_ClipDistance[0] = dot(worldPos, u_clipPlane);
#endif

	gl_Position = u_projMat * u_viewMat * worldPos;
	v_fragPos = vec3(worldPos);
	v_texCoord = i_texCoord;
	v_normal = normalize(vec3(u_modelMat * boneMat * vec4(i_normal, 0.0)));

	for (int i = 0; i < u_dlightCount; i++) {
		v_fragPos_dlight[i] = u_dlight_projViewMat[i] * worldPos;
	}
	for (int i = 0; i < u_slightCount; ++i) {
		v_fragPos_slight[i] = u_slight_projViewMat[i] * worldPos;
	}
}
