uniform sampler2D uTexture;


in vec2 v_texCoord;

out vec4 f_color;


void main() { 
	f_color = texture(uTexture, v_texCoord);
}
