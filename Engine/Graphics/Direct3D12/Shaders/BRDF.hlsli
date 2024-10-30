float pow5(float x)
{
    float xx = x * x;
    
    return xx * xx * x;
}

float d_ggx(float n_o_h, float a)
{
    float d = (n_o_h * a - n_o_h) * n_o_h + 1;
    
    return a / (PI * d * d);
}

float v_Smith_ggx_correlated(float n_o_v, float n_o_l, float a)
{
    float ggxl = n_o_v * sqrt((-n_o_l * a + n_o_l) * n_o_l + a);
    float ggxv = n_o_l * sqrt((-n_o_v * a + n_o_v) * n_o_v + a);

    return .5f / (ggxl + ggxv);
}

float v_Smith_ggx_correlated_approx(float n_o_v, float n_o_l, float a)
{
    float ggxv = n_o_l * ((-n_o_v * a + n_o_v) + a);
    float ggxl = n_o_v * ((-n_o_l * a + n_o_l) + a);

    return .5f / (ggxv + ggxl);
}

float3 f_Schlick(float3 f0, float v_o_h)
{
    float u = pow5(1.f - v_o_h);
    float3 f90_approx = saturate(50.f * f0.g);
    
    return f90_approx * u + (1 - u) * f0;
}

float3 f_Schlick(float u, float3 f0, float3 f90)
{
    return f0 + (f90 - f0) * pow5(1.f - u);
}

float3 diffuse_Lambert()
{
    return 1 / PI;
}

float3 diffuse_Burley(float n_o_v, float n_o_l, float v_o_h, float roughness)
{
    float u = pow5(1.f - n_o_v);
    float v = pow5(1.f - n_o_l);
    
    float fd90 = .5f + 2.f * v_o_h * v_o_h * roughness;
    float f_d_v = 1.f + (u * fd90 - u);
    float f_d_l = 1.f + (v * fd90 - v);

    return (1.f / PI) * f_d_v * f_d_l;

}