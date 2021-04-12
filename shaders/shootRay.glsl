// Based on https://github.com/RikoOphorst/dxr-path-tracing

/* uint initialSeed(uint frst, uint scnd, uint iterations) */
/* { */
/*   uint wght = 0; */

/*   for (uint n = 0; n < iterations; n++) */
/*   { */
/*     wght += 0x9e3779b9; */
/*     frst += ((scnd << 4) + 0xa341316c) ^ (scnd + wght) ^ ((scnd >> 5) + 0xc8013ea4); */
/*     scnd += ((frst << 4) + 0xad90777d) ^ (frst + wght) ^ ((frst >> 5) + 0x7e95761e); */
/*   } */
/*   return frst; */
/* } */

/* float nextRand(inout uint seed) */
/* { */
/*   seed = (1664525u * seed + 1013904223u); */
/*   return float(seed & 0x00FFFFFF) / float(0x01000000); */
/* } */

uint TausStep(uint z, int S1, int S2, int S3, uint M)
{
    uint b = (((z << S1) ^ z) >> S2);
    return (((z & M) << S3) ^ b);    
}

uint LCGStep(uint z, uint A, uint C)
{
    return (A * z + C);    
}

float nextRand(inout uvec4 state)
{
    state.x = TausStep(state.x, 13, 19, 12, 4294967294);
    state.y = TausStep(state.y, 2, 25, 4, 4294967288);
    state.z = TausStep(state.z, 3, 11, 17, 4294967280);
    state.w = LCGStep(state.w, 1664525, 1013904223);

    return 2.3283064365387e-10 * (state.x ^ state.y ^ state.z ^ state.w);
}

vec3 RandomUnitVectorInHemisphereOf(inout uvec4 seed, Vertex v) {
    float phi = 2 * 3.14159265f * float(nextRand(seed));
    float h = 2 * float(nextRand(seed)) - 1;

    float x = sin(phi) * sqrt(1 - h * h);
    float y = cos(phi) * sqrt(1 - h * h);
    float z = h;

    return normalize(v.tangent * y + v.bitangent * x + v.normal * z);
}

vec3 RandomCosineVectorOf(inout uvec4 seed, Vertex v) {
    float e = 1.0f;
    float phi = 2 * 3.14159265f * float(nextRand(seed));

    float cosTheta = pow(1.0f - float(nextRand(seed)), 1.0f / (e + 1.0f));
    float sinTheta = sqrt(1 - cosTheta * cosTheta);

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;

    return normalize(v.tangent * y + v.bitangent * x + v.normal * z);
}

void colorRay(vec3 origin, vec3 direction, uvec4 seed, uint depth) {
    if (depth >= 4)
        return;

	float tmin = 0.001f;
	float tmax = 10000.0f;

    hitValue.seed = seed;
    hitValue.depth = depth;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);
}
