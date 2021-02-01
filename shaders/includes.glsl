struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 texCoord;
  vec3 tangent;
  vec3 bitangent;
};

struct Material
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 emissive;
  vec3 transparent;
  float shininess;
  float iorefraction;
  float dissolve;
  int shadingModel;
  int diffuseMapCount;
};

struct hitPayload
{
    vec3 color;
    uint seed;
    uint depth;
    bool diffuse;
    float prevPDF;
    vec3 prevNrm;
};

struct Light
{
    vec3 color;
    float intensity;
    vec3 normal;
    vec3 a;
    vec3 ab;
    vec3 ac;
};
