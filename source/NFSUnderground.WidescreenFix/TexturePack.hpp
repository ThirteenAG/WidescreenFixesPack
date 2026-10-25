#pragma once
#include <windows.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Texture packs (TPK version 4) built in memory: the DDS files in the module's "TPKDDS" resources are
// packed the same way the game's own texture packs are, so the game's chunk loader can load them like a
// TPK file.
namespace TexturePack
{
    // bStringHash, the hash the games use for texture names
    inline uint32_t Hash(const char* str)
    {
        uint32_t result = 0xFFFFFFFF;
        for (; *str; ++str)
            result = static_cast<uint8_t>(*str) + 33 * result;
        return result;
    }

    struct Texture
    {
        std::string name;
        uint32_t hash;
        const uint8_t* dds;
        DWORD ddsSize;
    };

    class Writer
    {
    public:
        std::vector<uint8_t> data;

        size_t BeginChunk(uint32_t id)
        {
            Put(id);
            Put(uint32_t(0));
            return data.size();
        }

        void EndChunk(size_t start)
        {
            *reinterpret_cast<uint32_t*>(&data[start - 4]) = uint32_t(data.size() - start);
        }

        template<typename T> void Put(const T& value)
        {
            auto bytes = reinterpret_cast<const uint8_t*>(&value);
            data.insert(data.end(), bytes, bytes + sizeof(T));
        }

        void PutBytes(const void* bytes, size_t size)
        {
            data.insert(data.end(), static_cast<const uint8_t*>(bytes), static_cast<const uint8_t*>(bytes) + size);
        }

        void PutFill(uint8_t value, size_t size)
        {
            data.insert(data.end(), size, value);
        }

        void PutString(const std::string& text, size_t size)
        {
            std::vector<char> field(size, '\0');
            memcpy(field.data(), text.data(), std::min(text.size(), size - 1));
            PutBytes(field.data(), size);
        }

        // Inserts an empty chunk so that the following chunk starts at a multiple of alignment
        void Align(size_t alignment)
        {
            size_t padding = (alignment - (data.size() + 8) % alignment) % alignment;
            Put(uint32_t(0));
            Put(uint32_t(padding));
            PutFill(0, padding);
        }
    };

#pragma pack(push, 1)
    struct TextureInfo
    {
        uint32_t unknown[3];
        char debugName[24];
        uint32_t nameHash;
        uint32_t classNameHash;
        uint32_t unknown2;
        int32_t imagePlacement;
        int32_t palettePlacement;
        int32_t imageSize;
        int32_t paletteSize;
        int32_t baseImageSize;
        int16_t width;
        int16_t height;
        uint8_t shiftWidth;
        uint8_t shiftHeight;
        uint8_t imageCompressionType;
        uint8_t paletteCompressionType;
        int16_t numPaletteEntries;
        uint8_t numMipMapLevels;
        uint8_t tilableUV;
        uint8_t biasLevel;
        uint8_t renderingOrder;
        uint8_t scrollType;
        uint8_t usedFlag;
        uint8_t applyAlphaSorting;
        uint8_t alphaUsageType;
        uint8_t alphaBlendType;
        uint8_t flags;
        uint8_t mipmapBiasType;
        uint8_t unknown3;
        int16_t scrollTimeStep;
        int16_t scrollSpeedS;
        int16_t scrollSpeedT;
        int16_t offsetS;
        int16_t offsetT;
        int16_t scaleS;
        int16_t scaleT;
        uint32_t runtime[5];
    };
    static_assert(sizeof(TextureInfo) == 0x7C);

    struct PixelFormat
    {
        uint32_t unknown[2];
        uint32_t unknown1;  // 1
        uint32_t unknown2;  // 5
        uint32_t unknown3;  // 6
        uint32_t fourCC;
        uint32_t unknown4[2];
    };
    static_assert(sizeof(PixelFormat) == 0x20);
#pragma pack(pop)

    constexpr uint32_t FourCC(const char (&code)[5])
    {
        return uint32_t(uint8_t(code[0])) | (uint32_t(uint8_t(code[1])) << 8) | (uint32_t(uint8_t(code[2])) << 16) | (uint32_t(uint8_t(code[3])) << 24);
    }

    inline uint32_t Log2(uint32_t value)
    {
        uint32_t result = 0;
        while ((1u << (result + 1)) <= value)
            ++result;
        return result;
    }

    inline std::vector<Texture> LoadResourceTextures()
    {
        std::vector<Texture> textures;
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadResourceTextures), &module);

        EnumResourceNamesA(module, "TPKDDS", [](HMODULE module, LPCSTR type, LPSTR name, LONG_PTR param) -> BOOL
        {
            auto& list = *reinterpret_cast<std::vector<Texture>*>(param);
            if (IS_INTRESOURCE(name))
                return TRUE;

            HRSRC resource = FindResourceA(module, name, type);
            HGLOBAL handle = resource ? LoadResource(module, resource) : nullptr;
            auto bytes = handle ? static_cast<const uint8_t*>(LockResource(handle)) : nullptr;
            DWORD size = resource ? SizeofResource(module, resource) : 0;
            if (!bytes || size <= 128 || memcmp(bytes, "DDS ", 4) != 0)
                return TRUE;

            // Resource names are stored in upper case, which is also what texture hashes are made from
            list.push_back({ name, Hash(name), bytes, size });
            return TRUE;
        }, reinterpret_cast<LONG_PTR>(&textures));

        // The game looks textures up with a binary search over the hash list
        std::sort(textures.begin(), textures.end(), [](const Texture& a, const Texture& b) { return a.hash < b.hash; });
        return textures;
    }

    inline std::vector<uint8_t> Build(const std::vector<Texture>& textures, const std::string& packName)
    {
        uint32_t packHash = Hash(packName.c_str());
        Writer w;

        auto capsule = w.BeginChunk(0xB3300000);
        w.Align(0x40);

        auto info = w.BeginChunk(0xB3310000);
        {
            auto header = w.BeginChunk(0x33310001);
            w.Put(uint32_t(4));         // version
            w.PutString(packName, 0x1C);
            w.PutString(packName, 0x40);
            w.Put(packHash);
            w.PutFill(0, 6 * sizeof(uint32_t));
            w.EndChunk(header);

            auto hashes = w.BeginChunk(0x33310002);
            for (const auto& texture : textures)
            {
                w.Put(texture.hash);
                w.Put(uint32_t(0));
            }
            w.EndChunk(hashes);

            auto infos = w.BeginChunk(0x33310004);
            int32_t placement = 0;
            for (const auto& texture : textures)
            {
                auto dds = texture.dds;
                uint32_t height = *reinterpret_cast<const uint32_t*>(dds + 12);
                uint32_t width = *reinterpret_cast<const uint32_t*>(dds + 16);
                uint32_t mips = std::max(*reinterpret_cast<const uint32_t*>(dds + 28), 1u);
                uint32_t fourCC = *reinterpret_cast<const uint32_t*>(dds + 84);
                int32_t imageSize = int32_t(texture.ddsSize - 128);
                uint32_t blockSize = (fourCC == FourCC("DXT1")) ? 8 : 16;

                TextureInfo entry = {};
                strncpy_s(entry.debugName, texture.name.c_str(), _TRUNCATE);
                entry.nameHash = texture.hash;
                entry.classNameHash = 0x1A93CF;
                entry.imagePlacement = placement;
                entry.imageSize = imageSize;
                entry.baseImageSize = int32_t(std::max(1u, width / 4) * std::max(1u, height / 4) * blockSize);
                entry.width = int16_t(width);
                entry.height = int16_t(height);
                entry.shiftWidth = uint8_t(Log2(width));
                entry.shiftHeight = uint8_t(Log2(height));
                entry.imageCompressionType = (fourCC == FourCC("DXT1")) ? 0x22 : (fourCC == FourCC("DXT3")) ? 0x24 : 0x26;
                entry.numMipMapLevels = uint8_t(mips);
                entry.renderingOrder = 5;
                entry.alphaUsageType = 2;
                entry.alphaBlendType = 1;
                entry.offsetT = 0x100;
                entry.scaleS = 0x100;

                w.Put(entry);
                placement += imageSize;
            }
            w.EndChunk(infos);

            auto formats = w.BeginChunk(0x33310005);
            for (const auto& texture : textures)
            {
                uint32_t fourCC = *reinterpret_cast<const uint32_t*>(texture.dds + 84);
                PixelFormat pixelFormat = {};
                pixelFormat.unknown1 = 1;
                pixelFormat.unknown2 = 5;
                pixelFormat.unknown3 = 6;
                pixelFormat.fourCC = fourCC;
                w.Put(pixelFormat);
            }
            w.EndChunk(formats);
        }
        w.EndChunk(info);

        w.Align(0x80);
        auto pixels = w.BeginChunk(0xB3320000);
        {
            auto link = w.BeginChunk(0x33320001);
            w.Put(uint32_t(0));
            w.Put(uint32_t(0));
            w.Put(uint32_t(1));         // number of packs
            w.Put(packHash);
            w.Put(uint32_t(0));
            w.Put(uint32_t(0));
            w.EndChunk(link);

            w.Align(0x80);
            auto images = w.BeginChunk(0x33320002);
            w.PutFill(0x11, 0x78);      // image data starts on a 0x80 boundary
            for (const auto& texture : textures)
                w.PutBytes(texture.dds + 128, texture.ddsSize - 128);
            w.EndChunk(images);
        }
        w.EndChunk(pixels);
        w.EndChunk(capsule);
        return std::move(w.data);
    }
}
