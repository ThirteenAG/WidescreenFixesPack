// The scene's depth as R32F for DLAA and FSR: the depth texture itself (INTZ) can't be copied to a shared texture

sampler2D SceneDepth : register(s0);

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    return tex2Dlod(SceneDepth, float4(uv, 0.0, 0.0)).rrrr;
}
