#include "Common.hlsli"

#if USE_BOUNDING_SPHERES

static const uint max_lights_per_group = 1024;

groupshared uint _min_depth_vs;
groupshared uint _max_depth_vs;
groupshared uint _light_count;
groupshared uint _light_index_start_offset;
groupshared uint _light_index_list[max_lights_per_group];
groupshared uint _light_flags_opaque[max_lights_per_group];
groupshared uint _spotlight_start_offset;
groupshared uint2 _opaque_light_index;

ConstantBuffer<GlobalShaderData> global_data : register(b0, space0);
ConstantBuffer<LightCullingDispatchParameters> shader_params : register(b1, space0);

StructuredBuffer<Frustum> frustums : register(t0, space0);
StructuredBuffer<LightCullingLightInfo> lights : register(t1, space0);
StructuredBuffer<Sphere> bounding_spheres : register(t2, space0);

RWStructuredBuffer<uint> light_index_counter : register(u0, space0);
RWStructuredBuffer<uint2> light_grid_opaque : register(u1, space0);
RWStructuredBuffer<uint> light_index_list_opaque : register(u3, space0);

Sphere get_cone_bounding_sphere(float3 tip, float range, float3 direction, float cos_penumbra)
{
    Sphere sphere;
    sphere.radius = range / (2.f * cos_penumbra);
    sphere.center = tip + sphere.radius * direction;

    if (cos_penumbra < .707107f) // cos(45�) = 0.707107
    {
        // sin^2(x) + cos^2(x) = 1 => sin(x) = sqrt(1 - cos^2(x))
        const float cone_sin = sqrt(1.f - cos_penumbra * cos_penumbra);
        sphere.center = tip + cos_penumbra * range * direction;
        sphere.radius = cone_sin * range;
    }
    
    return sphere;
}

bool intersects(Frustum frustum, Sphere sphere, float min_depth, float max_depth)
{
    if ((sphere.center.z - sphere.radius > min_depth) || (sphere.center.z + sphere.radius < max_depth))
    {
        return false;
    }

    const float3 light_rejection = sphere.center - dot(sphere.center, frustum.cone_direction) * frustum.cone_direction;
    const float dist_sq = dot(light_rejection, light_rejection);
    const float radius = sphere.center.z * frustum.unit_radius + sphere.radius;
    const float radius_sq = radius * radius;
    
    return dist_sq <= radius_sq;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void cull_lights_cs(ComputeShaderInput cs_in)
{
    // INITIALIZATION
    //
    // Coordinate system: right-handed
    //
    // Major projection matrices:
    //
    //  Projection                  Inverse projection:
    //  |   A   0   0   0   |       |   1/A 0   0   0   |
    //  |   0   B   0   0   |       |   0   1/B 0   0   |
    //  |   0   0   C   D   |       |   0   0   0   -1  |
    //  |   0   0   -1  0   |       |   0   0   1/D C/D |
    //
    // Transform position vector v from clip t oview space:
    //
    // q = mul(inverse_projection, v);
    // v_view_space = q / q.w;
    //
    // z-component of v_view_space
    //
    // v_view_space = -D / (depth + C)
    
    const float depth = Texture2D( ResourceDescriptorHeap[shader_params.depth_buffer_srv_index])[cs_in.dispatch_thread_id.xy].r;
    const float c = global_data.projection._m22;
    const float d = global_data.projection._m23;
    const uint grid_index = cs_in.group_id.x + (cs_in.group_id.y * shader_params.num_thread_groups.x);
    const Frustum frustum = frustums[grid_index];
    
    if (cs_in.group_index == 0)
    {
        _min_depth_vs = 0x7f7fffff;
        _max_depth_vs = 0;
        _light_count = 0;
        _opaque_light_index = 0;
    }

    uint i = 0, index = 0;
    
    for (i = cs_in.group_index; i < max_lights_per_group; i += TILE_SIZE * TILE_SIZE)
    {
        _light_flags_opaque[i] = 0;
    }

    // DEPTH MIN/MAX
        GroupMemoryBarrierWithGroupSync();

    if (depth != 0)
    {
        const float depth_min = WaveActiveMax(depth);
        const float depth_max = WaveActiveMin(depth);

        if (WaveIsFirstLane())
        {
            const uint z_min = asuint(d / (depth_min + c));
            const uint z_max = asuint(d / (depth_max + c));
            InterlockedMin(_min_depth_vs, z_min);
            InterlockedMax(_max_depth_vs, z_max);
        }
    }

    // LIGHT CULLING
    GroupMemoryBarrierWithGroupSync();

    const float min_depth_vs = -asfloat(_min_depth_vs);
    const float max_depth_vs = -asfloat(_max_depth_vs);

    for (i = cs_in.group_index; i < shader_params.num_lights; i += TILE_SIZE * TILE_SIZE)
    {
        
        Sphere sphere = bounding_spheres[i];
        sphere.center = mul(global_data.view, float4(sphere.center, 1.f)).xyz;
        
        if (intersects(frustum, sphere, min_depth_vs, max_depth_vs))
        {
            InterlockedAdd(_light_count, 1, index);
            if (index < max_lights_per_group)
            {
                _light_index_list[index] = i;
            }
        }
    }

    // LIGHT_PRUNING
    GroupMemoryBarrierWithGroupSync();

    const uint light_count = min(_light_count, max_lights_per_group);
    const float2 inv_view_dimension = 1.f / float2(global_data.view_width, global_data.view_height);
    const float3 pos = unproject_uv(cs_in.dispatch_thread_id.xy * inv_view_dimension, depth, global_data.inv_view_projection).xyz;
    
    for (i = 0; i < light_count; ++i)
    {
        index = _light_index_list[i];
        const LightCullingLightInfo light = lights[index];
        const float3 d = pos - light.position;
        const float dist_sq = dot(d, d);

        if (dist_sq <= light.range * light.range)
        {
            const bool is_point_light = light.cos_penumbra == -1.f;
            
            if (is_point_light || (dot(d * rsqrt(dist_sq), light.direction) >= light.cos_penumbra))
            {
                _light_flags_opaque[i] = 2 - uint(is_point_light);

            }
        }
    }
    
    // UPDATE_LIGHT_GRID
    GroupMemoryBarrierWithGroupSync();
    
    if (cs_in.group_index == 0)
    {
        uint num_point_lights = 0;
        uint num_spotlights = 0;
        
        for (i = 0; i < light_count; ++i)
        {
            num_point_lights += (_light_flags_opaque[i] & 1);
            num_spotlights += (_light_flags_opaque[i] >> 1);
        }
        
        InterlockedAdd(light_index_counter[0], num_point_lights + num_spotlights, _light_index_start_offset);
        _spotlight_start_offset = _light_index_start_offset + num_point_lights;
        light_grid_opaque[grid_index] = uint2(_light_index_start_offset, (num_point_lights << 16) | num_spotlights);
    }

    // UPDATE LIGHT INDEX LIST
    GroupMemoryBarrierWithGroupSync();

    uint point_index, spot_index;
    
    for (i = cs_in.group_index; i < light_count; i += TILE_SIZE * TILE_SIZE)
    {
        if (_light_flags_opaque[i] == 1)
        {
            InterlockedAdd(_opaque_light_index.x, 1, point_index);
            light_index_list_opaque[_light_index_start_offset + point_index] = _light_index_list[i];
        }
        else if (_light_flags_opaque[i] == 2)
        {
            InterlockedAdd(_opaque_light_index.y, 1, spot_index);
            light_index_list_opaque[_spotlight_start_offset + spot_index] = _light_index_list[i];
        }
    }
}

#else
static const uint max_lights_per_group = 1024;

groupshared uint _min_depth_vs;
groupshared uint _max_depth_vs;
groupshared uint _light_count;
groupshared uint _light_index_start_offset;
groupshared uint _light_index_list[max_lights_per_group];

ConstantBuffer<GlobalShaderData> global_data : register(b0, space0);
ConstantBuffer<LightCullingDispatchParameters> shader_params : register(b1, space0);
StructuredBuffer<Frustum> frustums : register(t0, space0);
StructuredBuffer<LightCullingLightInfo> lights : register(t1, space0);

RWStructuredBuffer<uint> light_index_counter : register(u0, space0);
RWStructuredBuffer<uint2> light_grid_opaque : register(u1, space0);
RWStructuredBuffer<uint> light_index_list_opaque : register(u3, space0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void cull_lights_cs(ComputeShaderInput cs_in)
{
    if (cs_in.group_index == 0)
    {
        _min_depth_vs = 0x7f7fffff;
        _max_depth_vs = 0;
        _light_count = 0;
    }

    uint i = 0, index = 0;

    GroupMemoryBarrierWithGroupSync();

    const float depth = Texture2D(ResourceDescriptorHeap[shader_params.depth_buffer_srv_index])[cs_in.dispatch_thread_id.xy].r;
    const float depth_vs = clip_to_view(float4(0.f, 0.f, depth, 1.f), global_data.inverse_projection).z;
    const uint z = asuint(-depth_vs);

    if (depth != 0)
    {
        InterlockedMin(_min_depth_vs, z);
        InterlockedMax(_max_depth_vs, z);
    }

    GroupMemoryBarrierWithGroupSync();

    const uint grid_index = cs_in.group_id.x + (cs_in.group_id.y * shader_params.num_thread_groups.x);
    const Frustum frustum = frustums[grid_index];
    const float min_depth_vs = -asfloat(_min_depth_vs);
    const float max_depth_vs = -asfloat(_max_depth_vs);

    for (i = cs_in.group_index; i < shader_params.num_lights; i += TILE_SIZE * TILE_SIZE)
    {
        const LightCullingLightInfo light = lights[i];
        const float3 light_position_vs = mul(global_data.view, float4(light.position, 1.f)).xyz;

        if (light.type == LIGHT_TYPE_POINT_LIGHT)
        {
            const Sphere sphere = { light_position_vs, light.range };
            if (sphere_inside_frustum(sphere, frustum, min_depth_vs, max_depth_vs))
            {
                InterlockedAdd(_light_count, 1, index);
                if (index < max_lights_per_group)
                    _light_index_list[index] = i;
            }
        }
        else if (light.type == LIGHT_TYPE_SPOTLIGHT)
        {
            const float3 light_direction_vs = mul(global_data.view, float4(light.direction, 0.f)).xyz;
            const Cone cone = { light_position_vs, light.range, light_direction_vs, light.cone_radius };
            if (cone_inside_frustum(cone, frustum, min_depth_vs, max_depth_vs))
            {
                InterlockedAdd(_light_count, 1, index);
                if (index < max_lights_per_group)
                    _light_index_list[index] = i;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    const uint light_count = min(_light_count, max_lights_per_group);

    if (cs_in.group_index == 0)
    {
        InterlockedAdd(light_index_counter[0], light_count, _light_index_start_offset);
        light_grid_opaque[grid_index] = uint2(_light_index_start_offset, light_count);
    }

    GroupMemoryBarrierWithGroupSync();

    for (i = cs_in.group_index; i < light_count; i += TILE_SIZE * TILE_SIZE)
    {
        light_index_list_opaque[_light_index_start_offset + i] = _light_index_list[i];
    }
}
#endif