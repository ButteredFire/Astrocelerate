#version 450

// NOTE: It is possible to bind multiple descriptor sets simultaneously:
// layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
// You can use this feature to put descriptors that vary per-object and descriptors that are shared into separate descriptor sets. In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.

    // Define custom type for the UBO -> layout(...) uniform UniformBufferObject UBO;
layout(set = 0, binding = 0) uniform globalUniformBufferObject {
    mat4 view;
    mat4 projection;
} globalUBO;

layout(set = 0, binding = 1) uniform perObjectUniformBufferObject {
    mat4 model;
} objectUBO;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTextureCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;


void main() {
    // A vertex is transformed as follows:
    // v_clip = projection * view * model * v_local   , where v_local is the position of the vertex in local space
    gl_Position = globalUBO.projection * globalUBO.view * objectUBO.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTextureCoord = inTextureCoord;
    fragNormal = inNormal;
    fragTangent = inTangent;
}