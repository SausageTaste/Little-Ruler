

// From Master
uniform vec3 uViewPos;
uniform vec3 uBaseAmbient;
uniform highp int uDlightCount;
uniform int uPlightCount;

uniform bool u_doClip;
uniform vec4 u_clipPlane;

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
in vec4 v_worldPos;

out vec4 fColor;


vec3 procDlight(int index, vec3 viewDir) {
	vec3 lightLocDir = normalize(-uDlightDirecs[index]);
	vec3 lightColor = uDlightColors[index];

	float diff = max(dot(vNormalVec, lightLocDir), 0.0);
	vec3 diffuse = max(diff * lightColor, vec3(0.0));

	// Calculate specular lighting.
	if (uShininess <= 0.0)
	{
		return diffuse;
	}
	else
	{
		vec3 reflectDir = reflect(-lightLocDir, vNormalVec);
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
#ifdef GL_ES
	if (u_doClip) {
		if ( dot(v_worldPos, u_clipPlane) < 0.0 ) discard;
	}
#endif

	vec3 viewDir = normalize(uViewPos - vFragPos);
	vec3 lightedColor = uBaseAmbient;
	vec3 fragNormal = vNormalVec;

	int i;
	for (i = 0; i < uDlightCount; i++) {
		lightedColor += max(procDlight(i, viewDir) * calcShadowDlight(i), vec3(0.0));
	}
	for (i = 0; i < uPlightCount; i++) {
		lightedColor += max(procPlight(i, viewDir, fragNormal), vec3(0.0));
	}

	if (uHasDiffuseMap != 0) {
		vec4 texColor = texture(uDiffuseMap, vTexCoord);
		if (texColor.a == 0.0) discard;
		fColor = texColor * vec4(lightedColor, 1.0);
	}
	else {
		fColor = vec4(uDiffuseColor, 1.0) * vec4(lightedColor, 1.0);
	}
}