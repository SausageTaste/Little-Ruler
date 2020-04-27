#include <i_lighting_forV.glsl>


layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec2 i_normal;


uniform highp vec3 u_viewPos;
uniform highp mat4 u_projMat;
uniform highp mat4 u_viewMat;

uniform vec2 u_texScale;


out vec3 v_fragPos;
out vec2 v_texCoord;
out vec4 v_fragPos_dlight[3];
out vec4 v_fragPos_slight[3];
out vec4 v_clipSpace;
out vec3 v_toCamera;


void main(void) {
	vec4 worldPos = vec4(i_position, 1.0);
	v_clipSpace = u_projMat * u_viewMat * worldPos;
	gl_Position = v_clipSpace;
	v_fragPos = vec3(worldPos);
	v_texCoord = vec2(i_texCoord.x * u_texScale.x, i_texCoord.y * u_texScale.y);
	v_toCamera = u_viewPos - worldPos.xyz;

	for ( int i = 0; i < u_dlightCount; ++i ) {
		v_fragPos_dlight[i] = u_dlight_projViewMat[i] * worldPos;
	}
	for ( int i = 0; i < u_slightCount; ++i ) {
		v_fragPos_slight[i] = u_slight_projViewMat[i] * worldPos;
	}
}