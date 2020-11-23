#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "includes.glsl"

layout(location = 0) rayPayloadInEXT hitPayload hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, set = 0) uniform sampler2D texSamplers[];
layout(binding = 4, set = 0, scalar) buffer Vertices { Vertex v[]; } vertices[];
layout(binding = 5, set = 0) buffer Indices { uint i[]; } indices[];
layout(binding = 6, set = 0, scalar) buffer Materials { Material m; } materials[];

vec3 lightPos = vec3(-4.0f, 5.0f, -2.0f);
float lightIntensity = 0.7;

#include "shootRay.glsl"

void main()
{
    // Indices of the Triangle
    ivec3 index = ivec3(indices[nonuniformEXT(gl_InstanceCustomIndexEXT)].i[3 * gl_PrimitiveID + 0],
                      indices[nonuniformEXT(gl_InstanceCustomIndexEXT)].i[3 * gl_PrimitiveID + 1],
                      indices[nonuniformEXT(gl_InstanceCustomIndexEXT)].i[3 * gl_PrimitiveID + 2]);

    // Vertex of the Triangle
    Vertex v0 = vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[index.x];
    Vertex v1 = vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[index.y];
    Vertex v2 = vertices[nonuniformEXT(gl_InstanceCustomIndexEXT)].v[index.z];

    // Light
    const vec3 barycentric = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 normal = bInterpolation(v0.normal, v1.normal, v2.normal, barycentric);

    vec3 lightVector = normalize(lightPos);
    vec3 lightComputed = vec3(max(dot(normal, lightVector), 0.2));

    // Material
    Material mat = materials[nonuniformEXT(gl_InstanceCustomIndexEXT)].m;

    // Diffuse
    vec3 diffuse = mat.diffuse * max(dot(normal, lightVector), 0.0);
    if (mat.shadingModel >= 1)
        diffuse += mat.ambient;

    vec2 texCoord = v0.texCoord * barycentric.x + v1.texCoord * barycentric.y + v2.texCoord * barycentric.z;
    diffuse *= texture(texSamplers[nonuniformEXT(gl_InstanceCustomIndexEXT)], texCoord).xyz;

    // Specular
    const float shininess = max(mat.shininess, 4.0);
    const float energyConservation = (2.0 + shininess) / (2.0 * 3.14159265);

    vec3 viewVector = normalize(-gl_WorldRayDirectionEXT);
    vec3 reflection = reflect(-lightVector, normal);

    vec3 specular = mat.specular * energyConservation * pow(max(dot(viewVector, reflection), 0.0), shininess);

    // Cast Ray
    // vec2 texCoord = v0.texCoord * barycentric.x + v1.texCoord * barycentric.y + v2.texCoord * barycentric.z;
    // vec3 diffuse = texture(texSamplers[nonuniformEXT(gl_InstanceCustomIndexEXT)], texCoord).xyz;

    vec3 tangent = bInterpolation(v0.tangent, v1.tangent, v2.tangent, barycentric);
    vec3 bitangent = bInterpolation(v0.bitangent, v1.bitangent, v2.bitangent, barycentric);

	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	vec3 direction = CosineWeightedHemisphereSample(hitValue.seed, normal, tangent, bitangent);

    hitValue.color = (diffuse + specular + colorRay(origin, direction, hitValue.seed, hitValue.depth + 1)) / (hitValue.depth + 1) + mat.emissive;

    // vec3 specular = vec3(0.0);
    // float shadowness = 1.0;
	// shadowed = true;

    // // Cast Shadow Ray
	// float tmin = 0.001;
	// float tmax = 10000.0;

	// vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    // traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);

    // if (shadowed) {
    //     shadowness = 0.3;
    // } else if (mat.shadingModel >= 2) {
    //     // Specular
    //     const float shininess = max(mat.shininess, 4.0);
    //     const float energyConservation = (2.0 + shininess) / (2.0 * 3.14159265);

    //     vec3 viewVector = normalize(-gl_WorldRayDirectionEXT);
    //     vec3 reflection = reflect(-lightVector, normal);

    //     specular = mat.specular * energyConservation * pow(max(dot(viewVector, reflection), 0.0), shininess);
    // }

    // // Result
    // hitValue.color = lightComputed * shadowness * (diffuse + specular);
}
