#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable

#define NLIGHTS 4

// this defines the variable received from the Vertex Shader
// the locations must match the one of its out variables
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

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

void main() {
	vec3 Albedo = texture(tex, fragUV).rgb;

    Albedo = texture(tex, fragUV).rgb;
	
	outColor = vec4(Albedo, 1.0f);
}