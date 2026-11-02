module;

#include <stdafx.h>
#include <sstream>
#include <iomanip>

export module XeTexturePacker;

import ComVars;

// The Xbox 360 textures of the PC version (big-endian, tiled DXT1/DXT5/DXN/A8/A8R8G8B8) decoded straight to the
// A8R8G8B8 bitmap the game wants, with the same result as XenonTexture.dll: its endpoints and blends are computed in
// floats and rounded, DXN is a normal map with the blue channel rebuilt. Returns false for anything else, which the
// caller leaves to the game's own decoder.
namespace XeTextureDecoder
{
    uint32_t ReadBig(const uint8_t* p)
    {
        return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
    }

    uint32_t TiledOffset(uint32_t x, uint32_t y, uint32_t alignedWidth, uint32_t logBpb)
    {
        const uint32_t macro = ((x >> 5) + (y >> 5) * (alignedWidth >> 5)) << (logBpb + 7);
        const uint32_t micro = ((x & 7) + ((y & 6) << 2)) << logBpb;
        const uint32_t offset = macro + ((micro & ~15u) << 1) + (micro & 15) + ((y & 8) << (3 + logBpb)) + ((y & 1) << 4);
        return (((offset & ~511u) << 3) + ((offset & 448) << 2) + (offset & 63) + ((y & 16) << 7) +
            (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)) >> logBpb;
    }

    uint8_t Quantize(float v)
    {
        return uint8_t(int(v * 255.0f + 0.5f));
    }

    // One block's 16 pixels as B, G, R, A
    void ColorBlock(const uint8_t* b, bool dxt1, uint8_t out[16][4])
    {
        const uint16_t c0 = uint16_t(b[0] | (b[1] << 8)), c1 = uint16_t(b[2] | (b[3] << 8));
        const uint32_t indices = b[4] | (b[5] << 8) | (b[6] << 16) | (uint32_t(b[7]) << 24);
        const float e0[3] = { ((c0 >> 11) & 31) / 31.0f, ((c0 >> 5) & 63) / 63.0f, (c0 & 31) / 31.0f };
        const float e1[3] = { ((c1 >> 11) & 31) / 31.0f, ((c1 >> 5) & 63) / 63.0f, (c1 & 31) / 31.0f };
        uint8_t palette[4][4]{};
        const bool four = c0 > c1 || !dxt1;
        for (int c = 0; c < 3; ++c)
        {
            palette[0][2 - c] = Quantize(e0[c]);
            palette[1][2 - c] = Quantize(e1[c]);
            palette[2][2 - c] = Quantize(four ? (2 * e0[c] + e1[c]) / 3 : (e0[c] + e1[c]) / 2);
            palette[3][2 - c] = four ? Quantize((e0[c] + 2 * e1[c]) / 3) : 0;
        }
        palette[0][3] = palette[1][3] = palette[2][3] = 255;
        palette[3][3] = four ? 255 : 0;
        for (int i = 0; i < 16; ++i)
            std::memcpy(out[i], palette[(indices >> (2 * i)) & 3], 4);
    }

    void AlphaBlock(const uint8_t* b, uint8_t out[16])
    {
        const uint8_t a0 = b[0], a1 = b[1];
        uint64_t bits = 0;
        for (int i = 0; i < 6; ++i)
            bits |= uint64_t(b[2 + i]) << (8 * i);
        const float f0 = a0 / 255.0f, f1 = a1 / 255.0f;
        uint8_t palette[8] = { a0, a1 };
        if (a0 > a1)
            for (int i = 1; i < 7; ++i)
                palette[1 + i] = Quantize(((7 - i) * f0 + i * f1) / 7);
        else
        {
            for (int i = 1; i < 5; ++i)
                palette[1 + i] = Quantize(((5 - i) * f0 + i * f1) / 5);
            palette[6] = 0;
            palette[7] = 255;
        }
        for (int i = 0; i < 16; ++i)
            out[i] = palette[(bits >> (3 * i)) & 7];
    }

    // src: the texture file (32-byte big-endian header and level data); dst: width * height A8R8G8B8 pixels
    bool Decode(const uint8_t* src, size_t size, uint32_t width, uint32_t height, uint8_t* dst)
    {
        if (!src || size < 32 || ReadBig(src) != 0x44324B4B || ReadBig(src + 4) != width || ReadBig(src + 8) != height || !width || !height)
            return false;
        const uint32_t format = ReadBig(src + 12), tail = ReadBig(src + 20);
        const uint32_t gpu = format & 0x3F, endian = (format >> 6) & 3;
        const bool tiled = (format & 0x100) != 0;
        uint32_t blockSize, bpb;
        switch (gpu)
        {
            case 0x12: blockSize = 4; bpb = 8; break;  // DXT1
            case 0x14: blockSize = 4; bpb = 16; break; // DXT4/5
            case 0x31: blockSize = 4; bpb = 16; break; // DXN
            case 0x02: blockSize = 1; bpb = 1; break;  // A8
            case 0x06: blockSize = 1; bpb = 4; break;  // A8R8G8B8
            default: return false;
        }
        const uint32_t widthBlocks = (width + blockSize - 1) / blockSize, heightBlocks = (height + blockSize - 1) / blockSize;
        const uint32_t alignedWidth = (widthBlocks + 31) & ~31u;
        const uint32_t logBpb = bpb == 1 ? 0 : bpb == 4 ? 2 : bpb == 8 ? 3 : 4;

        // Level 0 inside the packed mip tail (textures of 16 texels): its place in the 32x32 tile
        uint32_t originX = 0, originY = 0;
        if (tail == 0)
        {
            if (blockSize != 4 || (width > 16 || height > 16) || (width != 16 && height != 16))
                return false;
            (width > height ? originY : originX) = 16 / blockSize;
        }
        if (!tiled && tail == 0)
            return false;

        const uint8_t* data = src + 32;
        const size_t dataSize = size - 32;
        const uint32_t sourceHeight = (originY + heightBlocks + 31) & ~31u;
        if (size_t(alignedWidth) * sourceHeight * bpb > dataSize && tiled)
            return false;
        if (!tiled && size_t(alignedWidth) * heightBlocks * bpb > dataSize)
            return false;

        // Byte order: 1 = 8 in 16, 2 = 8 in 32, 3 = 16 in 32
        uint8_t block[16];
        auto fetch = [&](uint32_t x, uint32_t y)
        {
            const size_t index = tiled ? TiledOffset(x + originX, y + originY, alignedWidth, logBpb) : size_t(y) * alignedWidth + x;
            const uint8_t* p = data + index * bpb;
            for (uint32_t i = 0; i < bpb; ++i)
            {
                uint32_t j = i;
                if (endian == 1) j = i ^ 1;
                else if (endian == 2) j = i ^ 3;
                else if (endian == 3) j = i ^ 2;
                block[i] = p[j];
            }
        };

        uint32_t* out = reinterpret_cast<uint32_t*>(dst);
        for (uint32_t by = 0; by < heightBlocks; ++by)
        {
            for (uint32_t bx = 0; bx < widthBlocks; ++bx)
            {
                fetch(bx, by);
                if (blockSize == 1)
                {
                    uint32_t pixel = 0;
                    if (gpu == 0x02)
                        pixel = uint32_t(block[0]) << 24;
                    else
                        std::memcpy(&pixel, block, 4);
                    out[size_t(by) * width + bx] = pixel;
                    continue;
                }
                uint8_t pixels[16][4];
                if (gpu == 0x12)
                    ColorBlock(block, true, pixels);
                else if (gpu == 0x14)
                {
                    uint8_t alpha[16];
                    ColorBlock(block + 8, false, pixels);
                    AlphaBlock(block, alpha);
                    for (int i = 0; i < 16; ++i)
                        pixels[i][3] = alpha[i];
                }
                else
                {
                    uint8_t red[16], green[16];
                    AlphaBlock(block, red);
                    AlphaBlock(block + 8, green);
                    for (int i = 0; i < 16; ++i)
                    {
                        const float x = red[i] / 255.0f * 2 - 1, y = green[i] / 255.0f * 2 - 1, zz = 1 - x * x - y * y;
                        // The game's decoder takes the root of a negative number here; the NaN becomes 0
                        pixels[i][0] = zz >= 0 ? Quantize(std::sqrt(zz) * 0.5f + 0.5f) : 0;
                        pixels[i][1] = Quantize(y * 0.5f + 0.5f);
                        pixels[i][2] = Quantize(x * 0.5f + 0.5f);
                        pixels[i][3] = 0;
                    }
                }
                for (int i = 0; i < 16; ++i)
                {
                    const uint32_t y = by * 4 + i / 4, x = bx * 4 + i % 4;
                    if (x < width && y < height)
                        std::memcpy(&out[size_t(y) * width + x], pixels[i], 4);
                }
            }
        }
        return true;
    }
}

namespace XeTexturePacker
{
    uint32_t currentTextureID;

    struct UncompressParams
    {
        uint32_t field0;
        uint32_t field1;
        uint32_t field2;
        uint32_t field3;
        uint32_t field4;
    };

    SafetyHookInline shUncompressTexture = {};
    int __cdecl UncompressTexture(void* srcBuffer, int srcSize, void* info)
    {
        int result = 0;

        auto width = *(uint16_t*)((uintptr_t)info + 4);
        auto height = *(uint16_t*)((uintptr_t)info + 6);
        auto dstBuffer = *(char**)((uintptr_t)info + 44);
        auto dataSize = 4 * width * height;
        currentTextureID = uint32_t(-1);

        // Decoded here directly, with the same result; the game's own decoder only for formats this doesn't cover
        if (srcSize > 0 && XeTextureDecoder::Decode(static_cast<const uint8_t*>(srcBuffer), size_t(srcSize), width, height, reinterpret_cast<uint8_t*>(dstBuffer)))
            return 1;

        UncompressParams uncompressParams = { width, height, 0, 1, 0 };
        if ((*pXeTexture_UncompressTexture)(srcBuffer, srcSize, &uncompressParams))
        {
            void* srcData = *(void**)(uncompressParams.field4 + 4);
            memcpy(dstBuffer, srcData, dataSize);
            result = 1;
        }
        return result;
    }

    SafetyHookInline shTEXhardwareload = {};
    void __cdecl TEXhardwareload(void* a1, uint32_t* texinfo, void* metadata, int index)
    {
        currentTextureID = texinfo[0];
        return shTEXhardwareload.unsafe_ccall(a1, texinfo, metadata, index);
    }
}

class TexturePacker
{
public:
    TexturePacker()
    {
        WFP::onInitEvent() += []()
        {
            // Fast texture decoding for Gamer's Edition
            if (!Screen.bSignatureEdition)
            {
                auto pattern = hook::pattern("FF 15 ? ? ? ? 83 C4 0C 85 C0 0F 95 C0");
                pXeTexture_UncompressTexture.SetAddress(*pattern.get_first<XeUncompressTextureFn*>(2));

                pattern = hook::pattern("53 55 56 6A 14");
                XeTexturePacker::shUncompressTexture = safetyhook::create_inline(pattern.get_first(), XeTexturePacker::UncompressTexture);

                pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC B4 00 00 00 53");
                XeTexturePacker::shTEXhardwareload = safetyhook::create_inline(pattern.get_first(), XeTexturePacker::TEXhardwareload);
            }
        };
    }
} TexturePacker;
