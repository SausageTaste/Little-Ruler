// Interf - Geometry
layout (location = 0) in vec3 i_position;

layout (location = 1) in vec2 iTexCoord;
layout (location = 2) in vec3 iNormal;

// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

uniform highp int uDlightCount;
uniform bool u_doClip;
uniform vec4 u_clipPlane;

uniform mat4 uDlightProjViewMat[3];

uniform float uTexScaleX;
uniform float uTexScaleY;


out vec3 vFragPos;
out vec2 vTexCoord;
out vec3 vNormalVec;
out vec4 vFragPosInDlight[3];


void main(void) {
	vec4 worldPos = u_modelMat * vec4(i_position, 1.0);

#ifndef GL_ES
	if (u_doClip) {
		gl_ClipDistance[0] = dot(worldPos, u_clipPlane);
	}
#endif

	gl_Position = u_projMat * u_viewMat * worldPos;
	vFragPos = vec3(worldPos);
	vTexCoord = vec2(iTexCoord.x * uTexScaleX, -iTexCoord.y * uTexScaleY);
	vNormalVec = normalize(vec3(u_modelMat * vec4(iNormal, 0.0)));

	for (int i = 0; i < uDlightCount; i++) {
		vFragPosInDlight[i] = uDlightProjViewMat[i] * worldPos;
	}
}