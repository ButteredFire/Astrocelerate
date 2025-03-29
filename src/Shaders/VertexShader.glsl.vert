#version 450

// NOTE: It is possible to bind multiple descriptor sets simultaneously:
// layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
// You can use this feature to put descriptors that vary per-object and descriptors that are shared into separate descriptor sets. In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.


layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} UBO;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    // A vertex is transformed as follows:
    // v_clip = projection * view * model * v_local   , where v_local is the position of the vertex in local space
    gl_Position = UBO.projection * UBO.view * UBO.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}