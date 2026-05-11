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
float BlurStrength; // offset scale — 1.0 = base spread, >1.0 = wider
float Darkness; // [0,1] darkening in final pass

// -------------------------------------------------------------
// Gamma approximation
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
    return float4(
        X360GammaApprox(color.x),
        X360GammaApprox(color.y),
        X360GammaApprox(color.z),
        1.0f);
}

// -------------------------------------------------------------
// Blur helpers
// Blur in linear color space to avoid perceived darkening.
float3 ToLinear(float3 c)
{
    return pow(saturate(c), 2.2f);
}

float3 ToGamma(float3 c)
{
    return pow(max(c, 0.0f), 1.0f / 2.2f);
}

// 13-tap separable Gaussian
static const float offsets[13] =
{
    -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f,
     1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f
};

static const float weights[13] =
{
    0.0185f, 0.0342f, 0.0563f, 0.0831f, 0.1097f, 0.1296f, 0.1370f,
    0.1296f, 0.1097f, 0.0831f, 0.0563f, 0.0342f, 0.0185f
};

float4 HorizontalBlurPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0f;
    float wsum = 0.0f;

    for (int i = 0; i < 13; i++)
    {
        float2 sampleUV = uv + float2(offsets[i] * TexelSize.x * BlurStrength, 0.0f);
        float w = weights[i];
        sum += ToLinear(tex2D(InputTex, sampleUV).rgb) * w;
        wsum += w;
    }

    sum /= max(wsum, 1e-6f);
    return float4(ToGamma(sum), 1.0f);
}

float4 VerticalBlurPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0f;
    float wsum = 0.0f;

    for (int i = 0; i < 13; i++)
    {
        float2 sampleUV = uv + float2(0.0f, offsets[i] * TexelSize.y * BlurStrength);
        float w = weights[i];
        sum += ToLinear(tex2D(InputTex, sampleUV).rgb) * w;
        wsum += w;
    }

    sum /= max(wsum, 1e-6f);
    return float4(ToGamma(sum), 1.0f);
}

// Final vertical pass with optional darkening
float4 VerticalBlurDarkenPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0f;
    float wsum = 0.0f;

    for (int i = 0; i < 13; i++)
    {
        float2 sampleUV = uv + float2(0.0f, offsets[i] * TexelSize.y * BlurStrength);
        float w = weights[i];
        sum += ToLinear(tex2D(InputTex, sampleUV).rgb) * w;
        wsum += w;
    }

    sum /= max(wsum, 1e-6f);
    float3 outColor = ToGamma(sum) * (1.0f - Darkness);
    return float4(outColor, 1.0f);
}

// -------------------------------------------------------------
// Shared pass state
#define COMMON_PASS_STATE         \
    ZEnable            = false;   \
    ZWriteEnable       = false;   \
    AlphaBlendEnable   = false;   \
    AlphaTestEnable    = false;   \
    StencilEnable      = false;   \
    CullMode           = None;    \
    ScissorTestEnable  = false;

technique ConsoleGamma
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 ConsoleGammaPS();
        COMMON_PASS_STATE
    }
}

technique BlurHorizontal
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 HorizontalBlurPS();
        COMMON_PASS_STATE
    }
}

technique BlurVertical
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 VerticalBlurPS();
        COMMON_PASS_STATE
    }
}

technique BlurVerticalDarken
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 VerticalBlurDarkenPS();
        COMMON_PASS_STATE
    }
}