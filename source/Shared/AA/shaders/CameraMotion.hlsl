// Camera motion vectors, reprojected from the scene's depth: previous - current position in texture coordinates,
// jitter excluded, so the history of a pixel is at uv + motion.
//
// Only the camera's motion is known. What moves with the camera (the player's car in a chase view, a cockpit) would get
// the motion of the world behind it, so the previous frame's depth checks it: where a point of the world would have
// been last frame, the depth must be what that point had then. When it isn't, and the depth at the same pixel is the
// same as now, the pixel shows something that moved with the camera: it gets no motion.

sampler2D SceneDepth : register(s0);                // standard [0, 1] depth of a perspective projection (INTZ or R32F)
sampler2D PreviousDepth : register(s1);             // the same, of the previous frame (R32F)

float4 gJitterNdc : register(c0);                   // xy: how far the rendered content moved this frame, in NDC
row_major float4x4 gReproject : register(c1);       // current clip space -> previous clip space, both without jitter
float4 gDepth : register(c5);                       // x: near, y: far, z: previous depth valid, w: 1 / height
float4 gTexel : register(c6);                       // xy: 1 / size

float Distance(float depth)
{
    // Standard depth (0 near) back to the distance along the view direction
    return gDepth.x * gDepth.y / (gDepth.y - depth * (gDepth.y - gDepth.x));
}

float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    float depth = tex2Dlod(SceneDepth, float4(uv, 0.0, 0.0)).r;
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0) - gJitterNdc.xy;
    float4 previous = mul(float4(ndc, depth, 1.0), gReproject);
    float w = abs(previous.w) > 1e-6 ? previous.w : 1e-6;
    float2 motion = (previous.xy / w - ndc) * float2(0.5, -0.5);

    if (gDepth.z > 0.0)
    {
        float2 previousUV = uv + motion;
        float expected = Distance(previous.z / w);
        bool onScreen = all(previousUV == saturate(previousUV));

        // The world point's depth last frame against the depths around where it was (any of them: depth changes fast
        // between pixels at grazing angles)
        float lo = 1e30;
        float hi = 0.0;
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            [unroll]
            for (int x = -1; x <= 1; ++x)
            {
                float d = Distance(tex2Dlod(PreviousDepth, float4(previousUV + float2(x, y) * gTexel.xy, 0.0, 0.0)).r);
                lo = min(lo, d);
                hi = max(hi, d);
            }
        }
        bool worldFits = !onScreen || (expected >= lo * 0.985 && expected <= hi * 1.015);

        float distance = Distance(depth);
        float still = Distance(tex2Dlod(PreviousDepth, float4(uv, 0.0, 0.0)).r);
        if (!worldFits && abs(still - distance) < distance * 0.01)
            motion = 0.0;
    }

    return float4(motion, 0.0, 1.0);
}
