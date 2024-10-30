struct VSOutput
{
    noperspective float4 position : SV_Position;
    noperspective float2 uv : TEXCOORD;
};

VSOutput fullscreen_triangle_vs(in uint VertexIdx : SV_VertexID)
{
    VSOutput output;
    /*
    float2 tex;
    float2 pos;
    
    if (VertexIdx == 0)
    {
        tex = float2(0, 0);
        pos = float2(-1, 1);
    }
    else if (VertexIdx == 1)
    {
        tex = float2(0, 2);
        pos = float2(-1, -3);
    }
    else if (VertexIdx == 2)
    {
        tex = float2(2, 0);
        pos = float2(3, 1);
    }
    
    output.position = float4(pos, 0, 1);
    output.uv = tex;
    */
    const float2 tex = float2(uint2(VertexIdx, VertexIdx << 1) & 2);
    output.position = float4(lerp(float2(-1, 1), float2(1, -1), tex), 0, 1);
    output.uv = tex;
    
    return output;
}