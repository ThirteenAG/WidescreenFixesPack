#include "../../includes/postfx/postfx.fx"

// -------------------------------------------------------------
// NFS Most Wanted fast gamma approximation: polynomial fit of the
// Xbox 360 gamma LUT, far cheaper than a 256-entry LUT lookup.
float MWGammaApproxFast(float x)
{
    x = saturate(x);
    float x2 = x * x;
    float x3 = x2 * x;
    float p = -0.184189 + 1.668233 * x - 3.0067 * x2 + 1.694186 * x3;
    return x + x * (1.0 - x) * p;
}

float4 MWGammaPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 color = tex2D(InputTex, uv).rgb;
    return float4(MWGammaApproxFast(color.r),
                  MWGammaApproxFast(color.g),
                  MWGammaApproxFast(color.b), 1.0);
}

technique MWGamma
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 MWGammaPS();
        ZEnable = 0;
        ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
        ScissorTestEnable = False;
    }
}
