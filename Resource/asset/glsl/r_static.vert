#include <i_lighting_forV.glsl>


layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

out vec3 v_fragPos;
out vec2 v_texCoord;
out vec3 v_normal;
out vec4 v_fragPos_dlight[3];
out vec4 v_fragPos_slight[3];


void main(void) {
	vec4 worldPos = u_modelMat * vec4(i_position, 1.0);

	gl_Position = u_projMat * u_viewMat * worldPos;
	v_fragPos = vec3(worldPos);
	v_texCoord = i_texCoord;
	v_normal = normalize(vec3(u_modelMat * vec4(i_normal, 0.0)));

	for (int i = 0; i < u_dlightCount; i++) {
		v_fragPos_dlight[i] = u_dlight_projViewMat[i] * worldPos;
	}
	for (int i = 0; i < u_slightCount; ++i) {
		v_fragPos_slight[i] = u_slight_projViewMat[i] * worldPos;
	}
}
