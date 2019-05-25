#include <inter_general.frag>


// From Master
uniform highp vec3 uViewPos;
uniform vec3 uBaseAmbient;
uniform highp int uDlightCount;
uniform int uPlightCount;

uniform sampler2D u_bansaTex;  // TEX 4
uniform sampler2D u_gooljulTex;  // TEX 5
uniform sampler2D u_dudvMap;  // TEX 6
uniform float u_dudvMoveFactor;
uniform sampler2D u_normalMap;  // TEX 7

// From Material
uniform vec3 uDiffuseColor;
uniform float uShininess;
uniform float uSpecularStrength;

// From Texture
uniform int uHasDiffuseMap;
uniform sampler2D uDiffuseMap;  // TEX 0

// From DirectionalLight
uniform vec3      uDlightDirecs[3];
uniform vec3      uDlightColors[3];
uniform sampler2D uDlightDepthMap[3];  // TEX 1, 2, 3

// From PointLight
uniform vec3 uPlightPoses[3];
uniform vec3 uPlightColors[3];
uniform float uPlightMaxDists[3];


in vec3 vFragPos;
in vec2 vTexCoord;
in vec3 vNormalVec;
in vec4 vFragPosInDlight[3];
in vec4 v_clipSpace;
in vec3 v_toCamera;

out vec4 fColor;


const float waveStren = 0.01;


vec3 procDlight(int index, vec3 viewDir, vec3 fragNormal) {
	vec3 lightLocDir = normalize(-uDlightDirecs[index]);
	vec3 lightColor = uDlightColors[index];

	float diff = max(dot(fragNormal, lightLocDir), 0.0);
	vec3 diffuse = max(diff * lightColor, vec3(0.0));

	// Calculate specular lighting.
	if (uShininess <= 0.0)
	{
		return diffuse;
	}
	else
	{
		vec3 reflectDir = reflect(-lightLocDir, fragNormal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
		vec3 specular = max(uSpecularStrength * spec * lightColor, vec3(0.0));

		return diffuse + specular;
	}
}


#ifdef GL_ES
float getShadowMapValue(int index, vec2 coord) {
	switch (index){
		case 0: return texture(uDlightDepthMap[0], coord).r;
		case 1: return texture(uDlightDepthMap[1], coord).r;
		case 2: return texture(uDlightDepthMap[2], coord).r;
		default: return 1.0;
	}
/*
	if (index == 0) {
		closestDepth = texture(uDlightDepthMap[0], projCoords.xy).r;
	}
	else if (index == 1) {
		closestDepth = texture(uDlightDepthMap[1], projCoords.xy).r;
	}
	else if (index == 2) {
		closestDepth = texture(uDlightDepthMap[2], projCoords.xy).r;
	}
	else {
		return 1.0;
	}
	*/
}
#endif


float calcShadowDlight(int index) {
	vec4 fragPosInLightSpace = vFragPosInDlight[index];
	vec3 projCoords = fragPosInLightSpace.xyz / fragPosInLightSpace.w;
	if (projCoords.z > 1.0) return 1.0;

	projCoords = projCoords * 0.5 + 0.5;
	if (projCoords.x > 1.0 || projCoords.x < 0.0) return 1.0;
	if (projCoords.y > 1.0 || projCoords.y < 0.0) return 1.0;

	float closestDepth;
#ifdef GL_ES
	closestDepth = getShadowMapValue(index, projCoords.xy);
#else
	closestDepth = texture(uDlightDepthMap[index], projCoords.xy).r;
#endif

	float currentDepth = projCoords.z;

	//float bias = max(0.05 * (1.0 - dot(vNormalVec, -uDlightDirs[index])), 0.005);
	float bias = 0.002;
	//float bias = 0.0;
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

	return 1.0 - shadow;
}


vec3 procPlight(int index, vec3 viewDir, vec3 fragNormal) {
	float distance_f = length(uPlightPoses[index] - vFragPos);
	vec3 lightDir = normalize(uPlightPoses[index] - vFragPos);
	float distanceDecreaser = -1.0 / (uPlightMaxDists[index]*uPlightMaxDists[index]) * (distance_f*distance_f) + 1.0;

	vec3 diffuse;
	if (distance_f > uPlightMaxDists[index]) {
		diffuse = vec3(0.0);
	}
	else {
		float diff_f = max(dot(fragNormal, lightDir), 0.0);
		diffuse = max(diff_f * uPlightColors[index], vec3(0.0));
	}

	// Calculate specular lighting.
	if (uShininess == 0.0) {
		return diffuse * distanceDecreaser;
	}
	else {
		vec3 reflectDir = reflect(-lightDir, fragNormal);

		//vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn
		//float spec = pow(max(dot(fragNormal, halfwayDir), 0.0), 64.0); // Blinn
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess); // Phong

		vec3 specular = max(uSpecularStrength * spec * uPlightColors[index], vec3(0.0));

		return diffuse * distanceDecreaser + specular * distanceDecreaser;
	}
}


void main(void) {
	vec3 viewDir = normalize(uViewPos - vFragPos);

	vec2 normalizedDeviceCoord = (v_clipSpace.xy / v_clipSpace.w) / 2.0 + 0.5;
	vec2 bansaCoord = vec2(normalizedDeviceCoord.x, -normalizedDeviceCoord.y);
	vec2 gooljulCoord = vec2(normalizedDeviceCoord.x, normalizedDeviceCoord.y);

	vec2 distoredTexCoords = texture(u_dudvMap, vec2(vTexCoord.x + u_dudvMoveFactor, vTexCoord.y)).rg * 0.1;
	distoredTexCoords = vTexCoord + vec2(distoredTexCoords.x, distoredTexCoords.y + u_dudvMoveFactor);
	vec2 totalDistortion = (texture(u_dudvMap, distoredTexCoords).rg * 2.0 - 1.0) * waveStren;

	bansaCoord += totalDistortion;
	gooljulCoord += totalDistortion;
	
	gooljulCoord = clamp(gooljulCoord, 0.001, 0.999);
	bansaCoord.x = clamp(bansaCoord.x, 0.001, 0.999);
	bansaCoord.y = clamp(bansaCoord.y, -0.999, -0.001);

	vec4 bansaColor = texture(u_bansaTex, bansaCoord);
	vec4 gooljulColor = texture(u_gooljulTex, gooljulCoord);

	vec4 normalColor = texture(u_normalMap, distoredTexCoords);
	vec3 texNormal = normalize(vec3(
		normalColor.r * 2.0 - 1.0,
		normalColor.b * 6.0,
		normalColor.g * 2.0 - 1.0
	));

	vec3 lightedColor = uBaseAmbient;
	vec3 fragNormal = texNormal;

	int i;
	for (i = 0; i < uDlightCount; i++) {
		lightedColor += max(procDlight(i, viewDir, fragNormal) * calcShadowDlight(i), vec3(0.0));
	}
	for (i = 0; i < uPlightCount; i++) {
		lightedColor += max(procPlight(i, viewDir, fragNormal), vec3(0.0));
	}

	vec3 viewVec = normalize(v_toCamera);
	float refractiveFactor = pow(dot(viewVec, vec3(0.0, 1.0, 0.0)), 0.8);

	vec4 waterImage = mix(bansaColor, gooljulColor, refractiveFactor);
	
	fColor = getHalf() * waterImage * (vec4(lightedColor, 1.0) + 1.0);
	fColor += vec4(0.05, 0.05, 0.1, 0.0);
}