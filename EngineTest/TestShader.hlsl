#include "Common.hlsli"
#include "BRDF.hlsli"

struct VertexOut {
    float4 homogeneous_position : SV_POSITION;
    float3 world_position : POSITIONT;
    float3 world_normal : NORMAL;
    float4 world_tangent : TANGENT;
    float2 uv : TEXTURE;
};

struct PixelOut {
    float4 color : SV_TARGET0;
};

struct Surface
{
    float3 base_color;
    float metallic;
    float3 normal;
    float perceptual_roughness;
    float3 emissive_color;
    float emissive_intensity;
    float3 v;                   // View direction
    float ambient_occlusion;
    float3 diffuse_color;
    float a2;                   // perceptual_roughness^4
    float3 specular_color;
    float n_o_v;
};

#define ELEMENTS_TYPE_POSITION_ONLY 0x00
#define ELEMENTS_TYPE_STATIC_NORMAL 0x01
#define ELEMENTS_TYPE_STATIC_NORMAL_TEXTURE 0x03
#define ELEMENTS_TYPE_STATIC_COLOR 0x04
#define ELEMENTS_TYPE_SKELETAL 0x08
#define ELEMENTS_TYPE_SKELETAL_COLOR ELEMENTS_TYPE_SKELETAL | ELEMENTS_TYPE_STATIC_COLOR
#define ELEMENTS_TYPE_SKELETAL_NORMAL ELEMENTS_TYPE_SKELETAL | ELEMENTS_TYPE_STATIC_NORMAL
#define ELEMENTS_TYPE_SKELETAL_NORMAL_COLOR ELEMENTS_TYPE_SKELETAL_NORMAL | ELEMENTS_TYPE_STATIC_COLOR
#define ELEMENTS_TYPE_SKELETAL_NORMAL_TEXTURE ELEMENTS_TYPE_SKELETAL | ELEMENTS_TYPE_STATIC_NORMAL_TEXTURE
#define ELEMENTS_TYPE_SKELETAL_NORMAL_TEXTURE_COLOR ELEMENTS_TYPE_SKELETAL_NORMAL_TEXTURE | ELEMENTS_TYPE_STATIC_COLOR

struct VertexElement
{
    #if ELEMENTS_TYPE == ELEMENTS_TYPE_STATIC_NORMAL
    uint color_t_sign;
    uint16_t2 normal;
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_STATIC_NORMAL_TEXTURE
    uint color_t_sign;
    uint16_t2 normal;
    uint16_t2 tangent;
    float2 uv;
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_STATIC_COLOR
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_SKELETAL
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_SKELETAL_NORMAL
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_SKELETAL_NORMAL_COLOR
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_SKELETAL_NORMAL_TEXTURE
    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_SKELETAL_NORMAL_TEXTURE_COLOR
    #endif
};

const static float inv_intervals = 2.f / ((1 << 16) - 1);

ConstantBuffer<GlobalShaderData> global_data : register(b0, space0);
ConstantBuffer<PerObjectData> per_object_buffer : register(b1, space0);

StructuredBuffer<float3> vertex_positions : register(t0, space0);
StructuredBuffer<VertexElement> elements : register(t1, space0);
StructuredBuffer<uint> srv_indicies : register(t2, space0);
StructuredBuffer<DirectionalLightParameters> directional_lights : register(t3, space0);
StructuredBuffer<LightParameters> cullable_lights : register(t4, space0);
StructuredBuffer<uint2> light_grid : register(t5, space0);
StructuredBuffer<uint> light_index_list : register(t6, space0);

SamplerState point_sampler : register(s0, space0);
SamplerState linear_sampler : register(s1, space0);
SamplerState anisotropic_sampler : register(s2, space0);

VertexOut test_shader_vs(in uint vertex_idx : SV_VertexID) {
    VertexOut vs_out;
    
    float4 position = float4(vertex_positions[vertex_idx], 1.f);
    float4 world_position = mul(per_object_buffer.world, position);
    
    #if ELEMENTS_TYPE == ELEMENTS_TYPE_STATIC_NORMAL
    VertexElement element = elements[vertex_idx];
    float2 n_xy = element.normal * inv_intervals - 1.f;
    uint signs = element.color_t_sign >> 24;
    float n_sign = float((signs & 0x04) >> 1) - 1.f;
    float3 normal = float3(n_xy, sqrt(saturate(1.f - dot(n_xy, n_xy))) * n_sign);
    
    vs_out.homogeneous_position = mul(per_object_buffer.world_view_projection, position);
    vs_out.world_position = world_position.xyz;
    vs_out.world_normal = mul(float4(normal, 0.f), per_object_buffer.inv_world).xyz;
    vs_out.world_tangent = 0.f;
    vs_out.uv = 0.f;

    #elif ELEMENTS_TYPE == ELEMENTS_TYPE_STATIC_NORMAL_TEXTURE
    VertexElement element = elements[vertex_idx];
    uint signs = element.color_t_sign >> 24;
    float n_sign = float((signs & 0x04) >> 1) - 1.f;
    float t_sign = float(signs & 0x02) - 1.f;
    float h_sign = float((signs & 0x01) << 1) - 1.f;
    
    float2 n_xy = element.normal * inv_intervals - 1.f;
    float3 normal = float3(n_xy, sqrt(saturate(1.f - dot(n_xy, n_xy))) * n_sign);
    
    float2 t_xy = element.tangent * inv_intervals - 1.f;
    float3 tangent = float3(t_xy, sqrt(saturate(1.f - dot(t_xy, t_xy))) * t_sign);
    tangent = tangent - normal * dot(normal, tangent);
    
    vs_out.homogeneous_position = mul(per_object_buffer.world_view_projection, position);
    vs_out.world_position = world_position.xyz;
    vs_out.world_normal = normalize(mul(normal, (float3x3)per_object_buffer.inv_world));
    vs_out.world_tangent = float4(normalize(mul(tangent, (float3x3)per_object_buffer.inv_world)), h_sign);
    vs_out.uv = element.uv;
    
    #else
    #undef ELEMENTS_TYPE
    vs_out.homogeneous_position = mul(per_object_buffer.world_view_projection, position);
    vs_out.world_position = world_position.xyz;
    vs_out.world_normal = 0.f;
    vs_out.world_tangent = 0.f;
    vs_out.uv = 0.f;
    #endif

    return vs_out;
}

#define TILE_SIZE 32
#define NO_LIGHT_ATTENUATION 0  // Turn on to test light edges
#define USE_PHONG 0             // Turn on to use old-style Phong shading
#define USE_BURLEY_BRDF 1       // Turn on to achive more precise lighting
// If both USE_PHONG and USE_BURLEY_BRDF is off, the shader uses physic-based Lambert shading

float4 sample(uint index, SamplerState s, float2 uv)
{
    return Texture2D( ResourceDescriptorHeap[index]).Sample(s, uv);
}

float3 PhongBRDF(float3 n, float3 l, float3 v, float3 diffuse_color, float3 specular_color, float shininess)
{
    float3 color = diffuse_color;
    const float3 r = reflect(-l, n);
    const float v_o_r = max(dot(v, r), 0.f);
    color += pow(v_o_r, max(shininess, 1.f)) * specular_color;
    
    return color;
}

float3 Cook_Torrence_BRDF(Surface s, float3 l)
{
    const float3 n = s.normal;
    const float3 h = normalize(s.v + l);
    const float n_o_v = abs(s.n_o_v) + 1e-5;
    const float n_o_l = saturate(dot(n, l));
    const float n_o_h = saturate(dot(n, h));
    const float v_o_h = saturate(dot(s.v, h));
    
    const float d = d_ggx(n_o_h, s.a2);
    const float g = v_Smith_ggx_correlated(n_o_v, n_o_l, s.a2);
    const float3 f = f_Schlick(s.specular_color, v_o_h);
    
    float3 specular_brdf = (d * g) * f;
    float3 rho = 1.f - f;
    
    #if USE_BURLEY_BRDF
    float3 diffuse_brdf = diffuse_Burley(n_o_v, n_o_l, v_o_h, s.perceptual_roughness * s.perceptual_roughness) * s.diffuse_color * rho;
    #else
    float3 diffuse_brdf = diffuse_Lambert() * s.diffuse_color * rho;
    #endif
    
    return (diffuse_brdf + specular_brdf) * n_o_l;
}

float3 calculate_lighting(Surface s, float3 l, float3 light_color)
{
    float3 color = 0;
    
    #if USE_PHONG
    const float3 n = s.normal;
    const float n_o_l = saturate(dot(n, l));
    color = PhongBRDF(n, l, s.v, s.base_color, 1.f, (1 - s.perceptual_roughness) * 100.f) * (n_o_l / PI) * light_color;
    #else
    color = Cook_Torrence_BRDF(s, l) * light_color;
    #endif
    
    color *= PI;

    return color;
}

float3 point_light(Surface s, float3 world_position, LightParameters light)
{
    float3 l = light.position - world_position;
    const float d_sq = dot(l, l);
    float3 color = 0.f;
    
    #if NO_LIGHT_ATTENUATION
    float3 n = s.normal;
    if(d_sq < light.range * light.range) {
        const float d_rcp = rsqrt(d_sq);
        l *= d_rcp;
        color = saturate(dot(n, l) * light.color * light.intensity * .05f);
    }
    return color;
    #else
    if (d_sq < light.range * light.range)
    {
        const float d_rcp = rsqrt(d_sq);
        l *= d_rcp;
        const float attenuation = 1.f - smoothstep(.1f * light.range, light.range, rcp(d_rcp));
        color = calculate_lighting(s, l, light.color * light.intensity * attenuation);
    }
    return color;
    #endif
}

float3 spotlight(Surface s, float3 world_position, LightParameters light)
{
    float3 l = light.position - world_position;
    const float d_sq = dot(l, l);
    float3 color = 0.f;
    
    #if NO_LIGHT_ATTENUATION
    float3 n = s.normal;
    
    if(d_sq < light.range * light.range) {
        const float d_rcp = rsqrt(d_sq);
        l *= d_rcp;
        const float cos_angle_to_light = saturate(dot(-l, light.direction)); 
        const float angular_attenuation = float(light.cos_penumbra < cos_angle_to_light); 
        color = saturate(dot(n, l) * light.color * light.intensity * angular_attenuation * .01f);
    }
    return color;
    #else
    
    if (d_sq < light.range * light.range)
    {
        const float d_rcp = rsqrt(d_sq);
        l *= d_rcp;
        const float attenuation = 1.f - smoothstep(.1f * light.range, light.range, rcp(d_rcp));
        const float cos_angle_to_light = saturate(dot(-l, light.direction));
        const float angular_attenuation = smoothstep(light.cos_penumbra, light.cos_umbra, cos_angle_to_light);
        color = calculate_lighting(s, l, light.intensity * attenuation * angular_attenuation);

    }
    
    return color;
    #endif
}

Surface get_surface(VertexOut ps_in, float3 v)
{   
    Surface s;
    
    s.base_color = per_object_buffer.base_color.rgb;
    s.metallic = per_object_buffer.metallic;
    s.normal = normalize(ps_in.world_normal);
    s.perceptual_roughness = max(per_object_buffer.roughness, .045f);
    s.emissive_color = per_object_buffer.emissive;
    s.emissive_intensity = per_object_buffer.emissive_intensity;
    s.ambient_occlusion = per_object_buffer.ambient_occlusion;
    
    #if TEXTURED_MTL
    float2 uv = ps_in.uv;
    s.ambient_occlusion = sample(srv_indicies[0], linear_sampler, uv).r;
    s.base_color = sample(srv_indicies[1], linear_sampler, uv).rgb;
    s.emissive_color = sample(srv_indicies[2], linear_sampler, uv).rgb;
    float2 metal_rough = sample(srv_indicies[3], linear_sampler, uv).rg;
    s.metallic = metal_rough.r;
    s.perceptual_roughness = max(metal_rough.g, .045f);
    s.emissive_intensity = 1.f;
    float3 n = sample(srv_indicies[4], linear_sampler, uv).rgb;
    n = n * 2.f - 1.f;
    n.z = sqrt(1.f - saturate(dot(n.xy, n.xy)));
    
    const float3 N = ps_in.world_normal;
    const float3 T = ps_in.world_tangent.xyz;
    const float3 B = cross(N, T) * ps_in.world_tangent.w;
    const float3x3 TBN = float3x3(T, B, N);
    
    s.normal = normalize(mul(n, TBN));
    #endif
    
    s.v = v;
    const float roughness = s.perceptual_roughness * s.perceptual_roughness;
    s.a2 = roughness * roughness;
    s.n_o_v = dot(v, s.normal);
    s.diffuse_color = s.base_color * (1.f - s.metallic);
    s.specular_color = lerp(.04f, s.base_color, s.metallic);
    
    return s;
}

uint get_grid_index(float2 pos_xy, float view_width)
{
    const uint2 pos = uint2(pos_xy);
    const uint tiles_x = ceil(view_width / TILE_SIZE);
    return (pos.x / TILE_SIZE) + (tiles_x * (pos.y / TILE_SIZE));
}

[earlydepthstencil]
PixelOut test_shader_ps(in VertexOut ps_in) {
    float3 view_dir = normalize(global_data.camera_position - ps_in.world_position);
    Surface s = get_surface(ps_in, view_dir);
    
    float3 color = 0;
    
    uint i = 0;
    for (i = 0; i < global_data.num_directional_lights; ++i)
    {
        DirectionalLightParameters light = directional_lights[i];
        
        float3 light_direction = light.direction;
        
        /*
        if (abs(light_direction.z - 1.f) < .001f)
        {
            light_direction = global_data.camera_direction;
        }
        */
        
        color += calculate_lighting(s, -light_direction, light.color * light.intensity);
    }
    
    const uint grid_index = get_grid_index(ps_in.homogeneous_position.xy, global_data.view_width);
    uint light_start_index = light_grid[grid_index].x;
    const uint light_count = light_grid[grid_index].y;
    
    #if USE_BOUNDING_SPHERES
    const uint num_point_lights = light_start_index + (light_count >> 16);
    const uint num_spotlights = num_point_lights + (light_count & 0xffff);
    
    for(i = light_start_index; i < num_point_lights; ++i) {
        const uint light_index = light_index_list[i];
        LightParameters light = cullable_lights[light_index];
        color += point_light(s, ps_in.world_position, light);
    }
    
    for(i = num_point_lights; i < num_spotlights; ++i) {
        const uint light_index = light_index_list[i];
        LightParameters light = cullable_lights[light_index];
        color += spotlight(s, ps_in.world_position, light);
    }
    #else
    for (i = 0; i < light_count; ++i)
    {
        const uint light_index = light_index_list[light_start_index + i];
        LightParameters light = cullable_lights[light_index];
        
        if (light.type == LIGHT_TYPE_POINT_LIGHT)
        {
            color += point_light(s, ps_in.world_position, light);
        }
        else if (light.type == LIGHT_TYPE_SPOTLIGHT)
        {
            color += spotlight(s, ps_in.world_position, light);
        }

    }
    #endif
    
    #if TEXTURED_MTL
    float v_o_n = s.n_o_v * 1.3f;
    float v_o_n_2 = v_o_n * v_o_n;
    float v_o_n_4 = v_o_n_2 * v_o_n_2;
    float3 e = s.emissive_color;
    s.emissive_color = max(v_o_n_4 * v_o_n_4, .1f) * e * e;
    
    #endif
    
    PixelOut ps_out;
    ps_out.color = float4(color * s.ambient_occlusion + s.emissive_color * s.emissive_intensity, 1.f);

    return ps_out;
}