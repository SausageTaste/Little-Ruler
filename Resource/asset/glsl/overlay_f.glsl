#ifdef GL_ES
precision mediump float;
#endif


uniform vec4 uColor;

uniform sampler2D mDiffuseMap;  // 0
uniform bool mHasDiffuseMap;

uniform sampler2D mMaskMap;  // 1
uniform bool mHasMaskMap;


in vec2 vTexCoord;
in vec2 vTexCoord_maskMap;

out vec4 fColor;


void main() {
    if (mHasDiffuseMap) {
        fColor = texture(mDiffuseMap, vTexCoord);
    }
    else {
        fColor = uColor;
    }

    if (mHasMaskMap) {
        fColor.a = texture(mMaskMap, vTexCoord_maskMap).r;
    }
}