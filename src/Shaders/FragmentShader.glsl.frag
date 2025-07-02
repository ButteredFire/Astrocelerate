#version 450
#extension GL_EXT_nonuniform_qualifier : enable // Required for bindless textures

// The `layout(location = i)` modifier specifies the index of the framebuffer
	// NOTE: There are also `sampler1D` and `sampler3D` types for other types of images

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec3 cameraPosition;
    vec3 lightDirection;
    vec3 lightColor;
} globalUBO;

// Samplers for PBR Textures
layout(set = 1, binding = 0) uniform pbrMaterialParameters {
    vec3 albedoColor;
	int albedoMapIndex;

	float metallicFactor;
	float roughnessFactor;
	int metallicRoughnessMapIndex;

	int normalMapIndex;

	int aoMapIndex;

	vec3 emissiveColor;
	int emissiveMapIndex;

	float opacity;
} material;

layout(set = 1, binding = 1) uniform sampler2D textureMap[512];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTextureCoord_0;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;


const float PI = 3.14159265359;
const int INVALID_INDEX = -1;


/* D: Normal Distribution Function (Trowbridge-Reitz GGX) */
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}


/* G: Geometry Obstruction Function (Schlick-GGX) */
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // For direct lighting
    // float k = (roughness * roughness) / 2.0; // For IBL

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}


/* G: Geometry Smith Function (combines two Schlick-GGX functions) */
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}


/* F: Fresnel (Schlick's approximation) */
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}



vec3 computePBRFragment() {
    vec3 normal = normalize(fragNormal);
    vec3 viewVec = normalize(globalUBO.cameraPosition - fragPosition);  // View vector from fragment to camera

    // ----- Acquire material property data -----
        // Albedo
    vec3 albedo = material.albedoColor;
    if (material.albedoMapIndex != INVALID_INDEX) {
        albedo *= texture(nonuniformEXT(textureMap[material.albedoMapIndex]), fragTextureCoord_0).rgb;
    }


        // Metallic & Roughness
    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.metallicRoughnessMapIndex != INVALID_INDEX) {
        // Assimp follows the glTF convention for the metallic-roughness map:
        // (R: Unused/AO, G: Roughness, B: Metallic, A: Unused)
        vec4 metRoughSample = texture(nonuniformEXT(textureMap[material.metallicRoughnessMapIndex]), fragTextureCoord_0);

        metallic = metRoughSample.b;
        roughness = metRoughSample.g;
    }

            // Default F0 for non-metals (becomes the albedo color for metals)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic); // mix(x, y, a) linearly interpolates in the [x, y] range with `a` as the weight
    
    
        // Ambient occlusion
    float ao = 1.0;
    if (material.aoMapIndex != INVALID_INDEX) {
        // AO is typically the red channel of the AO map
        ao = texture(nonuniformEXT(textureMap[material.aoMapIndex]), fragTextureCoord_0).r;
    }


        // Emissive color
    vec3 emissive = material.emissiveColor;
    if (material.emissiveMapIndex != INVALID_INDEX) {
        emissive += texture(nonuniformEXT(textureMap[material.emissiveMapIndex]), fragTextureCoord_0).rgb;
    }


        // Normal map
    if (material.normalMapIndex != INVALID_INDEX) {
        // NOTE: This assumes fragTangent and fragNormal are orthogonal and form a right-handed basis
        
        vec3 tangent = normalize(fragTangent);
        vec3 bitangent = normalize(cross(normal, tangent));

        // Tangent, Bi-tangent, Normal basis
        mat3 TBN = mat3(tangent, bitangent, normal);

        vec3 sampledNormal = texture(nonuniformEXT(textureMap[material.normalMapIndex]), fragTextureCoord_0).rgb;
        
        // Converts from [0, 1] to [-1, 1] (because of Vulkan's nature)
        sampledNormal = normalize(sampledNormal * 2.0 - 1.0);

        // Transforms sampled normal from tangent space to world space
        normal = normalize(TBN * sampledNormal);
    }



    // ----- PBR Lighting Calculation -----
    vec3 outgoingRadiance = vec3(0.0);

    vec3 fragToLightDirection = normalize(-globalUBO.lightDirection);   // Light direction from fragment to light
    vec3 halfwayVec = normalize(viewVec + fragToLightDirection);  // Halfway vector between view and light

        // Compute light properties
            // For simplicity, the light's color will be used directly as radiance for now
    float lightIntensity = 5.0;
    vec3 radiance = globalUBO.lightColor * lightIntensity;


    float NDF = DistributionGGX(normal, halfwayVec, roughness);
    float G = GeometrySmith(normal, viewVec, fragToLightDirection, roughness);
    vec3 F = FresnelSchlick(max(dot(halfwayVec, viewVec), 0.0), F0);


        // Ks (specular reflectance) and Kd (diffuse reflectance)
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Metals have no diffuse contribution


        // BRDF for this light
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewVec), 0.0) * max(dot(normal, fragToLightDirection), 0.0) + 0.0001; // Prevents division by zero by adding a small value to the denominator
    vec3 specular = numerator / denominator;


        // Apply light contribution
    float NdotL = max(dot(normal, fragToLightDirection), 0.0);
    outgoingRadiance += (kD * albedo / PI + specular) * radiance * NdotL; // Cook-Torrance BRDF + Diffuse


    // --- Ambient Lighting (Simple Approximation) ---
    // TODO: Implement a fully functioning ambient lighting model
    // NOTE: In a full PBR setup, this would be IBL (irradiance map + prefiltered env map)
    vec3 ambientColor = vec3(0.00001); // TODO: This should be editable via globalUBO.ambientStrength
    vec3 ambient = ambientColor * albedo * ao;


    // Final color
    vec3 finalColor = outgoingRadiance * ao + emissive + ambient;

    // ----- Tone Mapping & Gamma Correction -----
        // Tone mapping (Reinhard tone mapping)
        // NOTE: Reinhard tone mapping maps RGB values from [0, inf) to [0, 1) to compress bright HDR values into the visible LDR range without clipping them.
    finalColor = finalColor / (finalColor + vec3(1.0));

        // Gamma correction (sRGB is ~2.2 gamma)
    finalColor = pow(finalColor, vec3(1.0 / 2.2)); // Raised to the inverse of the gamma value

    return finalColor;
}


// The main function is called for every fragment
void main() {
	vec3 finalColor = computePBRFragment();
    outColor = vec4(finalColor, material.opacity);
}
