#ifdef GL_ES
precision mediump float;
#endif


uniform sampler2D uTexture;

in vec2 vTexCoord;

out vec4 fColor;

void main() { 
	fColor = texture(uTexture, vTexCoord);
}