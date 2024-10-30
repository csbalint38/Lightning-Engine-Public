const static float PI = 3.1415926535897932f;
const static float SAMPLE_OFFSET = .5f;

cbuffer Constants : register(b0)
{
    uint g_cube_map_in_size;
    uint g_cube_map_out_size;
    uint g_sample_count_or_mirror;
    float g_roughness;
};

Texture2D<float4> env_map : register(t0);

RWTexture2DArray<float4> output : register(u0);

SamplerState linear_sampler : register(s0);

float3 get_sample_direction_equirectangular(uint face, float x, float y)
{
    float3 direction[6] =
    {
        { -x, 1.f, -y }, // x+ left
        { x, -1.f, -y }, // x- right
        { y, x, 1.f }, // y+ bottom
        { -y, x, -1.f }, // y- top
        { 1.f, x, -y }, // z+ front
        { -1.f, -x, y }, // z- back
    };
    
    return normalize(direction[face]);
}

float2 direction_to_equirectangular_uv(float3 dir)
{
    float phi = atan2(dir.y, dir.x);
    float theta = acos(dir.z);
    float u = phi * (.5f / PI) + .5f;
    float v = theta * (1.f / PI);
    
    return float2(u, v);
}

[numthreads(16, 16, 1)]
void equirectangular_to_cube_map_cs(uint3 dispatch_thread_id : SV_DispatchThreadID, uint3 group_id : SV_GroupID)
{
    uint face = group_id.z;
    uint size = g_cube_map_out_size;
    
    if (dispatch_thread_id.x >= size || dispatch_thread_id.y >= size || face >= 6) return;

    float2 uv = (float2(dispatch_thread_id.xy) + SAMPLE_OFFSET) / size;
    float2 pos = 2.f * uv - 1.f;
    float3 sampleDirection = get_sample_direction_equirectangular(face, pos.x, pos.y);
    float2 dir = direction_to_equirectangular_uv(sampleDirection);
    
    if (g_sample_count_or_mirror == 1) dir.x = 1.f - dir.x;
    float4 env_sample = env_map.SampleLevel(linear_sampler, dir, 0);

    output[uint3(dispatch_thread_id.x, dispatch_thread_id.y, face)] = env_sample;
}
