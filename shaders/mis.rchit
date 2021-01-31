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
} sizes;

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
        return 0.3f;
    else
        return 1.0f;
}

float shadowRay(vec3 origin, float shadowBias, vec3 target) {
	shadowed = true;

    vec3 where = target - origin;
    vec3 direction = normalize(where);
    float dist = length(where);

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

vec3 lightPoint(Light light) {
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

float lightPDF(Light light, vec3 hitPos, vec3 origin) {
    vec3 dir = hitPos - origin;
    float D = length(dir);
    return D * D / (lightArea(light) * dot(light.normal, -normalize(dir)));
}

struct LightSample {
    vec3 pos;
    float pdf;
};

LightSample lightSample(Light light, Vertex v) {
    LightSample lightSam;
    lightSam.pos = lightPoint(light);
    lightSam.pdf = lightPDF(light, lightSam.pos, v.pos);
    return lightSam;
}

vec3 evalBRDF(Vertex v, Material mat, vec3 texColor, vec3 lpos) {
    vec3 sdir = normalize(lpos - v.pos);
    float cosTheta2 = dot(sdir, v.normal);
    return texColor * cosTheta2 / pi;
}

struct BRDFSample {
    vec3 val;
    float pdf;
    vec3 dir;
    float cosTheta;
};

BRDFSample brdfSample(Vertex v, Material mat, vec3 texColor) {
    BRDFSample iSample;

    iSample.dir = CosineWeightedHemisphereSample(hitValue.seed, v, iSample.cosTheta);
    iSample.val = texColor / pi;
    iSample.pdf = iSample.cosTheta / pi;

    return iSample;
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
        vec3 hitPos = prevHitPos + gl_WorldRayDirectionEXT * gl_HitTEXT;

        float p_e = lightPDF(light, hitPos, prevHitPos);
        float p_i = hitValue.prevPDF;
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

    // MIS
    Light light = lights.l[int(nextRand(hitValue.seed) * sizes.lightsSize)];

    LightSample lightSam = lightSample(light, v);
    float shadow = shadowRay(v.pos, shadowBias, lightSam.pos);
    vec3 shadeColor = evalBRDF(v, mat, texColor, lightSam.pos);
    vec3 explicitColor = shadow * (light.intensity / lightSam.pdf) * shadeColor;

    BRDFSample brdfSam = brdfSample(v, mat, texColor);

    hitValue.prevPDF = brdfSam.pdf;
    colorRay(v.pos, brdfSam.dir, hitValue.seed, hitValue.depth + 1);
    vec3 indirectColor = hitValue.color;

    float p_e = lightSam.pdf;
    float p_i = brdfSam.pdf; // Это очень плохо, так как нужен PDF теневого луча
    float w_e = p_e * p_e / (p_e * p_e + p_i * p_i);

    hitValue.color = explicitColor * w_e + (brdfSam.val / brdfSam.pdf) * brdfSam.cosTheta * indirectColor;
}
