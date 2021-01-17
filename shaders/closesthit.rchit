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

float shadowBias = 0.0001f;
float pi = 3.14159265f;
float albedo = 0.18f;

#include "shootRay.glsl"

float shadowRay(vec3 origin, float shadowBias, vec3 direction, float dist) {
	shadowed = true;

    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 
        0xFF, 0, 0, 1, origin, shadowBias, direction, dist, 2);

    if (shadowed)
        return 0.3f;
    else
        return 1.0f;
}

void getLightInfo(PointLight light, vec3 iPoint, out vec3 dir, out vec3 intensity, out float dist) {
    dir = iPoint - light.pos;
    dist = length(dir);
    dir = normalize(dir);

    float r2 = dist * dist;
    intensity = light.intensity * light.color / (4 * pi * r2);
}

Vertex barycentricVertex(Vertex v0, Vertex v1, Vertex v2) {
    const vec3 barycentric = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 origin    = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal    = v0.normal * barycentric.x + v1.normal * barycentric.y + v2.normal * barycentric.z;
    vec2 texCoord  = v0.texCoord * barycentric.x + v1.texCoord * barycentric.y + v2.texCoord * barycentric.z;
    vec3 tangent   = v0.tangent * barycentric.x + v1.tangent * barycentric.y + v2.tangent * barycentric.z;
    vec3 bitangent = v0.bitangent * barycentric.x + v1.bitangent * barycentric.y + v2.bitangent * barycentric.z;

    return Vertex(origin, normal, texCoord, tangent, bitangent);
}

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

    // Interpolated vertex
    Vertex v = barycentricVertex(v0, v1, v2);

    // Sample texture
    vec3 texColor = texture(texSamplers[nonuniformEXT(gl_InstanceCustomIndexEXT)], v.texCoord).xyz;

    // Light
    PointLight light;
    light.pos = vec3(-4.0f, 5.0f, -2.0f);
    light.color = vec3(0.0f, 0.0f, 1.0f);
    light.intensity = 3000.0f;

    PointLight light1;
    light1.pos = vec3(4.0f, 5.0f, 3.0f);
    light1.color = vec3(1.0f, 0.0f, 0.0f);
    light1.intensity = 4000.0f;

    PointLight light2;
    light2.pos = vec3(4.0f, 5.0f, -3.0f);
    light2.color = vec3(0.0f, 1.0f, 0.0f);
    light2.intensity = 2000.0f;

    const int lightsNum = 3;
    PointLight lights[lightsNum] = { light, light1, light2 };

    for (int i = 0; i < lightsNum; i++) {
        vec3  lightDir;
        vec3  lightIntensity;
        float lightDistance;
        getLightInfo(lights[i], v.pos, lightDir, lightIntensity, lightDistance);

        vec3 L = -lightDir;

        float shadowness = shadowRay(v.pos, shadowBias, L, lightDistance);

        hitValue.color += shadowness * albedo / pi * lightIntensity * max(0.0f, dot(v.normal, L));
    }

//    vec3 lightComputed = vec3(max(dot(v.normal, lightVector), 0.2));

    // Material
//    Material mat = materials[nonuniformEXT(gl_InstanceCustomIndexEXT)].m;

    // Diffuse
//    vec3 diffuse = mat.diffuse * max(dot(v.normal, lightVector), 0.0);
//    if (mat.shadingModel >= 1)
//        diffuse += mat.ambient;
//
//    diffuse *= texture(texSamplers[nonuniformEXT(gl_InstanceCustomIndexEXT)], v.texCoord).xyz;
//
//    vec3 specular = vec3(0.0);

//    if (shadowed) {
//        shadowness = 0.3;
//    } else if (mat.shadingModel >= 2) {
//        // Specular
//        const float shininess = max(mat.shininess, 4.0);
//        const float energyConservation = (2.0 + shininess) / (2.0 * 3.14159265);
//
//        vec3 viewVector = normalize(-gl_WorldRayDirectionEXT);
//        vec3 reflection = reflect(-lightVector, v.normal);
//
//        specular = mat.specular * energyConservation * pow(max(dot(viewVector, reflection), 0.0), shininess);
//    }


//    // Direct Result
//    vec3 directColor = lightComputed * shadowness * (diffuse + specular);
//
//    // Indirect Result
//	vec3 indirectColor = vec3(0.0, 0.0, 0.0);
//
//    uint N = 16;
//	for (uint i = 0; i < N; i++) {
//        float cosTheta;
//        vec3 direction = CosineWeightedHemisphereSample(hitValue.seed, v, cosTheta);
//        
//        indirectColor += colorRay(v.pos, direction, hitValue.seed, hitValue.depth + 1) * cosTheta;
//    }
//    indirectColor /= N;
//
//    hitValue.color = directColor + indirectColor + mat.emissive;
}
