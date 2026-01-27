void FullscreenVS(in float4 iPosition : POSITION, in float2 iTexcoord : TEXCOORD0,
        out float4 oPosition : POSITION, out float2 oTexcoord : TEXCOORD0)
{
    oPosition = iPosition;
    oTexcoord = iTexcoord;
}

texture2D colorTex2D;
texture2D depthTex2D;

float nearPlane;
float farPlane;

float2 texelSize;

sampler2D colorTex
{
    Texture = <colorTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Linear;
    MinFilter = Linear;
    MagFilter = Linear;
};
sampler2D depthTex
{
    Texture = <depthTex2D>;
    AddressU = Clamp;
    AddressV = Clamp;
    MipFilter = Point;
    MinFilter = Point;
    MagFilter = Point;
};

float LinearizeDepth(float rawDepth)
{
    return nearPlane * farPlane / (farPlane - rawDepth * (farPlane - nearPlane));
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

float3 X360GammaApprox(float3 color)
{
    return float3(
        X360GammaApprox(color.x),
        X360GammaApprox(color.y),
        X360GammaApprox(color.z)
    );
}

float4 DoDistanceBlur(in float2 uv : TEXCOORD0) : COLOR0
{
   // float rawDepth = tex2D(depthTex, uv).r;
   // float4 color = float4(tex2D(colorTex, uv).rgb * float3(0.0, 0.0, 1.0), 1.0);
   // return float4(rawDepth, rawDepth, rawDepth, 1.0);
    float sceneDepth = 1.0 - tex2D(depthTex, uv).r;

    // Linearize depth (on SM3.0 we approximate)
    float linearDepth = LinearizeDepth(sceneDepth); // you can replace with proper linearize if projection info available
    // Compute blur factor based on distance
    static const float BlurStart = 25.0f; // distance where blur begins
    static const float BlurEnd = 30.0f;
    
    float blurFactor = saturate((linearDepth - BlurStart) / (BlurEnd - BlurStart));
    
    // Sample neighborhood
    float3 color = tex2D(colorTex, uv).rgb;

    // 8-tap blur
    float2 offsets[8] =
    {
        float2(-1, -1), float2(1, -1),
        float2(-1, 1), float2(1, 1),
        float2(-2, 0), float2(2, 0),
        float2(0, -2), float2(0, 2)
    };

    for (int i = 0; i < 8; i++)
    {
        float2 sampUV = uv + offsets[i] * texelSize * blurFactor;
        color += tex2D(colorTex, sampUV).rgb;
    }

    return float4(X360GammaApprox(color.rgb / 9.0f), 1.0);
}

technique DistanceBlur
{
    pass DistanceBlur
    {
        PixelShader = compile ps_3_0 DoDistanceBlur();
        VertexShader = compile vs_3_0 FullscreenVS();
        ZEnable = 0;
        ZWriteEnable = false;
        AlphaBlendEnable = false;
        AlphaTestEnable = false;
        StencilEnable = false;
    }
}