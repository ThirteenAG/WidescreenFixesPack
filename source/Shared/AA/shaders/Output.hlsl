// The anti-aliased scene back into the game's target, sharpened like AMD's contrast adaptive sharpening: fine detail
// more than flat areas and strong edges, so the softness of temporal anti-aliasing goes without halos.

sampler2D Source : register(s0);

float4 gOutput : register(c0);  // xy: 1 / size, z: sharpness 0 to 1

float3 Fetch(float2 uv)
{
    return saturate(tex2Dlod(Source, float4(uv, 0.0, 0.0)).rgb);
}

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    float3 c = Fetch(uv);
    if (gOutput.z <= 0.0)
        return float4(c, 1.0);

    float3 n = Fetch(uv + float2(0.0, -gOutput.y));
    float3 s = Fetch(uv + float2(0.0, gOutput.y));
    float3 w = Fetch(uv + float2(-gOutput.x, 0.0));
    float3 e = Fetch(uv + float2(gOutput.x, 0.0));

    float3 lo = min(c, min(min(n, s), min(w, e)));
    float3 hi = max(c, max(max(n, s), max(w, e)));
    float3 amount = sqrt(saturate(min(lo, 1.0 - hi) / max(hi, 1e-4)));
    float3 weight = -amount * 0.2 * gOutput.z;
    float3 result = (c + (n + s + w + e) * weight) / (1.0 + 4.0 * weight);
    return float4(saturate(result), 1.0);
}
