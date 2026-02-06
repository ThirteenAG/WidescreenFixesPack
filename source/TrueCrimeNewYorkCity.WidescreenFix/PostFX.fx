void FullscreenVS(in float4 iPosition : POSITION, in float2 iTexcoord : TEXCOORD0,
        out float4 oPosition : POSITION, out float2 oTexcoord : TEXCOORD0)
{
    oPosition = iPosition;
    oTexcoord = iTexcoord;
}

texture2D InputTex2D;
texture2D DepthTex2D;

sampler2D InputTex
{
    Texture = <InputTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
};
sampler2D DepthTex
{
    Texture = <DepthTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Point;
    MinFilter = Point;
    MagFilter = Point;
};

float2 vec2ViewportSize;

// dist blir
float fNearPlane;
float fFarPlane;
float fDistBlurStrength;

// bloom
float fBloomIntensity;
float fBloomThreshold;
float fBloomSoftThreshold;
float2 vec2BloomTexelSize;

float LinearizeDepth(float rawDepth)
{
    return fNearPlane * fFarPlane / (fFarPlane - rawDepth * (fFarPlane - fNearPlane));
}

float X360GammaApprox(float x)
{
    float A = 0.541901f;
    float B = 1.13465f;
    float C = 13.53054f;
    float D = 6.56649f;
    float E = 0.311465f;
    
    float f1 = A * x;
    float f2 = pow(x, B) * (1.0f - exp2(-C * x));
    float f3 = saturate(x * D + E);
    
    return lerp(f1, f2, f3);
}

float GetDepthCoef(float2 uv, sampler2D depthSampler)
{
    float worldDepth = 1.0 - tex2D(depthSampler, uv).r; // INTZ depth uses reverse z
    worldDepth = LinearizeDepth(worldDepth);
                                       
    static const float BlurStart = 25.0f;
    static const float BlurEnd = 30.0f;
    float coefDepth = saturate((worldDepth - BlurStart) / (BlurEnd - BlurStart));
    
    return coefDepth;
}

// bloom, thanks Tervel1337
float4 Box2x2Sample(sampler2D s, float2 uv)
{
    float2 texel = 1.0 / vec2ViewportSize;
    float4 o = texel.xyxy * float2(-1, 1).xxyy;

    float4 c00 = tex2D(s, uv + o.xy);
    float4 c10 = tex2D(s, uv + o.zy);
    float4 c01 = tex2D(s, uv + o.xw);
    float4 c11 = tex2D(s, uv + o.zw);

    return (c00 + c10 + c01 + c11) * 0.25;
}

float3 ExtractHighlights(float3 color, float threshold, float soft_threshold)
{
    color *= fBloomIntensity;
    
    //if (bLuminance)
    //{
    //    float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
    //    float highlight = smoothstep(threshold, threshold + 0.1, luminance);
    //    return color * highlight;
    //}
   
    // Use max channel as brightness because SDR doesn't scale well with luminance 
    float brightness = max(color.r, max(color.g, color.b));
    float knee = threshold * soft_threshold;
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0, 2 * knee);
    soft = soft * soft / (4 * knee + 0.00001);
    float contribution = max(soft, brightness - threshold);
    contribution /= max(brightness, 0.00001);
    return color * contribution;
}

float4 BloomPS(in float2 texcoord : TEXCOORD0) : COLOR0
{
    float4 boxColor = Box2x2Sample(InputTex, texcoord);
    float3 highlightColor = ExtractHighlights(boxColor.rgb, fBloomThreshold, fBloomSoftThreshold);
    return float4(highlightColor, 1.0);
}

float4 BloomBlurPS(in float2 texcoord : TEXCOORD0) : COLOR0
{
    static const float weights[9] =
    {
        0.00000001725527099, // -7.005856
        0.0000648805071, // -5.022637
        0.0165408142, // -3.083472
        0.318351150, // -1.263687
        0.330086291, //  0.000000
        0.318351150, // +1.263687
        0.0165408142, // +3.083472
        0.0000648805071, // +5.022637
        0.00000001725527099 // +7.005856
    };

    static const float Offsets[9] =
    {
        -7.00585556,
        -5.022637,
        -3.08347154,
        -1.26368654,
        0.000000f,
        1.26368654,
        3.08347154,
        5.022637,
        7.00585556
    };
    
    float2 uv = texcoord;

    float2 offsetDir = vec2BloomTexelSize;

    float3 color = 0;

    [unroll]
    for (int i = 0; i < 9; i++)
    {
        color += tex2D(InputTex, uv + offsetDir * Offsets[i]).rgb * weights[i];
    }

    return float4(color, 1.0);
}

float4 BloomOutputPS(in float2 texcoord : TEXCOORD0) : COLOR0
{
    return float4(tex2D(InputTex, texcoord).rgb, 1.0);
}

float4 DistanceBlurPS(in float2 uv : TEXCOORD0) : COLOR0 // thanks to Tervel1337
{
    static const float2 HexPattern[13] =
    {
        float2(0.0, 0.0), // center

        // Inner ring (radius = 1.0)
        float2(0.5, 0.8660),
        float2(-0.5, 0.8660),
        float2(-1.0, 0.0),
        float2(-0.5, -0.8660),
        float2(0.5, -0.8660),
        float2(1.0, 0.0),

        // Outer ring (radius = 2.0)
        float2(1.0, 1.7320),
        float2(-1.0, 1.7320),
        float2(-2.0, 0.0),
        float2(-1.0, -1.7320),
        float2(1.0, -1.7320),
        float2(2.0, 0.0)
    };
    
    float3 output = 0;
    float count = 0;
    
    float coefDepth = GetDepthCoef(uv, DepthTex);
    
    [unroll]
    for (int i = 0; i < 13; ++i)
    {
        float w = exp(-dot(HexPattern[i], HexPattern[i]) * 0.5);
        float2 offset = (HexPattern[i] / vec2ViewportSize) * coefDepth;
        output += tex2D(InputTex, uv + offset).rgb * w;
        count += w;
    }
    
    return float4(output / count, 1.0);
}

float4 ConsoleGammaPS(in float2 uv : TEXCOORD0) : COLOR0
{
    float3 color = tex2D(InputTex, uv).rgb;
    float3 outColor = float3(X360GammaApprox(color.x), X360GammaApprox(color.y), X360GammaApprox(color.z));
    
    return float4(outColor, 1.0);
}

technique DistanceBlur
{
    pass DistanceBlur
    {
        PixelShader = compile ps_3_0 DistanceBlurPS();
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

technique Bloom
{
    pass Bloom
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 BloomPS();
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = ZERO;
        AlphaTestEnable = FALSE;
        ZEnable = 0;
        ZWriteEnable = FALSE;
        CullMode = None;
        ScissorTestEnable = False;
    }
    pass BloomBlur
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 BloomBlurPS();
        AlphaBlendEnable = TRUE;
        SrcBlend = ONE;
        DestBlend = ZERO;
        AlphaTestEnable = FALSE;
        ZEnable = 0;
        ZWriteEnable = FALSE;
        CullMode = None;
        ScissorTestEnable = False;
    }
    pass BloomOutput
    {
        VertexShader = compile vs_3_0 FullscreenVS();
        PixelShader = compile ps_3_0 BloomOutputPS();
        AlphaBlendEnable = TRUE;
        SrcBlend = ONE;
        DestBlend = ONE;
        AlphaTestEnable = FALSE;
        ZEnable = 0;
        ZWriteEnable = FALSE;
        CullMode = None;
        ScissorTestEnable = False;
    }
}

// SMAA

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
    }
}