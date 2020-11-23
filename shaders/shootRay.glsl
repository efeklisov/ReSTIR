// Based on https://github.com/RikoOphorst/dxr-path-tracing

uint initialSeed(uint frst, uint scnd, uint iterations)
{
  uint wght = 0;

  for (uint n = 0; n < iterations; n++)
  {
    wght += 0x9e3779b9;
    frst += ((scnd << 4) + 0xa341316c) ^ (scnd + wght) ^ ((scnd >> 5) + 0xc8013ea4);
    scnd += ((frst << 4) + 0xad90777d) ^ (frst + wght) ^ ((frst >> 5) + 0x7e95761e);
  }
  return frst;
}

float nextRand(inout uint seed)
{
  seed = (1664525u * seed + 1013904223u);
  return float(seed & 0x00FFFFFF) / float(0x01000000);
}

vec3 CosineWeightedHemisphereSample(inout uint seed, vec3 normal, vec3 tangent, vec3 bitangent)
{
  vec2 rand = vec2(nextRand(seed), nextRand(seed));

  float r = sqrt(rand.x);
  float phi = 2.0f * 3.14159265f * rand.y;

  return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + normal * sqrt(1 - rand.x);
}

vec3 bInterpolation(vec3 frst, vec3 scnd, vec3 thrd, vec3 barycentric) {
    return normalize(frst * barycentric.x + scnd * barycentric.y + thrd * barycentric.z);
}

vec3 colorRay(vec3 origin, vec3 direction, uint seed, uint depth) {
    if (depth >= 32)
        return vec3(0.0f);

	float tmin = 0.001f;
	float tmax = 10000.0f;

    hitValue.color = vec3(0.0);
    hitValue.seed = seed;
    hitValue.depth = depth;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);

    return hitValue.color;
}
