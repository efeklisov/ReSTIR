#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 texCoord;
};

layout(binding = 3, set = 0) uniform sampler2D texSampler;
layout(binding = 4, set = 0, scalar) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 5, set = 0) buffer Indices { uint i[]; } indices;

vec3 lightPos = vec3(0.0f, 0.0f, 100.0f);

void main()
{
  // Indices of the triangle
  ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID + 0],
                      indices.i[3 * gl_PrimitiveID + 1],
                      indices.i[3 * gl_PrimitiveID + 2]);

  // Vertex of the triangle
  Vertex v0 = vertices.v[index.x];
  Vertex v1 = vertices.v[index.y];
  Vertex v2 = vertices.v[index.z];

  const vec3 barycentric = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  vec3 normal = normalize(v0.normal * barycentric.x + v1.normal * barycentric.y + v2.normal * barycentric.z);

  vec3 lightVector = normalize(lightPos);

  vec2 texCoord = v0.texCoord * barycentric.x + v1.texCoord * barycentric.y + v2.texCoord * barycentric.z;

  hitValue = vec3(max(dot(normal, lightVector), 0.2));
  //hitValue = barycentric;
  hitValue *= texture(texSampler, texCoord).xyz;
}
