// The fullscreen triangle of the Direct3D 11 passes (AntiAliasingD3D11.hpp): no vertex buffer, three vertices from
// SV_VertexID. TEXCOORD0 comes first, where the pixel shaders (shaders/*.hlsl, compiled with /Gec) expect it.

struct Output
{
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(uint id : SV_VertexID)
{
    Output o;
    o.uv = float2((id << 1) & 2, id & 2);
    o.position = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}
