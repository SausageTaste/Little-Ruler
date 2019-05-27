#include <inter_lighting.frag>


uniform sampler2D u_bansaTex;  // TEX 4
uniform sampler2D u_gooljulTex;  // TEX 5
uniform sampler2D u_dudvMap;  // TEX 6
uniform float u_dudvMoveFactor;
uniform sampler2D u_normalMap;  // TEX 7

// From Material
uniform vec3 uDiffuseColor;

// From Texture
uniform int uHasDiffuseMap;
uniform sampler2D uDiffuseMap;  // TEX 0


in vec3 vFragPos;
in vec2 vTexCoord;
in vec3 vNormalVec;
in vec4 vFragPosInDlight[3];
in vec4 v_clipSpace;
in vec3 v_toCamera;

out vec4 fColor;


const float gk_waveStren = 0.01;


vec2 getDistortedCoords() {
	vec2 distoredTexCoords = texture(u_dudvMap, vec2(vTexCoord.x + u_dudvMoveFactor, vTexCoord.y)).rg * 0.1;
	return vTexCoord + vec2(distoredTexCoords.x, distoredTexCoords.y + u_dudvMoveFactor);
}


vec3 makeFragNormal(vec2 distortedCoords) {
	vec4 normalColor = texture(u_normalMap, distortedCoords);
	return normalize(vec3(
		normalColor.r * 2.0 - 1.0,
		normalColor.b * 6.0,
		normalColor.g * 2.0 - 1.0
	));
}


vec4 calculateWater(vec3 fragNormal, vec2 distortedCoords) {
	vec2 normalizedDeviceCoord = (v_clipSpace.xy / v_clipSpace.w) / 2.0 + 0.5;
	vec2 bansaCoord = vec2(normalizedDeviceCoord.x, -normalizedDeviceCoord.y);
	vec2 gooljulCoord = vec2(normalizedDeviceCoord.x, normalizedDeviceCoord.y);

	vec2 totalDistortion = (texture(u_dudvMap, distortedCoords).rg * 2.0 - 1.0) * gk_waveStren;

	bansaCoord += totalDistortion;
	gooljulCoord += totalDistortion;
	
	gooljulCoord = clamp(gooljulCoord, 0.001, 0.999);
	bansaCoord.x = clamp(bansaCoord.x, 0.001, 0.999);
	bansaCoord.y = clamp(bansaCoord.y, -0.999, -0.001);

	vec4 bansaColor = texture(u_bansaTex, bansaCoord);
	vec4 gooljulColor = texture(u_gooljulTex, gooljulCoord);

	vec3 viewVec = normalize(v_toCamera);
	float refractiveFactor = pow(dot(viewVec, vec3(0.0, 1.0, 0.0)), 0.8);

	return mix(bansaColor, gooljulColor, refractiveFactor);
}


void main(void) {
	// Needed values
	vec3 viewDir = normalize(uViewPos - vFragPos);
	vec2 distoredTexCoords = getDistortedCoords();
	vec3 fragNormal = makeFragNormal(distoredTexCoords);

	// Lighting
	int i;
	vec3 lightedColor = uBaseAmbient;
	for (i = 0; i < uDlightCount; i++) {
		lightedColor += getDlightFactor(i, viewDir, fragNormal, vFragPosInDlight[i]) * uDlightColors[i];
	}
	for (i = 0; i < uPlightCount; i++) {
		lightedColor += getLightFactor_point(i, viewDir, fragNormal, vFragPos) * uPlightColors[i];
	}

	// Water
	vec4 waterImage = calculateWater(fragNormal, distoredTexCoords);

	// Final color
	fColor = 0.5 * waterImage * (vec4(lightedColor, 1.0) + 1.0);
	//fColor += vec4(0.05, 0.05, 0.1, 0.0);
}