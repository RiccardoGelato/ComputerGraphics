#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0 ,binding = 1) uniform sampler2D skybox;
layout(set = 0 ,binding = 2) uniform sampler2D stars;
layout(set = 1, binding = 0) uniform NormalMapParUniformBufferObject {
	float Ang;
	float ShowCloud;
	float sinSun;
} mubo;

void main() {
	//adatto la texture a coordinate sferiche della skybox
	float yawstars = -(atan(fragTexCoord.x, fragTexCoord.z)/6.2831853 + 0.5);
	float yaw = -(atan(fragTexCoord.x, fragTexCoord.z)/6.2831853 +mubo.Ang + 0.5); //permette di ruotare le nuvole
	float pitch = -(atan(fragTexCoord.y, sqrt(fragTexCoord.x*fragTexCoord.x+fragTexCoord.z*fragTexCoord.z))/3.14159265+ 0.5);


	//calcolo il colore della skybox
	float sinSun;
	if(mubo.sinSun < 0){
	    sinSun = 0.0;
	}else{
	    sinSun = mubo.sinSun;
	}

	outColor = (vec4(0.1,0.5,0.9,1.0) * 0.8 + texture(stars, vec2(yaw, pitch)) * mubo.ShowCloud*0.2) * sinSun
	+ (texture(skybox, vec2(yawstars, pitch))*0.9 + texture(stars, vec2(yaw, pitch))*mubo.ShowCloud*0.1) * (1 - sinSun);

}