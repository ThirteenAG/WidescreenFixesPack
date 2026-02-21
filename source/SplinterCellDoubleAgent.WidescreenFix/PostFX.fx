void FullscreenVS(in float4 iPosition : POSITION, in float2 iTexcoord : TEXCOORD0,
        out float4 oPosition : POSITION, out float2 oTexcoord : TEXCOORD0)
{
    oPosition = iPosition;
    oTexcoord = iTexcoord;
}

texture2D InputTex2D;

sampler2D InputTex
{
    Texture = <InputTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
};

float X360GammaApprox(float x)
{
    float A = 0.541901f;
    float B = 1.13465f;
    float C = 13.53054f;
    float D = 6.56649f;
    float E = 0.311465f;
    
    x = max(0.0f, x);
    float f1 = A * x;
    float f2 = pow(x, B) * (1.0f - exp2(-C * x));
    float f3 = saturate(x * D + E);
    
    return lerp(f1, f2, f3);
}

float4 ConsoleGammaPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 color = tex2D(InputTex, uv).rgb;
    float3 outColor = float3(X360GammaApprox(color.x), X360GammaApprox(color.y), X360GammaApprox(color.z));
    
    return float4(outColor, 1.0);
}

technique ConsoleGamma
{
    pass ConsoleGamma
    {
        PixelShader = compile ps_3_0 ConsoleGammaPS();
        VertexShader = compile vs_3_0 FullscreenVS();
        ZEnable = 0;
        ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
        ScissorTestEnable = False;
    }
}
