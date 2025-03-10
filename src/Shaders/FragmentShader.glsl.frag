#version 450

// The `layout(location = i)` modifier specifies the index of the framebuffer
layout(location = 0) out vec4 outFragColor;

layout(location = 0) in vec3 inFragColor;

// The main function is called for every fragment
void main() {
	outFragColor = vec4(inFragColor, 1.0);
}
