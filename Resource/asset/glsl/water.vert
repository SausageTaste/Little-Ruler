// Interf - Geometry
layout (location = 0) in vec3 i_position;

layout (location = 1) in vec2 iTexCoord;
layout (location = 2) in vec3 iNormal;

// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

uniform highp int uDlightCount;
uniform highp vec3 uViewPos;

uniform mat4 uDlightProjViewMat[3];

uniform float uTexScaleX;
uniform float uTexScaleY;


out vec3 vFragPos;
out vec2 vTexCoord;
out vec3 vNormalVec;
out vec4 vFragPosInDlight[3];
out vec4 v_clipSpace;
out vec3 v_toCamera;


void main(void) {
	vec4 worldPos = u_modelMat * vec4(i_position, 1.0);
	v_clipSpace = u_projMat * u_viewMat * worldPos;
	gl_Position = v_clipSpace;
	vFragPos = vec3(worldPos);
	vTexCoord = vec2(iTexCoord.x * uTexScaleX, -iTexCoord.y * uTexScaleY);
	vNormalVec = normalize(vec3(u_modelMat * vec4(iNormal, 0.0)));
	v_toCamera = uViewPos - worldPos.xyz;

	for (int i = 0; i < uDlightCount; i++) {
		vFragPosInDlight[i] = uDlightProjViewMat[i] * vec4(vFragPos, 1.0);
	}
}