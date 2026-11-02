sampler2D texture_0 : register(s0);
sampler2D texture_1 : register(s1);
sampler2D texture_2 : register(s2);

float4 waterblend(
    float4 diffuse : COLOR0,
    float4 specular_and_fog : COLOR1,
    float4 uv_0 : TEXCOORD0,
    float4 uv_1 : TEXCOORD1,
    float4 uv_2 : TEXCOORD2
) : COLOR
{
    half4 sample_0 = tex2D(texture_0, uv_0);
    half4 sample_1 = tex2D(texture_1, uv_1);
    half4 sample_2 = tex2D(texture_2, uv_2);

    // Blend sample_1 into sample_0 using sample_0's alpha
    half4 blended = lerp(sample_0, sample_1, sample_0.a);

    // Use blended color with sample_2's alpha
    return half4(blended.rgb, sample_2.a);
}