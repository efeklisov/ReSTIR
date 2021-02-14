#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "../includes.glsl"

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
} sizes;

float shadowBias = 0.0001f;
float pi = 3.14159265f;
float albedo = 0.18f;
float specularPower = 35;

#include "../shootRay.glsl"

float shadowRay(vec3 origin, float shadowBias, vec3 direction, float dist) {
	shadowed = true;

    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 
        0xFF, 0, 0, 1, origin, shadowBias, direction, dist, 2);

    if (shadowed)
        return 0.3f;
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

float lightArea(Light light) {
    return length(light.ab) * length(light.ac);
}

vec3 lightSample(Light light) {
    vec3 a = light.a;
    vec3 b = light.ab + light.a;
    vec3 c = light.ac + light.a;
    vec3 d = light.ab + light.ac + light.a;
    
    float s = nextRand(hitValue.seed);
    vec3 e = s * a + (1 - s) * b;
    vec3 f = s * c + (1 - s) * d;

    float t = nextRand(hitValue.seed);
    return t * e + (1 - t) * f;
}

void main()
{
    uint instance = nonuniformEXT(gl_InstanceCustomIndexEXT);

    // Ray direction
    vec3 rayDir = -normalize(gl_WorldRayDirectionEXT);

    if (instance >= sizes.meshesSize) {
        uint lightNo = instance - sizes.meshesSize;
        Light light = lights.l[lightNo];

        bool frontSide = dot(rayDir, light.normal) > 0;
        if(!hitValue.diffuse) {
            if (frontSide)
                hitValue.color = lights.l[lightNo].color * lights.l[lightNo].intensity;
            else
                hitValue.color = vec3(0.1f, 0.1f, 0.1f);
            return;
        }

        if (!frontSide)
            return;

        vec3 prevHitPos = gl_WorldRayOriginEXT;
        vec3 direction = gl_WorldRayDirectionEXT;
        vec3 hitPos = prevHitPos + direction * gl_HitTEXT;

        float D = length(hitPos - prevHitPos);
        float p_e = D * D / (lightArea(light) * dot(light.normal, -direction));
        float p_i = dot(hitValue.prevNrm, direction) / pi;
        float w_i = p_i * p_i / (p_e * p_e + p_i * p_i);

        hitValue.color = light.intensity * light.color * w_i;
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
    Light light = lights.l[int(nextRand(hitValue.seed) * sizes.lightsSize)];

    vec3 lpos = lightSample(light);
    float R = length(v.pos - lpos);
    vec3 sdir = normalize(lpos - v.pos);
    float shadow = shadowRay(v.pos, shadowBias, sdir, R);

    float lgtCosTheta = -dot(sdir, light.normal);
    float lgtPdf = (1.0 / lightArea(light)) * R * R / lgtCosTheta;

    float lambCosTheta = dot(sdir, v.normal);
    float lambPdf = lambCosTheta / pi;

    vec3 explicitColor = light.intensity * texColor * shadow * lambPdf / lgtPdf;

    // Cast new ray
    vec3 newRayD = RandomCosineVectorOf(hitValue.seed, v);
    float cosTheta = dot(newRayD, v.normal);

    float PDF = cosTheta / pi;
    vec3 BRDF = texColor / pi;

    float p_e = R * R / (lightArea(light) * lgtCosTheta);
    float p_i = cosTheta / pi;
    float w_e = p_e * p_e / (p_e * p_e + p_i * p_i);

    hitValue.prevNrm = v.normal;
    colorRay(v.pos, newRayD, hitValue.seed, hitValue.depth + 1);
    vec3 indirectColor = hitValue.color;

    hitValue.color = explicitColor * w_e + (BRDF / PDF) * cosTheta * indirectColor;
}
