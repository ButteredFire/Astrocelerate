#version 450

// The `layout(location = i)` modifier specifies the index of the framebuffer
	// NOTE: There are also `sampler1D` and `sampler3D` types for other types of images
layout(binding = 2) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTextureCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;

layout(location = 0) out vec4 outColor;


// The main function is called for every fragment
void main() {
	// texture(sampler, coord) takes a sampler and coordinate as arguments. 
	outColor = texture(textureSampler, fragTextureCoord);
}
