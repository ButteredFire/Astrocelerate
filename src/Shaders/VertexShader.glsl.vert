#version 450

// NOTE: It is possible to bind multiple descriptor sets simultaneously:
// layout(set = n, binding = k) [type] [name] [...];
// where [name] is the data at the k-th binding of the n-th descriptor set layout.

// You can use this feature to put descriptors that vary per-object and descriptors that are shared into separate descriptor sets. In that case, you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.

    // Define custom type for the UBO -> layout(...) uniform UniformBufferObject UBO;
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec3 cameraPosition;
    vec3 lightDirection;
    vec3 lightColor;
} globalUBO;

layout(set = 0, binding = 1) uniform ObjectUBO {
    mat4 model;
    mat4 normalMatrix;
} objectUBO;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoord_0;  // This assumes there is only 1 UV channel. More inputs must be added for multiple channels.
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTextureCoord_0;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragPosition;


void main() {
    // A vertex is transformed as follows:
    // v_clip = projection * view * model * v_local   , where v_local is the position of the vertex in local space
    gl_Position = globalUBO.projection * globalUBO.view * objectUBO.model * vec4(inPosition, 1.0);


    fragColor = inColor;
    fragTextureCoord_0 = inTextureCoord_0;


    // Transform vertex properties to world space
        // NOTE: Normals transform with the inverse transpose of the model matrix
    fragNormal = normalize((objectUBO.normalMatrix * vec4(inNormal, 0.0)).xyz);

        // NOTE: Tangents transform like directions (with model matrix, but no translation component)
    fragTangent = normalize((objectUBO.model * vec4(inTangent, 0.0)).xyz);
    
    fragPosition = (objectUBO.model * vec4(inPosition, 1.0)).xyz;
}