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
layout(binding = 7, set = 0, scalar) buffer Lights { Light l[]; } lights;
layout(binding = 8, set = 0) uniform Sizes {
    uint meshesSize;    
    uint lightsSize;
    uint M;
} sizes;

#define MAX_SAMPLES 64

float shadowBias = 0.0001f;
float pi = 3.14159265f;
float albedo = 0.18f;
float specularPower = 35;

#include "shootRay.glsl"

float shadowRay(vec3 origin, float shadowBias, vec3 direction, float dist) {
	shadowed = true;

    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 
        0xFF, 0, 0, 1, origin, shadowBias, direction, dist, 2);

    if (shadowed)
        return 0.0f;
    else
        return 1.0f;
}

// void getLightInfo(PointLight light, vec3 iPoint, out vec3 dir, out vec3 intensity, out float dist) {
    // dir = iPoint - light.pos;
    // dist = length(dir);
    // dir = normalize(dir);

    // float r2 = dist * dist;
    // intensity = light.intensity * light.color / (4 * pi * r2);
// }

Vertex barycentricVertex(Vertex v0, Vertex v1, Vertex v2) {
    const vec3 barycentric = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 origin    = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal    = v0.normal * barycentric.x + v1.normal * barycentric.y + v2.normal * barycentric.z;
    vec2 texCoord  = v0.texCoord * barycentric.x + v1.texCoord * barycentric.y + v2.texCoord * barycentric.z;
    vec3 tangent   = v0.tangent * barycentric.x + v1.tangent * barycentric.y + v2.tangent * barycentric.z;
    vec3 bitangent = v0.bitangent * barycentric.x + v1.bitangent * barycentric.y + v2.bitangent * barycentric.z;

    return Vertex(origin, normal, texCoord, tangent, bitangent);
}

vec3 lightSample(Light light, float eps) {
    float eps2 = nextRand(hitValue.seed);
    return light.a + eps * light.ab + eps2 * light.ac;
}

float desPdf(Light light, Vertex v, vec3 lpos, vec3 texColor) {
    float L_e = light.intensity;
    /* float BRDF = length(texColor) / pi; // Lambert */

    vec3  ldir = normalize(v.pos - lpos);
    float norm = length(v.pos - lpos);

    return /*BRDF* L_e * */ dot(ldir, light.normal) / (norm * norm);
}

float lgtPdf(Light light) {
    return 2.0f / length(cross(light.ab, light.ac));
}

void main()
{
    uint instance = nonuniformEXT(gl_InstanceCustomIndexEXT);

    // Ray direction
    vec3 rayDir = -normalize(gl_WorldRayDirectionEXT);

    if (instance >= sizes.meshesSize) {
        if (hitValue.diffuse) {
            hitValue.color = vec3(0.0f);
            return;
        }

        uint lightNo = instance - sizes.meshesSize;

        if (dot(rayDir, lights.l[lightNo].normal) > 0)
            hitValue.color = lights.l[lightNo].color * lights.l[lightNo].intensity;
        else
            hitValue.color = vec3(0.1f, 0.1f, 0.1f);
        return;
    }

    hitValue.diffuse = true;

    // Indices of the Triangle
    ivec3 index = ivec3(indices[instance].i[3 * gl_PrimitiveID + 0],
                      indices[instance].i[3 * gl_PrimitiveID + 1],
                      indices[instance].i[3 * gl_PrimitiveID + 2]);

    // Vertex of the Triangle
    Vertex v0 = vertices[instance].v[index.x];
    Vertex v1 = vertices[instance].v[index.y];
    Vertex v2 = vertices[instance].v[index.z];

    // Interpolated vertex
    Vertex v = barycentricVertex(v0, v1, v2);

    // Sample texture
    vec3 texColor = texture(texSamplers[nonuniformEXT(gl_InstanceCustomIndexEXT)], v.texCoord).xyz;

    // Sample material
    Material mat = materials[nonuniformEXT(gl_InstanceCustomIndexEXT)].m;

    // Light
    uint  L[MAX_SAMPLES];
    vec3  Samples[MAX_SAMPLES];
    float W[MAX_SAMPLES];
    float Wsum = 0.0f;
    for (uint i = 0; i < sizes.M; i++) {
        float idx = nextRand(hitValue.seed) * sizes.lightsSize;
        float reusedEps = idx - uint(idx);

        Light light = lights.l[uint(idx)];
        Samples[i] = lightSample(light, reusedEps);

        L[i] = uint(idx);
        W[i] = desPdf(light, v, Samples[i], texColor) / lgtPdf(light); // P_6 / P_5 LOL
        Wsum += W[i];
    }

    float eps = nextRand(hitValue.seed) * Wsum;
    uint idx;
    for (idx = 0; idx < sizes.M; idx++) {
        eps -= W[idx]; 

        if (eps <= 0.0f)
            break;
    }

    Light light = lights.l[L[idx]];
    vec3  lpos = Samples[idx];

    vec3  ldir = normalize(v.pos - lpos);
    float norm = length(v.pos - lpos);
    float shadow = shadowRay(v.pos, shadowBias, -ldir, norm);
    
    float C = 2.0f;
    float L_e = light.intensity;
    vec3 BRDF = texColor / pi; // Lambert

    // vec3 explicitColor = C * shadow * BRDF * L_e * dot(-ldir, v.normal) * dot(ldir, light.normal) / (desPdf(light, v, lpos, texColor) * norm * norm);
    vec3 explicitColor = C * shadow * BRDF * L_e * dot(-ldir, v.normal);

    // Cast new ray
    // vec3 newRayD = RandomCosineVectorOf(hitValue.seed, v);
    // float cosTheta = dot(normalize(newRayD), v.normal);

    // float PDF = cosTheta / pi;
    // vec3 BRDF = texColor / pi;

    // colorRay(v.pos, newRayD, hitValue.seed, hitValue.depth + 1);
    // vec3 indirectColor = hitValue.color;

    hitValue.color = explicitColor * Wsum / sizes.M; // + (BRDF / PDF) * cosTheta * indirectColor;
}
