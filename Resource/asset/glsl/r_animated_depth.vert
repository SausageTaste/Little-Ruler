#include <i_skeleton.glsl>


layout (location = 0) in vec3 i_position;
layout (location = 4) in ivec3 i_jointIDs;
layout (location = 5) in vec3 i_weights;


uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;


void main(void) {
	mat4 boneMat = makeJointTransform(i_jointIDs, i_weights);
	gl_Position = u_projMat * u_viewMat * u_modelMat * boneMat * vec4(i_position, 1.0);
}
