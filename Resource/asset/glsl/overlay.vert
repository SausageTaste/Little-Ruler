

layout (location = 0) in vec2 iPosition;


uniform vec2 uPoint1;
uniform vec2 uPoint2;

uniform bool mUpsideDown_maskMap;
uniform bool m_upsideDown_diffuseMap;

uniform vec2 u_texOffset;
uniform vec2 u_texScale;

out vec2 vTexCoord;
out vec2 vTexCoord_maskMap;


void main(void) {
    vTexCoord = iPosition;
    if (mUpsideDown_maskMap) {
        vTexCoord_maskMap = vec2( iPosition.x, 1.0 - iPosition.y );
    }
    else {
        vTexCoord_maskMap = iPosition;
    }

    if (m_upsideDown_diffuseMap) {
        vTexCoord = vec2( iPosition.x, 1.0 - iPosition.y );
    }
    else {
        vTexCoord = iPosition;
    }

    vTexCoord = vTexCoord * u_texScale + u_texOffset;
    vTexCoord_maskMap = vTexCoord_maskMap * u_texScale + u_texOffset;

    gl_Position = vec4( (uPoint1 + iPosition * uPoint2), 0.0, 1.0 );
}
