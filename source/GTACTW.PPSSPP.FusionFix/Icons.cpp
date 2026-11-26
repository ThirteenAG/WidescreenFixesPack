#include "Game.hpp"
#include <cstdio>

namespace ctw
{
    namespace
    {
        // Button prompts are glyphs of the icon font, one 256x256 4-bit texture. Its
        // header, as the texture binder reads it: format, wrap, mip levels, wrap, log2
        // width, log2 height, palette link, ...; texels follow at +16, then 16
        // RGBA8888 colours. A selected set binds an 8-bit copy instead: the game's own
        // texels and colours (entries 0-15) with the set's button cells and colours
        // (entries 16-255) from icons/<set>.bin, written by tools/gamepad-icons/ctw.py.
        // The PSP set binds the game's texture unchanged.
        SafetyMipsInline binder;
        SafetyMipsMid borrowTexture, publishTexture, freeTexture;
        injector::hook_back<void(uintptr_t, const uint8_t*, uintptr_t)> bind;
        constexpr const char* sets[] = {"PSP", "360", "XBO", "PS3", "PS4", "PS5", "SWI", "DEK", "Auto"};
        constexpr unsigned setCount = sizeof(sets) / sizeof(*sets), autoSet = setCount - 1;
        constexpr auto folder = "ms0:/PSP/PLUGINS/GTACTW.PPSSPP.FusionFix/icons/";
        constexpr unsigned side = 256;
        // The original font texture: its colours and the 48 glyph rows (6 swizzled
        // block rows of 128-byte lines).
        constexpr uint32_t paletteHash = 0x772EB874u, glyphHash = 0x7F3440D9u;
        constexpr unsigned glyphBytes = 6 * 8 * 128;
        alignas(64) uint8_t texture[16 + side * side + 256 * 4];
        alignas(4) uint8_t setData[8 * 1024];
        unsigned setSize, selected, loaded = ~0u, built = ~0u;
        const uint8_t* verified;
        const uint8_t* builtFrom;

        struct SetHeader
        {
            char magic[4];
            uint16_t version, colours, cells, reserved;
        };
        struct Cell
        {
            uint8_t x, y, width, height;
        };
        static_assert(sizeof(SetHeader) == 12 && sizeof(Cell) == 4);

        uint32_t Hash(const uint8_t* data, unsigned bytes)
        {
            uint32_t hash = 2166136261u;
            for (unsigned i = 0; i < bytes; ++i)
                hash = (hash ^ data[i]) * 16777619u;
            return hash;
        }
        // GE swizzle: 16-byte by 8-row blocks, block rows stride / 16 blocks wide.
        unsigned Swizzle(unsigned x, unsigned y, unsigned stride)
        {
            return ((y >> 3) * (stride >> 4) + (x >> 4)) * 128 + (y & 7) * 16 + (x & 15);
        }
        unsigned Wanted()
        {
            return selected == autoSet ? 1u : selected;
        }

        // Walks the set file; false when it is malformed or names colours it lacks.
        bool ValidSet()
        {
            if (setSize < sizeof(SetHeader))
                return false;
            const auto& header = *reinterpret_cast<const SetHeader*>(setData);
            if (std::memcmp(header.magic, "CTWI", 4) || header.version != 1 || !header.colours || header.colours > 240)
                return false;
            uint32_t at = sizeof(SetHeader) + header.colours * 4u;
            for (unsigned i = 0; i < header.cells; ++i)
            {
                if (at + sizeof(Cell) > setSize)
                    return false;
                const auto& cell = *reinterpret_cast<const Cell*>(setData + at);
                const uint32_t pixels = uint32_t(cell.width) * cell.height;
                at += sizeof(Cell);
                if (cell.x + cell.width > side || cell.y + cell.height > 48 || pixels > setSize - at)
                    return false;
                for (uint32_t p = 0; p < pixels; ++p)
                {
                    const uint8_t index = setData[at + p];
                    if (index < 16 || index >= 16 + header.colours)
                        return false;
                }
                at += pixels;
            }
            return at == setSize;
        }
        bool Load(unsigned set)
        {
            if (loaded == set)
                return true;
            loaded = ~0u;
            setSize = 0;
            built = ~0u;
            if (!set)
                return false;
            char path[128];
            std::snprintf(path, sizeof(path), "%s%s.bin", folder, sets[set]);
            const SceUID file = sceIoOpen(path, PSP_O_RDONLY, 0777);
            if (file < 0)
            {
                logger.WriteF("Controller icons: %s is missing; the original icons stay.\n", path);
                return false;
            }
            const int count = sceIoRead(file, setData, sizeof(setData));
            sceIoClose(file);
            setSize = count > 0 ? uint32_t(count) : 0;
            if (!ValidSet())
            {
                logger.WriteF("Controller icons: %s is not a valid icon set; the original icons stay.\n", path);
                setSize = 0;
                return false;
            }
            loaded = set;
            return true;
        }
        bool IconFont(const uint8_t* header)
        {
            if (!Guest(uintptr_t(header), 16 + side * side / 2 + 64) ||
                header[0] != 4 || header[4] != 8 || header[5] != 8 || header[6])
                return false;
            if (Hash(header + 16 + side * side / 2, 64) != paletteHash)
                return false;
            if (header == verified)
                return true;
            if (Hash(header + 16, glyphBytes) != glyphHash)
                return false;
            verified = header;
            return true;
        }

        void ForgetTexture(uintptr_t first, uint32_t bytes = 1)
        {
            if (uintptr_t(verified) - first < bytes)
                verified = nullptr;
            if (uintptr_t(builtFrom) - first < bytes)
                builtFrom = nullptr;
        }
        void Build(const uint8_t* header)
        {
            std::memcpy(texture, header, 16);
            texture[0] = 5; // 8-bit indices, 256-colour CLUT
            texture[2] = 0;
            const uint32_t bytes = side * side + 256 * 4;
            std::memcpy(texture + 12, &bytes, 4);
            const uint8_t* source = header + 16;
            uint8_t* texels = texture + 16;
            for (unsigned y = 0; y != side; ++y)
            {
                for (unsigned x = 0; x != side / 2; ++x)
                {
                    const uint8_t pair = source[Swizzle(x, y, side / 2)];
                    texels[Swizzle(x * 2, y, side)] = pair & 15;
                    texels[Swizzle(x * 2 + 1, y, side)] = pair >> 4;
                }
            }
            uint8_t* clut = texels + side * side;
            std::memset(clut, 0, 256 * 4);
            std::memcpy(clut, source + side * side / 2, 64);
            const auto& set = *reinterpret_cast<const SetHeader*>(setData);
            std::memcpy(clut + 64, setData + sizeof(SetHeader), set.colours * 4u);
            uint32_t at = sizeof(SetHeader) + set.colours * 4u;
            for (unsigned i = 0; i < set.cells; ++i)
            {
                const auto& cell = *reinterpret_cast<const Cell*>(setData + at);
                at += sizeof(Cell);
                for (unsigned y = 0; y != cell.height; ++y)
                    for (unsigned x = 0; x != cell.width; ++x)
                        texels[Swizzle(cell.x + x, cell.y + y, side)] = setData[at++];
            }
            sceKernelDcacheWritebackRange(texture, sizeof(texture));
            builtFrom = header;
            built = loaded;
        }
        void Bind(uintptr_t object, const uint8_t* header, uintptr_t palette)
        {
            const unsigned wanted = Wanted();
            if (wanted && loaded == wanted && setSize && IconFont(header))
            {
                if (built != wanted || builtFrom != header)
                    Build(header);
                // The palette follows the texels, as in the game's own textures.
                bind.fun(object, texture, 0);
                return;
            }
            bind.fun(object, header, palette);
        }
    }

    // MAIN.GamepadIcons: Auto uses the Xbox 360 set, as the emulated game cannot
    // tell which controller PPSSPP reads.
    void SelectIcons(const char* name)
    {
        unsigned set = autoSet;
        for (unsigned i = 0; i < setCount; ++i)
            if (std::strcmp(name, sets[i]) == 0)
            {
                set = i;
                break;
            }
        if (set == selected)
            return;
        selected = set;
        const unsigned wanted = Wanted();
        if (wanted)
            Load(wanted);
    }
    void InstallIcons()
    {
        // Texture storage can be reused by a later archive or a borrowed
        // texture. Drop the cached atlas at native publication and release.
        borrowTexture = safetymips::create_mid(Address<0x08A88658>(), [](SafetyMipsContext& regs)
        {
            ForgetTexture(regs.a1);
        });
        publishTexture = safetymips::create_mid(Address<0x08A8870C>(), [](SafetyMipsContext& regs)
        {
            ForgetTexture(regs.a2);
        });
        freeTexture = safetymips::create_mid(Address<0x08938830>(), [](SafetyMipsContext& regs)
        {
            if (!verified && !builtFrom)
                return;
            const auto first = uintptr_t(regs.a1);
            if (Guest(first - 8, 8))
            {
                const uint32_t allocation = at<uint32_t>(first - 8);
                if (allocation > 8 && Guest(first, allocation - 8))
                    ForgetTexture(first, allocation - 8);
            }
        });
        binder = safetymips::create_inline(Address<0x08A88AE0>(), Bind);
        bind.fun = binder.original<void (*)(uintptr_t, const uint8_t*, uintptr_t)>();
    }
}
