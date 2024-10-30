#define M_RE_START -2.8f
#define M_RE_END 1.f
#define M_IM_START -1.5f
#define M_IM_END 1.5f
#define M_MAX_ITERATION 1000

#define J_RE_START -2.f
#define J_RE_END 2.f
#define J_IM_START -1.5f
#define J_IM_END 1.5f
#define J_MAX_ITERATION 1000


float3 map_color(float t)
{
    float3 ambient = float3(0.009f, 0.012f, 0.016f);
    return float3(2.f * t, 4.f * t, 8.f * t) + ambient;

}

float2 complex_sq(float2 c)
{
    return float2(c.x * c.x - c.y * c.y, 2.f * c.x * c.y);

}

float3 draw_mandelbrot(float2 uv)
{
    const float2 c = float2(M_RE_START + uv.x * (M_RE_END - M_RE_START), M_IM_START + uv.y * (M_IM_END - M_IM_START));
    float2 z = 0.f;
    for (int i = 0; i < M_MAX_ITERATION; i++)
    {
        z = complex_sq(z) + c;
        const float d = dot(z, z);
        if (d > 4.f)
        {
            const float t = i + 1 - log(log2(d));
            return map_color(t / M_MAX_ITERATION);
        }
    }
    return 1.f;
}

float3 draw_julia_set(float2 uv, uint frame)
{
    float2 z = float2(J_RE_START + uv.x * (J_RE_END - J_RE_START), J_IM_START + uv.y * (J_IM_END - J_IM_START));
    const float f = frame * 0.0002f;
    const float2 w = float2(cos(f), sin(f));
    const float2 c = (2.f * w - complex_sq(w)) * 0.26f;
    
    for (int i = 0; i < J_MAX_ITERATION; i++)
    {
        z = complex_sq(z) + c;
        const float d = dot(z, z);
        if (d > 4.f)
        {
            const float t = i + 1 - log(log2(d));
            return map_color(t / J_MAX_ITERATION);
        }
    }
    return 1.f;

}

float3 draw_hearth(float2 uv, uint frame)
{
    uv = uv * 2.0 - 1.0;
    
    float scale = 0.3 + 0.3 * sin(frame * 0.01);
    uv /= scale;
    uv = - uv;
    
    float x = uv.x;
    float y = uv.y;
    float heart = (x * x + y * y - 1) * (x * x + y * y - 1) * (x * x + y * y - 1) - x * x * y * y * y;

    if (heart <= 0.0)
    {
        return float3(1.0, 0.11, 0.56);;
    }
    else
    {
        return float3(0.06, 0.13, 0.22);
    }
}
