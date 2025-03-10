#version 450

layout(location = 0) out vec3 outFragColor;

vec2 testPositions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(-0.5, 0.5),
	vec2(0.5, 0.5)
);

vec3 vertColors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

// The main function is invoked for every vertex
void main() {
	// gl_VertexIndex contains the index of the current vertex being evaluated by the main function
	gl_Position = vec4(testPositions[gl_VertexIndex], 0.0, 1.0);
    outFragColor = vertColors[gl_VertexIndex];
}
