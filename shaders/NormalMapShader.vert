#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec4 inTan;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec4 fragTan;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out int instance;

const int NCOIN=10;
layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat[NCOIN];
	mat4 mMat[NCOIN];
	mat4 nMat[NCOIN];
} ubo;

void main() {
	int i = gl_InstanceIndex ;
	gl_Position = ubo.mvpMat[i] * vec4(inPos, 1.0);
    fragPos = (ubo.mMat[i] * vec4(inPos, 1.0)).xyz;
    fragNorm = mat3(ubo.nMat[i]) * inNorm;
    fragTan = vec4(mat3(ubo.nMat[i]) * inTan.xyz, inTan.w);
    fragUV = inUV;
    instance = i;
}