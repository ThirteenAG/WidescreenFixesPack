module;

#include <stdafx.h>
#include <sstream>
#include <iomanip>

export module XeTexturePacker;

import ComVars;

namespace XeTexturePacker
{
    const std::string textureCacheDir = "Textures";

    uint32_t currentTextureID;

    std::string GetHexFilename(uint32_t texID)
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << texID;
        return ss.str() + ".bin";
    }

    bool GetCachedData(uint32_t texID, char* dstBuffer, size_t dataSize)
    {
        std::string filename = textureCacheDir + "/" + GetHexFilename(texID);
        std::ifstream file(filename, std::ios::binary | std::ios::in);
        if (file.is_open())
        {
            file.read(dstBuffer, dataSize);
            if (static_cast<size_t>(file.gcount()) == dataSize)
            {
                return true;
            }
        }
        return false;
    }

    struct UncompressParams
    {
        uint32_t field0;
        uint32_t field1;
        uint32_t field2;
        uint32_t field3;
        uint32_t field4;
    };

    int(__cdecl** XeTexture_UncompressTexture)(void*, int, UncompressParams*) = nullptr;

    SafetyHookInline shUncompressTexture = {};
    int __cdecl UncompressTexture(void* srcBuffer, int srcSize, void* info)
    {
        int result = 0;

        auto width = *(uint16_t*)((uintptr_t)info + 4);
        auto height = *(uint16_t*)((uintptr_t)info + 6);
        auto dstBuffer = *(char**)((uintptr_t)info + 44);
        auto dataSize = 4 * width * height;

        if (GetCachedData(currentTextureID, dstBuffer, dataSize))
        {
            return 1;
        }

        UncompressParams uncompressParams = { width, height, 0, 1, 0 };
        if ((*XeTexture_UncompressTexture)(srcBuffer, srcSize, &uncompressParams))
        {
            void* srcData = *(void**)(uncompressParams.field4 + 4);
            memcpy(dstBuffer, srcData, dataSize);
            result = 1;

            std::string dir = textureCacheDir;
            std::filesystem::create_directories(dir);
            std::string filename = dir + "/" + GetHexFilename(currentTextureID);
            std::ofstream file(filename, std::ios::binary | std::ios::out);
            if (file.is_open())
            {
                file.write(static_cast<const char*>(srcData), dataSize);
            }
        }
        currentTextureID = uint32_t(-1);
        return result;
    }

    SafetyHookInline shTEXhardwareload = {};
    void __cdecl TEXhardwareload(void* a1, uint32_t* texinfo, void* metadata, int index)
    {
        currentTextureID = texinfo[0];
        return shTEXhardwareload.unsafe_ccall(a1, texinfo, metadata, index);
    }
}

export void InitXeTexturePacker()
{
    // Texture caching for Gamer's Edition
    auto pattern = hook::pattern("FF 15 ? ? ? ? 83 C4 0C 85 C0 0F 95 C0");
    if (!pattern.empty())
    {
        XeTexturePacker::XeTexture_UncompressTexture = *(decltype(XeTexturePacker::XeTexture_UncompressTexture)*)pattern.get_first(2);

        pattern = hook::pattern("53 55 56 6A 14");
        XeTexturePacker::shUncompressTexture = safetyhook::create_inline(pattern.get_first(), XeTexturePacker::UncompressTexture);

        pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC B4 00 00 00 53");
        XeTexturePacker::shTEXhardwareload = safetyhook::create_inline(pattern.get_first(), XeTexturePacker::TEXhardwareload);
    }
}