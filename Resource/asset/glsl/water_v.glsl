

layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec2 iTexCoord;
layout (location = 2) in vec3 iNormal;

uniform mat4 uProjectMat;
uniform mat4 uViewMat;
uniform mat4 uModelMat;

// From Master
uniform highp int uDlightCount;
uniform mediump vec3 uViewPos;

// From Light
uniform mat4 uDlightProjViewMat[3];

// From Material
uniform float uTexScaleX;
uniform float uTexScaleY;


out vec3 vFragPos;
out vec2 vTexCoord;
out vec3 vNormalVec;
out vec4 vFragPosInDlight[3];
out vec4 v_clipSpace;
out vec3 v_toCamera;


void main(void) {
	vec4 worldPos = uModelMat * vec4(iPosition, 1.0);
	v_clipSpace = uProjectMat * uViewMat * worldPos;
	gl_Position = v_clipSpace;
	vFragPos = vec3(uModelMat * vec4(iPosition, 1.0));
	vTexCoord = vec2(iTexCoord.x * uTexScaleX, -iTexCoord.y * uTexScaleY);
	vNormalVec = normalize(vec3(uModelMat * vec4(iNormal, 0.0)));
	v_toCamera = uViewPos - worldPos.xyz;

	for (int i = 0; i < uDlightCount; i++) {
		vFragPosInDlight[i] = uDlightProjViewMat[i] * vec4(vFragPos, 1.0);
	}
}