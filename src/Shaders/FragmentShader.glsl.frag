#version 450

// The `layout(location = i)` modifier specifies the index of the framebuffer
layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;

// The main function is called for every fragment
void main() {
	outColor = vec4(fragColor, 1.0);
}
