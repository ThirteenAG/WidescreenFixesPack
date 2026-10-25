// The screen quad of the movies (WMV decoded to YUV), scaled to keep the video's 16:9 on other screens:
// c20.x = horizontal scale, c20.y = vertical scale
float2 fScreenHalfPixelOffset : register(c1);
float2 fMovieScale : register(c20);

struct VS_INPUT
{
    float4 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : POSITION;
    float4 uv : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = float4(-fScreenHalfPixelOffset.x + input.position.x * fMovieScale.x, fScreenHalfPixelOffset.y + input.position.y * fMovieScale.y, 0.0, 1.0);
    output.uv = float4(input.position.z, input.position.w, 0.0, 0.0);
    return output;
}
