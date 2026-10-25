// Temporal resolve, after GTA IV's Fusion Fix (source/resources/temporal/Temporal.hlsl): a 3x3 Gaussian reconstruction
// around the jittered sample, the motion of the closest depth in the 3x3, Catmull-Rom history and a YCoCg variance
// clip. For LDR and HDR scenes alike (the luma weighted tonemap is reversible).

sampler2D CurrentTex : register(s0);
sampler2D HistoryTex : register(s1);
sampler2D MotionTex  : register(s2);
sampler2D DepthTex   : register(s3);                // standard [0, 1] depth, 0 near

float4 gTexel : register(c0);                       // 1 / width, 1 / height, width, height
float4 gTaa   : register(c1);                       // xy: jitter in pixels (how far the content moved), z: history valid, w: variance clip gamma
float4 gBlend : register(c2);                       // x: minimum current weight, y: maximum current weight, z: weight per pixel of motion, w: luma weight
float4 gStatic : register(c3);                      // x: share of the unclipped history kept where nothing moves, y: motion in pixels below which a pixel is still

float Luma(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

// Luminance weighted reversible tonemap: keeps bright samples from dominating the filters
float3 Compress(float3 c)
{
    return c / (1.0 + Luma(c) * gBlend.w);
}

float3 Expand(float3 c)
{
    return c / max(1.0 - Luma(c) * gBlend.w, 1.0 / 65504.0);
}

float3 ToYCoCg(float3 c)
{
    return float3(dot(c, float3(0.25, 0.5, 0.25)), dot(c, float3(0.5, 0.0, -0.5)), dot(c, float3(-0.25, 0.5, -0.25)));
}

float3 FromYCoCg(float3 c)
{
    return float3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

float3 FetchCurrent(float2 uv)
{
    float3 c = tex2Dlod(CurrentTex, float4(uv, 0.0, 0.0)).rgb;
    return ToYCoCg(Compress(clamp(c, 0.0, 65000.0)));
}

float3 FetchHistory(float2 uv)
{
    // Catmull-Rom through five bilinear taps
    float2 position = uv * gTexel.zw;
    float2 center = floor(position - 0.5) + 0.5;
    float2 f = position - center;

    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);

    float2 w12 = w1 + w2;
    float2 tc12 = (center + w2 / w12) * gTexel.xy;
    float2 tc0 = (center - 1.0) * gTexel.xy;
    float2 tc3 = (center + 2.0) * gTexel.xy;

    float4 result = float4(tex2Dlod(HistoryTex, float4(tc12.x, tc0.y, 0.0, 0.0)).rgb, 1.0) * (w12.x * w0.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc0.x, tc12.y, 0.0, 0.0)).rgb, 1.0) * (w0.x * w12.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc12.x, tc12.y, 0.0, 0.0)).rgb, 1.0) * (w12.x * w12.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc3.x, tc12.y, 0.0, 0.0)).rgb, 1.0) * (w3.x * w12.y);
    result += float4(tex2Dlod(HistoryTex, float4(tc12.x, tc3.y, 0.0, 0.0)).rgb, 1.0) * (w12.x * w3.y);

    return ToYCoCg(Compress(clamp(result.rgb / result.a, 0.0, 65000.0)));
}

float3 ClipTowardsCenter(float3 boxMin, float3 boxMax, float3 history)
{
    float3 center = 0.5 * (boxMax + boxMin);
    float3 extents = 0.5 * (boxMax - boxMin) + 0.00001;
    float3 offset = history - center;
    float3 units = abs(offset / extents);
    float maxUnit = max(units.x, max(units.y, units.z));
    return maxUnit > 1.0 ? center + offset / maxUnit : history;
}

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    static const float2 offsets[9] =
    {
        float2(-1.0, -1.0), float2(0.0, -1.0), float2(1.0, -1.0),
        float2(-1.0,  0.0), float2(0.0,  0.0), float2(1.0,  0.0),
        float2(-1.0,  1.0), float2(0.0,  1.0), float2(1.0,  1.0)
    };

    float3 m1 = 0.0;
    float3 m2 = 0.0;
    float3 boxMin = 65504.0;
    float3 boxMax = -65504.0;
    float3 filtered = 0.0;
    float filterWeight = 0.0;

    float closestDepth = 1.0;
    float2 closestOffset = 0.0;

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        float2 tap = uv + offsets[i] * gTexel.xy;
        float3 c = FetchCurrent(tap);

        m1 += c;
        m2 += c * c;
        boxMin = min(boxMin, c);
        boxMax = max(boxMax, c);

        // Each pixel holds the scene sampled at its center minus the jitter
        float2 d = offsets[i] - gTaa.xy;
        float w = exp(-2.29 * dot(d, d));
        filtered += c * w;
        filterWeight += w;

        float depth = tex2Dlod(DepthTex, float4(tap, 0.0, 0.0)).r;
        if (depth < closestDepth)
        {
            closestDepth = depth;
            closestOffset = offsets[i];
        }
    }

    filtered /= filterWeight;

    float2 motion = tex2Dlod(MotionTex, float4(uv + closestOffset * gTexel.xy, 0.0, 0.0)).xy;
    float2 historyUV = uv + motion;

    float3 mean = m1 / 9.0;
    float3 sigma = sqrt(max(m2 / 9.0 - mean * mean, 0.0));
    float3 clipMin = max(boxMin, mean - gTaa.w * sigma);
    float3 clipMax = min(boxMax, mean + gTaa.w * sigma);

    float3 result = filtered;
    bool onScreen = all(historyUV == saturate(historyUV));

    if (gTaa.z > 0.0 && onScreen)
    {
        float3 rawHistory = FetchHistory(historyUV);
        float3 history = ClipTowardsCenter(clipMin, clipMax, rawHistory);

        // Detail thinner than a pixel is only hit by some of the jitter positions. In the frames it's missed by all nine
        // samples, the box doesn't contain it and the clip throws it out of the history, so it flickers even with a still
        // camera. Where nothing moves, part of the history is kept as is.
        float speed = length(motion * gTexel.zw);
        history = lerp(history, rawHistory, gStatic.x * saturate(1.0 - speed / gStatic.y));
        float currentWeight = lerp(gBlend.x, gBlend.y, saturate(speed * gBlend.z));
        result = lerp(history, filtered, currentWeight);
    }

    return float4(Expand(FromYCoCg(result)), 1.0);
}
