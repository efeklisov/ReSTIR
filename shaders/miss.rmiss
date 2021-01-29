#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "includes.glsl"

layout(location = 0) rayPayloadInEXT hitPayload hitValue;

void main()
{
    hitValue.color = vec3(0.0f);
}
