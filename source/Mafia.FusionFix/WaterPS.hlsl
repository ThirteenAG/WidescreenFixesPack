// Reflective water, Direct3D 9 path (pixel part)
sampler2D sReflection : register(s0);
sampler2D sWaves : register(s1);
sampler2D sWater : register(s3);

float4 cCamera : register(c0);
float4 cSun : register(c1);      // direction to the sun, w = 1 when the scene has one
float4 cSunColor : register(c2);
float4 cTint : register(c3);     // rgb: the game's water texture is darkened by it, a: reflection strength
float4 cParams : register(c4);   // x: distortion, y: reflectance at normal incidence, z: wave strength, w: 1 / calm distance

struct Input
{
    float4 color : COLOR0;
    float3 reflection : TEXCOORD0;
    float4 waves : TEXCOORD1;
    float4 waves2 : TEXCOORD2;
    float4 world : TEXCOORD3;
};

// The wave normals are stored as (x, z, up)
float3 Wave(float2 uv)
{
    float3 n = tex2D(sWaves, uv).rgb * 2.0 - 1.0;
    return float3(n.x, n.z, n.y);
}

float4 main(Input i) : COLOR
{
    float distance = i.world.w;
    float3 n = Wave(i.waves.xy) + Wave(i.waves.zw) * 0.6 + Wave(i.waves2.xy) * 0.35;
    float detail = saturate(1.0 - distance * cParams.w);
    n.xz *= cParams.z * (0.3 + 0.7 * detail);
    n = normalize(n);

    float3 view = normalize(cCamera.xyz - i.world.xyz);
    float facing = saturate(dot(n, view));
    float fresnel = cParams.y + (1.0 - cParams.y) * pow(1.0 - facing, 5.0);

    float2 uv = i.reflection.xy / i.reflection.z + n.xz * cParams.x / (1.0 + distance * 0.04);
    float3 reflection = tex2D(sReflection, uv).rgb;

    float4 water = tex2D(sWater, i.waves2.zw) * i.color;
    float3 color = lerp(water.rgb * cTint.rgb, reflection, saturate(fresnel * cTint.a));

    float3 halfway = normalize(cSun.xyz + view);
    float highlight = saturate(dot(n, halfway));
    float sun = pow(highlight, 700.0) * 5.0 + pow(highlight, 80.0) * 0.25;
    color += sun * cSunColor.rgb * cSun.w * (0.2 + fresnel);
    return float4(color, water.a);
}
