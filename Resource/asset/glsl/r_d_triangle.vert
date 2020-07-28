
uniform vec3 u_p0;
uniform vec3 u_p1;
uniform vec3 u_p2;

uniform mat4 u_mat;


void main() {
    switch ( gl_VertexID % 3 ) {

    case 0:
        gl_Position = u_mat * vec4(u_p0, 1.0);
        break;
    case 1:
        gl_Position = u_mat * vec4(u_p1, 1.0);
        break;
    case 2:
        gl_Position = u_mat * vec4(u_p2, 1.0);
        break;

    }
}
