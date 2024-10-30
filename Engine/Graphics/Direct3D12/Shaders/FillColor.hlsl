#include "Fractals.hlsli"

struct ShaderConstants
{
    float width;
    float height;
    uint frame;
};

ConstantBuffer<ShaderConstants> shader_params : register(b1);

#define SAMPLES 4

float4 fill_color_ps(in noperspective float4 position : SV_Position, in noperspective float2 in_uv : TEXCOORD) : SV_Target0
{
    const float offset = 0.2f;
    const float2 offsets[4] =
    {
        float2(-offset, offset),
        float2(offset, offset),
        float2(offset, -offset),
        float2(-offset, -offset),
    };

    const float2 inv_dim = float2(1.f / shader_params.width, 1.f / shader_params.height);
    
    float3 color = 0.f;
    for (int i = 0; i < SAMPLES; i++)
    {
        const float2 uv = (position.xy + offsets[i]) * inv_dim;
        color += draw_julia_set(uv, shader_params.frame); //draw_mandelbrot(uv); 

    }
    //return float4(draw_hearth(in_uv, shader_params.frame), 1.f);
    return float4(float3(color.z, color.x, 1.f) * color.x / SAMPLES, 1.f);
    //return float4(color / SAMPLES, 1.f);

}