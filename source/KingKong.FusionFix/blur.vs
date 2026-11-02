float g_fBigBlurFactor;
float4 g_vViewportUV;

struct VS_IN
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD;
};

struct VS_OUT
{
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
    float2 texcoord3 : TEXCOORD3;
};

VS_OUT main(VS_IN i)
{
    VS_OUT o;

    o.position = i.position;
    o.color = 0;
    o.texcoord = i.texcoord.xy;
    o.texcoord1 = i.texcoord.xy;
    o.texcoord2 = i.texcoord.xy;
    o.texcoord3 = i.texcoord.xy;

    return o;
}