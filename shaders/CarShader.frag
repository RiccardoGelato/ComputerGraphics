#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NLIGHTS 4

// this defines the variable received from the Vertex Shader
// the locations must match the one of its out variables
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

// This defines the color computed by this shader. Generally is always location 0.
layout(location = 0) out vec4 outColor;

// Here the Uniform buffers are defined. In this case, the Global Uniforms of Set 0
// The texture of Set 1 (binding 1), and the Material parameters (Set 1, binding 2)
// are used. Note that each definition must match the one used in the CPP code
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

layout(set = 1, binding = 2) uniform CarParUniformBufferObject {
	float Pow;
} mubo;

layout(set = 1, binding = 1) uniform sampler2D tex;

//FUNCTIONS DIFFERENT LIGHTS

vec3 direct_light_dir(vec3 pos, int i) {
	return normalize(gubo.lightDir[0]);
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

vec3 BRDFNormal(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {
// Compute the BRDF, with a given color <Albedo>, in a given position characterized bu a given normal vector <Norm>,
// for a light direct according to <LD>, and viewed from a direction <EyeDir>
	vec3 Diffuse;
	vec3 Specular;
	Diffuse = Albedo * max(dot(Norm, LD),0.0f);
	Specular = vec3(pow(max(dot(EyeDir, -reflect(LD, Norm)),0.0f), 160.0f));

	return Diffuse + Specular;
}

vec3 BRDFToon(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {

    vec3 Ms = vec3(1.0f);

    float cosa = dot(Norm, LD);

    float cosb = dot(EyeDir, -reflect(LD, Norm));

    //vec3 Diffuse = Albedo * clamp(dot(Norm, LD),0.0,1.0);
    vec3 Diffuse = Albedo;
    //vec3 Specular = Ms * vec3(pow(clamp(dot(EyeDir, -reflect(LD, Norm)),0.0,1.0), 200.0f));
    vec3 Specular = Ms;

    if(cosa <= 0){
        Diffuse = Diffuse * 0;
    }else if(cosa <= 0.1){
        Diffuse = Diffuse * (cosa / 0.1) * 0.15;
    }else if(cosa <= 0.7){
        Diffuse = Diffuse * 0.15;
    }else if(cosa <= 0.8){
        Diffuse = Diffuse * (((cosa - 0.7) / 0.1) * (1-0.15) + 0.15);
    }else{
        Diffuse = Diffuse;
    }

    if(cosb <= 0.9){
        Specular = Specular * 0;
    }else if(cosb <= 0.95){
        Specular = Specular * (((cosb - 0.9) / 0.05) * 1);
    }else{
        Specular = Specular;
    }

	return (Diffuse + Specular);
}

vec3 BRDF(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {
    vec3 result;
    if(gubo.BDRFState == 1){
        result = BRDFNormal(Albedo, Norm, EyeDir, LD);
    }else{
        result = BRDFToon(Albedo, Norm, EyeDir, LD);
    }
    return result;
}

// The main shader, implementing a simple Blinn + Lambert + constant Ambient BRDF model
// The scene is lit by a single Spot Light
void main() {
	vec3 Norm = normalize(fragNorm);
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);
	vec3 Albedo = texture(tex, fragUV).rgb;
	vec3 RendEqSol = vec3(0);
	vec3 lightDir;
    vec3 lightColor;

	lightDir = direct_light_dir(fragPos, 0);
    lightColor = direct_light_color(fragPos, 0);

	RendEqSol += BRDF(Albedo, Norm, EyeDir, lightDir) * lightColor;

	lightDir = spot_light_dir(fragPos, 1);
    lightColor = spot_light_color(fragPos, 1);

    RendEqSol += BRDF(Albedo, Norm, EyeDir, lightDir) * lightColor * gubo.lightOn;

    lightDir = spot_light_dir(fragPos, 2);
    lightColor = spot_light_color(fragPos, 2);

    RendEqSol += BRDF(Albedo, Norm, EyeDir, lightDir) * lightColor * gubo.lightOn;

    lightDir = direct_light_dir(fragPos, 3);
    lightColor = direct_light_color(fragPos, 3);

    RendEqSol += BRDF(Albedo, Norm, EyeDir, -lightDir) * lightColor;

	vec3 Ambient = texture(tex, fragUV).rgb * 0.025f;

	vec3 col = RendEqSol + Ambient;
	
	outColor = vec4(col, 1.0f);
}