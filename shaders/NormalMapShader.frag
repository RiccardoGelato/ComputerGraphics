#version 450
#extension GL_ARB_separate_shader_objects : enable

#define NLIGHTS 28
const int NCOINS=10;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec4 fragTan;
layout(location = 3) in vec2 fragUV;
layout(location = 4) flat in int instance;



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

layout(set = 1, binding = 1) uniform sampler2D texDiff;
layout(set = 1, binding = 2) uniform sampler2D texNM;

layout(set = 1, binding = 3) uniform NormalMapParUniformBufferObject {
	float Pow;
	mat4 CoinTaken[NCOINS];
} mubo;


//FUNCTIONS DIFFERENT LIGHTS
//Type:
//0-direct
//1-spot
//2-point

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
	float division = /*gubo.lightColor[i].a*/ 2 / length(gubo.lightPos[i] - pos);
	float decay = pow(division, 1.5);
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

vec3 light_dir(vec3 pos, int i, float type){
    vec3 dir;
    if(type == 0){
        dir = direct_light_dir(pos, i);
    }else if(type == 1){
        dir = point_light_dir(pos, i);
    }else{
        dir = spot_light_dir(pos, i);
    }
    return dir;
}

vec3 light_color(vec3 pos, int i,float type){
    vec3 color;
    if(type == 0){
        color = direct_light_color(pos, i);
    }else if(type == 1){
        color = point_light_color(pos, i);
    }else{
        color = spot_light_color(pos, i);
    }
    return color;
}

vec3 BRDFNormal(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {
	vec3 Diffuse;
	vec3 Specular;
	Diffuse = Albedo * max(dot(Norm, LD),0.0f); //lambert
	Specular = Albedo * vec3(pow(max(dot(Norm, normalize(LD + EyeDir)),0.0), mubo.Pow)); //blinn

	return Diffuse + Specular;
}

vec3 BRDFToon(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {

    float cosa = dot(Norm, LD);

    float cosb = dot(EyeDir, -reflect(LD, Norm));


    vec3 Diffuse = Albedo;
    vec3 Specular = Albedo;

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

void main() {
	vec3 Norm = normalize(fragNorm);
	vec3 Tan = normalize(fragTan.xyz - Norm * dot(fragTan.xyz, Norm));
	vec3 Bitan = cross(Norm, Tan) * fragTan.w;
	mat3 tbn = mat3(Tan, Bitan, Norm);
	vec4 nMap = texture(texNM, fragUV);
	vec3 N = normalize(tbn * (vec3(-1, 1, -1) + nMap.rgb * vec3(2, -2, 2)));
	vec3 EyeDir = normalize(gubo.eyePos - fragPos);

    //usiamo albedo sia per lo specular che per il diffuse perchè sono d'oro
	vec3 Albedo = texture(texDiff, fragUV).rgb;

	vec3 RendEqSol = vec3(0);

    vec3 lightDir = light_dir(fragPos, 0, 0);
    vec3 lightColor = light_color(fragPos, 0, 0);

    RendEqSol += BRDF(Albedo, N, EyeDir, lightDir) * lightColor;

    lightDir = light_dir(fragPos, 1, 2);
    lightColor = light_color(fragPos, 1, 2);

    RendEqSol += BRDF(Albedo, N, EyeDir, lightDir) * lightColor * gubo.lightOn;

    lightDir = light_dir(fragPos, 2, 2);
    lightColor = light_color(fragPos, 2, 2);

    RendEqSol += BRDF(Albedo, N, EyeDir, lightDir) * lightColor * gubo.lightOn;

    lightDir = light_dir(fragPos, 3, 0);
    lightColor = light_color(fragPos, 3, 0);

    RendEqSol += BRDF(Albedo, N, EyeDir, lightDir) * lightColor;

    for(int j = 4; j < NLIGHTS; j++){
            lightDir = light_dir(fragPos, j, 1);
            lightColor = light_color(fragPos, j, 1);

            RendEqSol += BRDF(Albedo, N, EyeDir, lightDir) * lightColor;
    }

    vec3 Ambient = texture(texDiff, fragUV).rgb * 0.025f;
    /*const vec3 cxp = vec3(1.0,1.0,1.0) * 0.025f;
    const vec3 cxn = vec3(1.0,1.0,1.0) * 0.025f;
    const vec3 cyp = vec3(0.0,1.0,0.0) * 0.025f;
    const vec3 cyn = vec3(0.0,1.0,0.0) * 0.025f;
    const vec3 czp = vec3(1.0,1.0,1.0) * 0.025f;
    const vec3 czn = vec3(1.0,1.0,1.0) * 0.025f;

    Ambient =((N.x > 0 ? cxp : cxn) * (N.x * N.x) +
              (N.y > 0 ? cyp : cyn) * (N.y * N.y) +
              (N.z > 0 ? czp : czn) * (N.z * N.z)) * Albedo;*/

    vec3 col = RendEqSol + Ambient;
    
    //metto la trasparenza se il coin è stato preso e il colore a nero
	if(mubo.CoinTaken[instance][0][0] == 1.0){
	    outColor = vec4(0.0,0.0,0.0, 0.5f);
	}else{
	    outColor = vec4(col, 1.0f);
	}

}