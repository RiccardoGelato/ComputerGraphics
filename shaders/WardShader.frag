#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NLIGHTS 4

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec4 fragTan;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CarUniformBufferObject {
	vec3 lightDir[NLIGHTS];
    vec4 lightColor[NLIGHTS];
    vec3 eyePos;
    vec3 lightPos[NLIGHTS];
    float cosIn;
    float cosOut;
    float lightOn;
    float BDRFState;
} gubo;

layout(set = 1, binding = 1) uniform sampler2D tex;
layout(set = 1, binding = 2) uniform sampler2D spet;

//FUNCTIONS DIFFERENT LIGHTS

vec3 direct_light_dir(vec3 pos, int i) {
	return normalize(gubo.lightDir[i]);
}

vec3 direct_light_color(vec3 pos, int i) {
	return gubo.lightColor[i].rgb;
}

vec3 point_light_dir(vec3 pos, int i) {
	return normalize(gubo.lightPos[i] - pos);
}

vec3 point_light_color(vec3 pos, int i) {
	float division = gubo.lightColor[i].a / length(gubo.lightPos[i] - pos);
	float decay = division * division;
	return decay * gubo.lightColor[i].rgb;
}

vec3 spot_light_dir(vec3 pos, int i) {
	return normalize(gubo.lightPos[i] - pos);
}

vec3 spot_light_color(vec3 pos, int i) {
	float cosa = dot(normalize(gubo.lightPos[i] - pos), gubo.lightDir[i]);
	float division = gubo.lightColor[i].a / length(gubo.lightPos[i] - pos);
    float decay = pow(division, 1.2);
	return decay * gubo.lightColor[i].rgb * gubo.lightColor[i].rgb * clamp((cosa - gubo.cosOut)/(gubo.cosIn - gubo.cosOut), 0 ,1);
}

//BRDF

vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 T, vec3 B, vec3 Md, vec3 Ms, float alphaT, float alphaB) {

	vec3 Diffuse = Md * clamp(dot(N, L),0.0,1.0);
	//vec3 Specular = Ms * vec3(pow(max(dot(Norm, normalize(LD + EyeDir)),0.0), mubo.Pow));
	vec3 H = normalize(L + V);
	float el1 = (dot(H,T)/alphaT) * (dot(H,T)/alphaT);
	float el2 = (dot(H,B)/alphaB) * (dot(H,B)/alphaB);
	float power = -1.0*(el1 + el2)/(dot(H,N) * dot(H,N));
	float numerator = exp(power);
	float PI = 3.1415926;
	float denominator = 4.0 * PI * alphaT * alphaB * sqrt( dot(V, N) / dot( L, N));
	vec3 Specular = Ms * numerator / denominator;

	return clamp((Diffuse + Specular), 0.0, 1.0);
}

void main() {

	    vec3 Norm = normalize(fragNorm);
    	vec3 EyeDir = normalize(gubo.eyePos - fragPos);
    	vec3 Albedo = texture(tex, fragUV).rgb;
    	vec3 specCol = texture(spet, fragUV).rgb;
    	vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm));
        vec3 Bitan = cross(Norm, Tan) * fragTan.w;
    	vec3 RendEqSol = vec3(0);
    	vec3 lightDir;
        vec3 lightColor;

    	lightDir = direct_light_dir(fragPos, 0);
        lightColor = direct_light_color(fragPos, 0);

    	RendEqSol += BRDF(EyeDir, Norm, lightDir, Tan, Bitan, Albedo, specCol, 0.1f, 0.4f) * lightColor;

    	lightDir = spot_light_dir(fragPos, 1);
        lightColor = spot_light_color(fragPos, 1);

        RendEqSol += BRDF(EyeDir, Norm, lightDir, Tan, Bitan, Albedo, specCol, 0.1f, 0.4f) * lightColor * gubo.lightOn;

        lightDir = spot_light_dir(fragPos, 2);
        lightColor = spot_light_color(fragPos, 2);

        RendEqSol += BRDF(EyeDir, Norm, lightDir, Tan, Bitan, Albedo, specCol, 0.1f, 0.4f) * lightColor * gubo.lightOn;

        lightDir = direct_light_dir(fragPos, 3);
        lightColor = direct_light_color(fragPos, 3);

        RendEqSol += BRDF(EyeDir, Norm, lightDir, Tan, Bitan, Albedo, specCol, 0.1f, 0.4f) * lightColor;

    	vec3 Ambient = texture(tex, fragUV).rgb * 0.025f;
    	/*const vec3 cxp = vec3(1.0,0.0,0.0) * 0.025f;
        const vec3 cxn = vec3(0.1,0.0,0.0) * 0.025f;
        const vec3 cyp = vec3(0.0,1.0,0.0) * 0.025f;
        const vec3 cyn = vec3(0.0,0.1,0.0) * 0.025f;
        const vec3 czp = vec3(0.0,0.0,0.1) * 0.025f;
        const vec3 czn = vec3(0.0,0.0,0.1) * 0.025f;

        Ambient =((Norm.x > 0 ? cxp : cxn) * (Norm.x * Norm.x) +
                  (Norm.y > 0 ? cyp : cyn) * (Norm.y * Norm.y) +
                  (Norm.z > 0 ? czp : czn) * (Norm.z * Norm.z)) * Albedo;
*/
    	vec3 col = RendEqSol + Ambient;
    	
    	outColor = vec4(col, 1.0f);
}