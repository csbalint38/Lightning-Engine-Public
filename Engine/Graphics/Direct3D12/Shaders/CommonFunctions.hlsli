#if !defined(LIGHTNING_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not include this header directly in shader files. Only include this file via Common.hlsli.
#endif

Plane compute_plane(float3 p0, float3 p1, float3 p2)
{
    Plane plane;

    const float3 v0 = p1 - p0;
    const float3 v2 = p2 - p0;

    plane.normal = normalize(cross(v0, v2));
    plane.distance = dot(plane.normal, p0);

    return plane;
}

bool point_inside_plane(float3 p, Plane plane)
{
    return dot(plane.normal, p) - plane.distance < 0;
}

bool sphere_inside_plane(Sphere sphere, Plane plane)
{
    return dot(plane.normal, sphere.center) - plane.distance < -sphere.radius;
}

bool cone_inside_plane(Cone cone, Plane plane)
{
    float3 m = cross(cross(plane.normal, cone.direction), cone.direction);
    float3 Q = cone.tip + cone.direction * cone.height - m * cone.radius;

    return point_inside_plane(cone.tip, plane) && point_inside_plane(Q, plane);
}

#if !USE_BOUNDING_SPHERES
bool sphere_inside_frustum(Sphere sphere, Frustum frustum, float near_z, float far_z)
{
    return !((sphere.center.z - sphere.radius > near_z || sphere.center.z + sphere.radius < far_z) || sphere_inside_plane(sphere, frustum.planes[0]) || sphere_inside_plane(sphere, frustum.planes[1]) || sphere_inside_plane(sphere, frustum.planes[2]) || sphere_inside_plane(sphere, frustum.planes[3]));
}

bool cone_inside_frustum(Cone cone, Frustum frustum, float near_z, float far_z)
{

    Plane near_plane = { float3(0, 0, -1), -near_z };
    Plane far_plane = { float3(0, 0, 1), far_z };

    if (cone_inside_plane(cone, near_plane) || cone_inside_plane(cone, far_plane))
    {
        return false;
    }

    for (int i = 0; i < 4; i++)
    {
        if (cone_inside_plane(cone, frustum.planes[i]))
        {
            return false;
        }
    }

    return true;
}
#endif

float4 unproject_uv(float2 uv, float depth, float4x4 inverse)
{
    float4 clip = float4(float2(uv.x, 1.f - uv.y) * 2.f - 1.f, depth, 1.f);
    float4 position = mul(inverse, clip);
    
    return position / position.w;
}

float4 clip_to_view(float4 clip, float4x4 inverse_projection)
{
    float4 view = mul(inverse_projection, clip);
    return view / view.w;
}

float4 screen_to_view(float4 screen, float2 inv_view_dimensions, float4x4 inverse_projection)
{
    // Convert to normalized texture coordinates
    float2 tex_coord = screen.xy * inv_view_dimensions;

    // Convert to clip space
    float4 clip = float4(float2(tex_coord.x, 1.f - tex_coord.y) * 2.f - 1.f, screen.z, screen.w);

    return clip_to_view(clip, inverse_projection);
}