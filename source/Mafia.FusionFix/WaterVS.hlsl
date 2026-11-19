// Reflective water, Direct3D 9 path (vertex part)
float4 cWorldViewProj[4] : register(c0);
float4 cWorld[3] : register(c4);
float4 cReflection[4] : register(c8);
float4 cWaves[6] : register(c12);
float4 cFog : register(c18);

struct Input
{
    float4 position : POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct Output
{
    float4 position : POSITION;
    float4 color : COLOR0;
    float3 reflection : TEXCOORD0;
    float4 waves : TEXCOORD1;
    float4 waves2 : TEXCOORD2;
    float4 world : TEXCOORD3;
    float fog : FOG;
};

Output main(Input i)
{
    Output o;
    float4 p = float4(i.position.xyz, 1.0);
    o.position = float4(dot(p, cWorldViewProj[0]), dot(p, cWorldViewProj[1]), dot(p, cWorldViewProj[2]), dot(p, cWorldViewProj[3]));
    float3 world = float3(dot(p, cWorld[0]), dot(p, cWorld[1]), dot(p, cWorld[2]));
    o.reflection = float3(dot(p, cReflection[0]), dot(p, cReflection[1]), dot(p, cReflection[3]));
    float3 plane = float3(world.x, world.z, 1.0);
    o.waves = float4(dot(plane, cWaves[0].xyz), dot(plane, cWaves[1].xyz), dot(plane, cWaves[2].xyz), dot(plane, cWaves[3].xyz));
    o.waves2 = float4(dot(plane, cWaves[4].xyz), dot(plane, cWaves[5].xyz), i.uv);
    o.world = float4(world, o.position.w);
    o.color = i.color;
    o.fog = (cFog.x - o.position.w) * cFog.y;
    return o;
}
