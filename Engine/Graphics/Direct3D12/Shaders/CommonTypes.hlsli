#if !defined(LIGHTNING_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not include this header in shader files directly. Include Common.hlsli instead.
#endif

#define USE_BOUNDING_SPHERES 1

struct GlobalShaderData
{
    float4x4 view;
    float4x4 projection;
    float4x4 inverse_projection;
    float4x4 view_projection;
    float4x4 inv_view_projection;
    
    float3 camera_position;
    float view_width;
    
    float3 camera_direction;
    float view_height;
    
    uint num_directional_lights;
    
    float delta_time;
};

struct PerObjectData
{
    float4x4 world;
    float4x4 inv_world;
    float4x4 world_view_projection;
    float4 base_color;
    float3 emissive;
    float emissive_intensity;
    float ambient_occlusion;
    float metallic;
    float roughness;
    uint _pad;
};

struct Plane
{
    float3 normal;
    float distance;
};

struct Sphere
{
    float3 center;
    float radius;
};

struct Cone
{
    float3 tip;
    float height;
    float3 direction;
    float radius;
};

#if USE_BOUNDING_SPHERES
struct Frustum
{
    float3 cone_direction;
    float unit_radius;
};
#else
struct Frustum
{
    Plane planes[4];
};
#endif

#ifndef __cplusplus
struct ComputeShaderInput
{
    uint3 group_id : SV_GroupID;
    uint3 group_thread_id : SV_GroupThreadID;
    uint3 dispatch_thread_id : SV_DispatchThreadID;
    uint group_index : SV_GroupIndex;
};
#endif

struct LightCullingDispatchParameters
{
    uint2 num_thread_groups;
    uint2 num_threds;
    
    uint num_lights;
    uint depth_buffer_srv_index;
};

struct LightCullingLightInfo
{
    float3 position;
    float range;
    float3 direction;
    
    #if USE_BOUNDING_SPHERES
    float cos_penumbra;
    #else
    float cone_radius;
    
    uint type;
    float3 _pad;
    #endif
};

struct LightParameters {
    float3 position;
    float intensity;
    float3 direction;
    float range;
    float3 color;
    float cos_umbra;
    float3 attenuation;
    float cos_penumbra; // If this is -1 light type is POINT
    
    #if !USE_BOUNDING_SPHERES
    uint type;
    float3 _pad;
    #endif
};

struct DirectionalLightParameters
{
    float3 direction;
    float intensity;
    float3 color;
    float _pad;
};

#ifdef __cplusplus
static_assert((sizeof(PerObjectData) % 16) == 0, "Make sure PerObjectData is formatted in 16-byte chunks without any implicit padding.");
static_assert((sizeof(PerObjectData) % 16) == 0, "Make sure LightParameters is formatted in 16-byte chunks without any implicit padding.");
static_assert((sizeof(LightCullingLightInfo) % 16) == 0, "Make sure LightCullingInfo is formatted in 16-byte chunks without any implicit padding.");
static_assert((sizeof(DirectionalLightParameters) % 16) == 0, "Make sure DirectionalLightParameters is formatted in 16-byte chunks without any implicit padding.");
#endif