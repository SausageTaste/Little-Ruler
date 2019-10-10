

uniform vec4 uColor;

uniform sampler2D mDiffuseMap;  // 0
uniform bool mHasDiffuseMap;

uniform sampler2D mMaskMap;  // 1
uniform bool mHasMaskMap;


in vec2 vTexCoord;
in vec2 vTexCoord_maskMap;

out vec4 fColor;


void main() {
    vec4 color = vec4(0.0);
    if (mHasDiffuseMap) {
        color = texture(mDiffuseMap, vTexCoord);
    }
    else {
        color = uColor;
    }

    if (mHasMaskMap) {
        fColor.xyz = color.xyz;
        fColor.a = texture(mMaskMap, vTexCoord_maskMap).r;
    }
    else {
        fColor.xyzw = color.xyzw;
    }
}