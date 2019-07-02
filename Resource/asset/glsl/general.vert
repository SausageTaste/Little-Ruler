// Interf - Geometry
layout (location = 0) in vec3 i_position;

// Interf - Mesh
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;


// Interf - Geometry
uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

// Interf - Mesh
uniform vec2 u_texScale;

uniform highp int uDlightCount;
uniform bool u_doClip;
uniform vec4 u_clipPlane;

uniform mat4 uDlightProjViewMat[3];


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
	vTexCoord = vec2(i_texCoord.x * u_texScale.x, -i_texCoord.y * u_texScale.y);
	vNormalVec = normalize(vec3(u_modelMat * vec4(i_normal, 0.0)));

	for (int i = 0; i < uDlightCount; i++) {
		vFragPosInDlight[i] = uDlightProjViewMat[i] * worldPos;
	}
}