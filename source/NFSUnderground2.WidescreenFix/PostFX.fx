// -------------------------------------------------------------
// Vertex shader (shared)
void FullscreenVS(
    in float4 iPosition : POSITION,
    in float2 iTexcoord : TEXCOORD0,
    out float4 oPosition : POSITION,
    out float2 oTexcoord : TEXCOORD0)
{
    oPosition = iPosition;
    oTexcoord = iTexcoord;
}

// -------------------------------------------------------------
// Samplers
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

float2 TexelSize;
float BlurStrength; // multiplier – 0.0 = off, 1.0 = base, >1.0 = stronger

// -------------------------------------------------------------
// Gamma approximation (unchanged)
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

// -------------------------------------------------------------
// Gamma pixel shader (renamed to avoid conflict)
float4 ConsoleGammaPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 color = tex2D(InputTex, uv).rgb;
    float3 outColor = float3(
        X360GammaApprox(color.x),
        X360GammaApprox(color.y),
        X360GammaApprox(color.z)
    );
    
    return float4(outColor, 1.0);
}

// -------------------------------------------------------------
// Blur helpers (stronger 9-tap Gaussian)
static const float offsets[9] = { -4, -3, -2, -1, 0, 1, 2, 3, 4 };
static const float weights[9] =
{
    0.016216, 0.054054, 0.121621, 0.194594, 0.227027,
    0.194594, 0.121621, 0.054054, 0.016216
};

float4 HorizontalBlurPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0.xxx;
    for (int i = 0; i < 9; i++)
    {
        float2 sampleUV = uv + float2(offsets[i] * TexelSize.x * BlurStrength, 0.0);
        sum += tex2D(InputTex, sampleUV).rgb * weights[i];
    }
    return float4(sum, 1.0);
}

float4 VerticalBlurPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0.xxx;
    for (int i = 0; i < 9; i++)
    {
        float2 sampleUV = uv + float2(0.0, offsets[i] * TexelSize.y * BlurStrength);
        sum += tex2D(InputTex, sampleUV).rgb * weights[i];
    }
    return float4(sum, 1.0);
}

// -------------------------------------------------------------
// Techniques
technique ConsoleGamma
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 ConsoleGammaPS();
        ZEnable = 0;
        ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
        ScissorTestEnable = False;
    }
}

technique BlurHorizontal
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 HorizontalBlurPS();
        ZEnable = 0;
        ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
        ScissorTestEnable = False;
    }
}

technique BlurVertical
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 VerticalBlurPS();
        ZEnable = 0;
        ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
        ScissorTestEnable = False;
    }
}