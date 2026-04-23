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
// NFS Most Wanted Xbox 360 gamma LUT
static const float kMWGammaLUT[256] =
{
    0.00000, 0.00391, 0.00684, 0.00978, 0.01173, 0.01466, 0.01662, 0.01955,
    0.02248, 0.02542, 0.02835, 0.03128, 0.03519, 0.03812, 0.04203, 0.04594,
    0.04888, 0.05376, 0.05767, 0.06158, 0.06647, 0.07038, 0.07527, 0.08016,
    0.08602, 0.09091, 0.09580, 0.10068, 0.10557, 0.11046, 0.11437, 0.11926,
    0.12414, 0.12903, 0.13392, 0.13783, 0.14272, 0.14761, 0.15249, 0.15640,
    0.16129, 0.16618, 0.17009, 0.17498, 0.17889, 0.18377, 0.18768, 0.19257,
    0.19648, 0.20235, 0.20626, 0.21114, 0.21505, 0.21896, 0.22385, 0.22776,
    0.23167, 0.23656, 0.24145, 0.24536, 0.25024, 0.25415, 0.25806, 0.26197,
    0.26686, 0.27175, 0.27566, 0.27957, 0.28446, 0.28837, 0.29228, 0.29717,
    0.30108, 0.30499, 0.30890, 0.31378, 0.31867, 0.32258, 0.32649, 0.33040,
    0.33431, 0.33920, 0.34311, 0.34702, 0.35093, 0.35484, 0.35973, 0.36364,
    0.36755, 0.37146, 0.37634, 0.38025, 0.38416, 0.38807, 0.39198, 0.39687,
    0.40078, 0.40469, 0.40762, 0.41251, 0.41642, 0.42033, 0.42424, 0.42913,
    0.43304, 0.43695, 0.43988, 0.44477, 0.44868, 0.45259, 0.45650, 0.46139,
    0.46432, 0.46823, 0.47312, 0.47703, 0.48094, 0.48387, 0.48876, 0.49267,
    0.49658, 0.49951, 0.50440, 0.50831, 0.51222, 0.51613, 0.52004, 0.52395,
    0.52688, 0.53177, 0.53568, 0.53959, 0.54350, 0.54741, 0.55132, 0.55523,
    0.55914, 0.56305, 0.56696, 0.57087, 0.57478, 0.57771, 0.58260, 0.58553,
    0.58944, 0.59433, 0.59726, 0.60117, 0.60606, 0.60899, 0.61290, 0.61681,
    0.62072, 0.62463, 0.62854, 0.63245, 0.63539, 0.64027, 0.64321, 0.64712,
    0.65200, 0.65494, 0.65885, 0.66276, 0.66667, 0.66960, 0.67449, 0.67742,
    0.68133, 0.68524, 0.68915, 0.69208, 0.69697, 0.69990, 0.70381, 0.70772,
    0.71163, 0.71456, 0.71945, 0.72239, 0.72630, 0.73021, 0.73412, 0.73705,
    0.74194, 0.74487, 0.74976, 0.75269, 0.75562, 0.76051, 0.76344, 0.76735,
    0.77126, 0.77517, 0.77810, 0.78299, 0.78592, 0.78983, 0.79374, 0.79668,
    0.80156, 0.80450, 0.80743, 0.81232, 0.81525, 0.82014, 0.82307, 0.82600,
    0.83089, 0.83382, 0.83675, 0.84164, 0.84457, 0.84946, 0.85239, 0.85533,
    0.86022, 0.86315, 0.86608, 0.87097, 0.87390, 0.87781, 0.88172, 0.88465,
    0.88856, 0.89247, 0.89638, 0.89932, 0.90323, 0.90714, 0.91007, 0.91496,
    0.91789, 0.92082, 0.92571, 0.92864, 0.93157, 0.93646, 0.93939, 0.94330,
    0.94624, 0.95015, 0.95406, 0.95699, 0.96188, 0.96481, 0.96774, 0.97165,
    0.97556, 0.97947, 0.98240, 0.98534, 0.99022, 0.99316, 0.99707, 1.00000
};

float MWGammaLUT(float x)
{
    x = saturate(x) * 255.0;
    int i0 = (int) x;
    int i1 = min(i0 + 1, 255);
    float f = x - (float) i0;
    return lerp(kMWGammaLUT[i0], kMWGammaLUT[i1], f);
}

// -------------------------------------------------------------
// NFS Most Wanted gamma pixel shader
float4 MWGammaPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 color = tex2D(InputTex, uv).rgb;
    return float4(MWGammaLUT(color.r),
                  MWGammaLUT(color.g),
                  MWGammaLUT(color.b), 1.0);
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