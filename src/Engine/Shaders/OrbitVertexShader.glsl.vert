#version 450


layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec3 cameraPosition;
    vec3 floatingOrigin;
    float ambientStrength;
    vec3 lightPosition;
    float radiantFlux;
    vec3 lightColor;
} globalUBO;

layout(push_constant) uniform PushConstants {
    vec4 color;
} pc;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 fragColor;


void main() {
    vec3 pos = inPosition - globalUBO.floatingOrigin;
    gl_Position = globalUBO.projection * globalUBO.view * vec4(pos, 1.0);

    fragColor = pc.color;
}
