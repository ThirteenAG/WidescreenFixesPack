// A plain copy into a target of another format (the scene into RGBA16F for DLAA and FSR)

sampler2D Source : register(s0);

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    return float4(tex2Dlod(Source, float4(uv, 0.0, 0.0)).rgb, 1.0);
}
