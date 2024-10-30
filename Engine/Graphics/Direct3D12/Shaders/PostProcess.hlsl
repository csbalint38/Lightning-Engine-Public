#include "Common.hlsli"

struct ShaderConstants
{
    uint gpass_main_buffer_index;
};

#define TILE_SIZE 32

ConstantBuffer<GlobalShaderData> global_data : register(b0, space0);
ConstantBuffer<ShaderConstants> shader_params : register(b1, space0);
StructuredBuffer<Frustum> frustums : register(t0, space0);
StructuredBuffer<uint2> light_grid_opaque : register(t1, space0);

uint get_grid_index(float2 pos_xy, float view_width)
{
    const uint2 pos = uint2(pos_xy);
    const uint tile_x = ceil(view_width / TILE_SIZE);
    return (pos.x / TILE_SIZE) + (tile_x * (pos.y / TILE_SIZE));
}

float4 heatmap(StructuredBuffer<uint2> buffer, float2 pos_xy, float blend)
{
    const float w = global_data.view_width;
    const uint grid_index = get_grid_index(pos_xy, w);
    uint num_lights = buffer[grid_index].y;
    
    #if USE_BOUNDING_SPHERES
    const uint num_point_lights = num_lights >> 16;
    const uint num_spot_lights = num_point_lights + (num_lights & 0xffff);
    num_lights = num_point_lights + num_spot_lights;
    #endif
    
    const float3 map_tex[] =
    {
        float3(0, 0, 0),
        float3(0, 0, 1),
        float3(0, 1, 1),
        float3(0, 1, 0),
        float3(1, 1, 0),
        float3(1, 0, 0),
    };
    const uint map_tex_len = 5;
    const uint max_heat = 40;
    float l = saturate((float) num_lights / max_heat) * map_tex_len;
    float3 a = map_tex[floor(l)];
    float3 b = map_tex[ceil(l)];
    float3 heatmap = lerp(a, b, l - floor(l));

    Texture2D gpass_main = ResourceDescriptorHeap[shader_params.gpass_main_buffer_index];
    return float4(lerp(gpass_main[pos_xy].xyz, heatmap, blend), 1.f);

}

float4 post_process_ps(in noperspective float4 position : SV_Position, in noperspective float2 uv : TEXCOORD) : SV_Target0
{
    #if 0
    const float w = global_data.view_width;
    const uint grid_index = get_grid_index(position.xy, w);
    const Frustum f = frustums[grid_index];
    
    #if USE_BOUNDING_SPHERES
    float3 color = abs(f.cone_direction);
    #else
    const uint half_tile = TILE_SIZE / 2;
    float3 color = abs(f.planes[1].normal);
    
    if (get_grid_index(float2(position.x + half_tile, position.y), w) == grid_index && get_grid_index(float2(position.x, position.y + half_tile), w) == grid_index)
    {
        color = abs(f.planes[0].normal);
    }
    else if (get_grid_index(float2(position.x + half_tile, position.y), w) != grid_index && get_grid_index(float2(position.x, position.y + half_tile), w) == grid_index)
    {
        color = abs(f.planes[2].normal);
    }
    else if (get_grid_index(float2(position.x + half_tile, position.y), w) == grid_index && get_grid_index(float2(position.x, position.y + half_tile), w) != grid_index)
    {
        color = abs(f.planes[3].normal);
    }
    #endif
    
    Texture2D gpass_main = ResourceDescriptorHeap[shader_params.gpass_main_buffer_index];
    color = lerp(gpass_main[position.xy].xyz, color, 1.f);
    
    return float4(color, 1.f);
     
    #elif 0 // INDEX VISUALIZATION
    const uint2 pos = uint2(position.xy);
    const uint tile_x = ceil(global_data.view_width / TILE_SIZE);
    const uint2 idx = pos / (uint2)TILE_SIZE;
    
    float c = (idx.x + tile_x * idx.y) * 0.00001f;
    
    if (idx.x % 2 == 0)
    {
        c += .1f;
    }
    if (idx.y % 2 == 0)
    {
        c += .1f;
    }
    
    return float4((float3) c, 1.f);
    
    #elif 0 // LIGHT GRID OPAQUE
    return heatmap(light_grid_opaque, position.xy, .75f);
    
    #elif 1 // SCENE
    Texture2D gpass_main = ResourceDescriptorHeap[shader_params.gpass_main_buffer_index];
    return float4(gpass_main[position.xy].xyz, 1.f);
    #endif
}