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
float Darkness; // [0,1] — how much to darken the final blurred output

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
// 13-tap separable Gaussian (sigma = 3.0)
// Weights computed from exp(-x² / (2·σ²)), then normalised.
static const int KERNEL_HALF = 6;
static const float offsets[13] = { -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6 };
static const float weights[13] =
{
    0.0185f, 0.0342f, 0.0563f, 0.0831f, 0.1097f, 0.1296f, 0.1370f,
    0.1296f, 0.1097f, 0.0831f, 0.0563f, 0.0342f, 0.0185f
};

float4 HorizontalBlurPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0f;
    for (int i = 0; i < 13; i++)
    {
        float2 sampleUV = uv + float2(offsets[i] * TexelSize.x * BlurStrength, 0.0f);
        sum += tex2D(InputTex, sampleUV).rgb * weights[i];
    }
    return float4(sum, 1.0f);
}

float4 VerticalBlurPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0f;
    for (int i = 0; i < 13; i++)
    {
        float2 sampleUV = uv + float2(0.0f, offsets[i] * TexelSize.y * BlurStrength);
        sum += tex2D(InputTex, sampleUV).rgb * weights[i];
    }
    return float4(sum, 1.0f);
}

// Final vertical pass that also applies the darkness overlay,
// removing the need for a separate dark-quad draw call on the CPU side.
float4 VerticalBlurDarkenPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 sum = 0.0f;
    for (int i = 0; i < 13; i++)
    {
        float2 sampleUV = uv + float2(0.0f, offsets[i] * TexelSize.y * BlurStrength);
        sum += tex2D(InputTex, sampleUV).rgb * weights[i];
    }
    return float4(sum * (1.0f - Darkness), 1.0f);
}

// -------------------------------------------------------------
// Shared pass state
#define COMMON_PASS_STATE       \
    ZEnable          = false;   \
    ZWriteEnable     = false;   \
    AlphaBlendEnable = false;   \
    AlphaTestEnable  = false;   \
    StencilEnable    = false;   \
    CullMode         = None;    \
    ScissorTestEnable = false;

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

// Use this instead of BlurVertical as the final pass to fold in
// the darkness multiplier and skip the separate dark-quad draw.
technique BlurVerticalDarken
{
    pass P0
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 VerticalBlurDarkenPS();
        COMMON_PASS_STATE
    }
}

// -------------------------------------------------------------
// SMAA
// Enhanced Subpixel Morphological Antialiasing, by iryoku:
// https://github.com/iryoku/smaa

/**
 * Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
 * Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
 * Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
 * Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. As clarification, there
 * is no requirement that the copyright notice and permission be included in
 * binary distributions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

texture2D colorTex2D;
texture2D edgesTex2D;
texture2D blendTex2D;
texture2D areaTex2D;
texture2D searchTex2D;

sampler2D colorTex
{
    Texture = <colorTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Point;
    MinFilter = Linear;
    MagFilter = Linear;
    SRGBTexture = true;
};

sampler2D colorGammaTex
{
    Texture = <colorTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
    SRGBTexture = false;
};

sampler2D edgesTex
{
    Texture = <edgesTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
    SRGBTexture = false;
};

sampler2D blendTex
{
    Texture = <blendTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
    SRGBTexture = false;
};

sampler2D areaTex
{
    Texture = <areaTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
    SRGBTexture = false;
};

sampler2D searchTex
{
    Texture = <searchTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
    MipFilter = Point;
    MinFilter = Point;
    MagFilter = Point;
    SRGBTexture = false;
};

float4 vec4SMAARTMetrics;
#define SMAA_RT_METRICS vec4SMAARTMetrics

#define SMAA_HLSL_3
#define SMAA_THRESHOLD 0.05
#define SMAA_MAX_SEARCH_STEPS 16
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#define SMAA_CORNER_ROUNDING 25

#include "SMAA.hlsl"

void SMAA_EdgeDetectionVS(inout float4 position : POSITION,
                         inout float2 texcoord : TEXCOORD0,
                         out float4 offset[3] : TEXCOORD1)
{
    SMAAEdgeDetectionVS(texcoord, offset);
}

void SMAA_BlendingWeightCalculationVS(inout float4 position : POSITION,
                                     inout float2 texcoord : TEXCOORD0,
                                     out float2 pixcoord : TEXCOORD1,
                                     out float4 offset[3] : TEXCOORD2)
{
    SMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);
}

void SMAA_NeighborhoodBlendingVS(inout float4 position : POSITION,
                                inout float2 texcoord : TEXCOORD0,
                                out float4 offset : TEXCOORD1)
{
    SMAANeighborhoodBlendingVS(texcoord, offset);
}

float4 SMAA_EdgeDetectionPS(float4 position : SV_POSITION,
                               float2 texcoord : TEXCOORD0,
                               float4 offset[3] : TEXCOORD1) : COLOR
{
    return float4(SMAALumaEdgeDetectionPS(texcoord, offset, colorGammaTex), 0.0, 0.0);
}

float4 SMAA_BlendingWeightCalculationPS(float4 position : SV_POSITION,
                                       float2 texcoord : TEXCOORD0,
                                       float2 pixcoord : TEXCOORD1,
                                       float4 offset[3] : TEXCOORD2) : COLOR
{
    return SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edgesTex, areaTex, searchTex, 0.0);
}

float4 SMAA_NeighborhoodBlendingPS(float4 position : SV_POSITION,
                                  float2 texcoord : TEXCOORD0,
                                  float4 offset : TEXCOORD1) : COLOR
{
    return SMAANeighborhoodBlendingPS(texcoord, offset, colorTex, blendTex);
}

technique SMAAEdgeDetection
{
    pass SMAAEdgeDetection
    {
        VertexShader = compile vs_3_0 SMAA_EdgeDetectionVS();
        PixelShader = compile ps_3_0 SMAA_EdgeDetectionPS();
        ZEnable = false;
        SRGBWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
    }
}

technique SMAABlendWeightCalculation
{
    pass SMAABlendWeightCalculation
    {
        VertexShader = compile vs_3_0 SMAA_BlendingWeightCalculationVS();
        PixelShader = compile ps_3_0 SMAA_BlendingWeightCalculationPS();
        ZEnable = false;
        SRGBWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
    }
}

technique SMAAOutputPass
{
    pass SMAAOutputPass
    {
        VertexShader = compile vs_3_0 SMAA_NeighborhoodBlendingVS();
        PixelShader = compile ps_3_0 SMAA_NeighborhoodBlendingPS();
        ZEnable = false;
        SRGBWriteEnable = true;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
        CullMode = None;
    }
}