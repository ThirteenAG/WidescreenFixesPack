sampler g_TextureSampler;

struct PS_IN
{
    float2 texcoord : TEXCOORD;
    float2 texcoord1 : TEXCOORD1;
    float2 texcoord2 : TEXCOORD2;
    float2 texcoord3 : TEXCOORD3;
};

float4 main(PS_IN i) : COLOR
{
    float4 o;

    float4 r0;
    float4 r1;
    r0 = tex2D(g_TextureSampler, i.texcoord1);
    r1 = r0 * 1.0;
    r0 = tex2D(g_TextureSampler, i.texcoord);
    r1 = r0 * 1.0 + r1;
    r0 = tex2D(g_TextureSampler, i.texcoord2);
    r1 = r0 * 1.0 + r1;
    r0 = tex2D(g_TextureSampler, i.texcoord3);
    o = r0 * 1.0 + r1;

    return o;
}