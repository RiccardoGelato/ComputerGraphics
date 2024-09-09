#version 450
#extension GL_ARB_separate_shader_objects : enable

// this defines the variable received from the Vertex Shader
// the locations must match the one of its out variables
layout(location = 0) in vec2 fragUV;

// This defines the color computed by this shader. Generally is always location 0.
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D tex;

layout(set = 1, binding = 0) uniform NormalMapParUniformBufferObject {
	float sinSun;
	float isMoon;
} mubo;

// The main shader, implementing a simple Blinn + Lambert + constant Ambient BRDF model
// The scene is lit by a single Spot Light
void main() {
	vec3 Emit = texture(tex, fragUV).rgb;

	float sinSun;
    	if(mubo.sinSun < 0){
    	    sinSun = 0.0;
    	}else{
    	    sinSun = mubo.sinSun;
    	}

	//cambio il colore in base a se è la luna o il sole, (se è il sole cambia il colore in base all'ora del giorno)
	outColor = vec4(Emit, 1.0f) * (1 - sinSun * mubo.isMoon) + vec4(1.0, 1.0, 1.0, 1.0) * sinSun * mubo.isMoon;
}