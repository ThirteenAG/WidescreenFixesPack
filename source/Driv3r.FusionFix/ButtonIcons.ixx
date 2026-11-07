module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include <fstream>


export module ButtonIcons;

import ComVars;
import NativeMenu;

export namespace Driv3rIcons
{
    struct Glyph
    {
        uint16_t code, metrics;
        uint32_t uv;
    };
    static_assert(sizeof(Glyph) == 8);
    struct Font
    {
        Glyph* original = nullptr;
        uint32_t count = 0;
        std::vector<Glyph> glyphs;
    };
    struct Texture
    {
        std::array<uint8_t, 32768> original{};
        int family = -1;
    };
    SafetyHookInline nameHook, axisHook, loadHook, releaseHook, textureHook;
    SafetyHookMid glyphHook;
    std::unordered_map<void*, Font> fonts;
    std::unordered_map<IDirect3DTexture9*, Texture> textures;
    std::vector<IDirect3DBaseTexture9*> fontTextures;
    std::vector<uint8_t> artwork;
    bool ready = false;
    bool textureFailed = false;

    template<class T> T& At(void* pointer, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(pointer) + offset);
    }
    int Family()
    {
        const int family = FixSettings().Int("MAIN.GamepadIcons");
        return family == 2 ? 1 : family == 3 ? 2 : 0;
    }
    bool PatchFont(void* pointer)
    {
        if (!pointer || !At<uint8_t>(pointer, 12)) return false;
        auto* original = At<Glyph*>(pointer, 24);
        auto found = fonts.find(pointer);
        if (found != fonts.end())
        {
            if (original == found->second.glyphs.data()) return true;
            fonts.erase(found);
        }
        const uint32_t count = At<uint32_t>(pointer, 32);
        if (!count || count > 4096 || At<uint32_t>(pointer, 56) != 2 || !original) return false;
        for (size_t index = 0; index < count; ++index)
        {
            const auto& glyph = original[index];
            if (glyph.code >= 0x2460 && glyph.code <= 0x2470) return false;
            if ((glyph.metrics >> 12) == 1 && ((glyph.uv >> 8) & 255) + ((glyph.metrics >> 6) & 63) > 128) return false;
        }
        Font font;
        font.original = original;
        font.count = count;
        font.glyphs.assign(original, original + count);
        for (uint16_t icon = 0; icon < 17; ++icon)
        {
            const uint32_t x = icon % 8 * 32, y = 128 + icon / 8 * 32;
            const bool shoulder = icon >= 4 && icon <= 7;
            font.glyphs.push_back({ static_cast<uint16_t>(0x2460 + icon), static_cast<uint16_t>(shoulder ? 0x1C30 : 0x1820),
                x | (y << 8) | (2 << 16) | (shoulder ? 8u << 26 : 0) });
        }
        std::sort(font.glyphs.begin(), font.glyphs.end(), [](const Glyph& left, const Glyph& right) { return left.code < right.code; });
        auto& owned = fonts.emplace(pointer, std::move(font)).first->second;
        At<Glyph*>(pointer, 24) = owned.glyphs.data();
        At<uint32_t>(pointer, 32) = static_cast<uint32_t>(owned.glyphs.size());
        return true;
    }
    void RestoreFont(void* pointer)
    {
        auto found = fonts.find(pointer);
        if (found == fonts.end()) return;
        if (At<Glyph*>(pointer, 24) == found->second.glyphs.data())
        {
            At<Glyph*>(pointer, 24) = found->second.original;
            At<uint32_t>(pointer, 32) = found->second.count;
        }
        fonts.erase(found);
    }
    int __fastcall Load(void* resource, void*, int bank, void* container, int index)
    {
        auto* font = static_cast<uint8_t*>(resource) - 4;
        RestoreFont(font);
        const int result = loadHook.thiscall<int>(resource, bank, container, index);
        if (ready) PatchFont(font);
        return result;
    }
    int __fastcall Release(void* resource, void*)
    {
        RestoreFont(static_cast<uint8_t*>(resource) - 4);
        return releaseHook.thiscall<int>(resource);
    }
    bool Copy(IDirect3DTexture9* texture, uint8_t* bytes, bool read)
    {
        RECT rectangle{ 0, 128, 256, 256 };
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, &rectangle, read ? D3DLOCK_READONLY : 0))) return false;
        if (lock.Pitch < 1024)
        {
            texture->UnlockRect(0);
            return false;
        }
        for (size_t row = 0; row < 32; ++row)
        {
            auto* pixels = static_cast<uint8_t*>(lock.pBits) + row * lock.Pitch;
            if (read) std::memcpy(bytes + row * 1024, pixels, 1024);
            else std::memcpy(pixels, bytes + row * 1024, 1024);
        }
        return SUCCEEDED(texture->UnlockRect(0));
    }
    bool Apply(IDirect3DTexture9* texture)
    {
        auto found = textures.find(texture);
        if (found == textures.end())
        {
            D3DSURFACE_DESC desc{};
            if (FAILED(texture->GetLevelDesc(0, &desc)) || desc.Width != 256 || desc.Height != 256 || desc.Format != D3DFMT_DXT3) return false;
            Texture saved;
            if (!Copy(texture, saved.original.data(), true)) return false;
            texture->AddRef();
            found = textures.emplace(texture, std::move(saved)).first;
        }
        auto& saved = found->second;
        const int family = Family();
        if (saved.family != family && Copy(texture, artwork.data() + 128 + family * 32768, false)) saved.family = family;
        return saved.family == family;
    }
    void RefreshTextureOwners()
    {
        fontTextures.clear();
        for (const auto& [font, saved] : fonts)
        {
            const auto bank = At<uint32_t>(font, 64);
            if (bank > 0x40) continue;
            void* container = ProtectedGameRef<void*>(reinterpret_cast<void**>(TextureManager.get_ptr() + 328 + bank * 12));
            if (!container || At<uint8_t>(container, 44) != 1) continue;
            auto* pages = At<void*>(container, 0);
            if (!pages) continue;
            auto* header = At<void*>(pages, 0);
            auto* descriptors = At<uint8_t*>(pages, 4);
            if (!header || At<uint32_t>(header, 8) < 2 || !descriptors) continue;
            auto* entries = At<void*>(descriptors + 24, 16);
            if (!At<uint32_t>(descriptors + 24, 4) || !entries) continue;
            auto* resource = At<void*>(entries, 4);
            if (!resource) continue;
            auto* native = At<void*>(resource, 24);
            if (!native) continue;
            auto* gpu = At<void*>(native, 4);
            if (gpu)
            {
                auto* texture = At<IDirect3DBaseTexture9*>(gpu, 48);
                if (texture) fontTextures.push_back(texture);
            }
        }
    }
    bool OwnsTexture(IDirect3DBaseTexture9* texture)
    {
        return std::find(fontTextures.begin(), fontTextures.end(), texture) != fontTextures.end();
    }
    HRESULT WINAPI SetTexture(IDirect3DDevice9* device, DWORD stage, IDirect3DBaseTexture9* texture)
    {
        if (ready && !stage && texture && OwnsTexture(texture) && texture->GetType() == D3DRTYPE_TEXTURE)
        {
            if (!Apply(static_cast<IDirect3DTexture9*>(texture)) && !textureFailed)
            {
                textureFailed = true;
                OutputDebugStringA("Driv3r: native controller font texture could not be updated.\n");
            }
        }
        return textureHook.stdcall<HRESULT>(device, stage, texture);
    }
    int __stdcall Name(uint32_t code, wchar_t* buffer, int size)
    {
        const int result = nameHook.stdcall<int>(code, buffer, size);
        if (!ready || !textureHook || textureFailed || fonts.empty() || !buffer || size < 2) return result;
        unsigned int icon = 17;
        if (code >= 0x1004 && code < 0x100E)
        {
            constexpr unsigned int buttons[] = { 0, 1, 2, 3, 4, 5, 10, 11, 8, 9 };
            icon = buttons[code - 0x1004];
        }
        else if (code >= 0x1000 && code < 0x1004) icon = 12 + code - 0x1000;
        if (icon < 17)
        {
            buffer[0] = static_cast<wchar_t>(0x2460 + icon);
            buffer[1] = 0;
        }
        return result;
    }
    int __stdcall AxisName(uint32_t axis, bool positive, wchar_t* buffer, int size)
    {
        const int result = axisHook.stdcall<int>(axis, positive, buffer, size);
        if (ready && textureHook && !textureFailed && !fonts.empty() && axis == Keymap::AxisZ && buffer && size >= 2)
        {
            buffer[0] = static_cast<wchar_t>(0x2460 + (positive ? 6 : 7));
            buffer[1] = 0;
        }
        return result;
    }
    void Reset()
    {
        for (auto& [texture, saved] : textures)
        {
            Copy(texture, saved.original.data(), false);
            texture->Release();
        }
        textures.clear();
        fontTextures.clear();
        textureFailed = false;
    }
    void Frame()
    {
        if (!ready) return;
        if (FontService.is_initialized() && FontService.get())
        {
            auto* service = static_cast<uint8_t*>(FontService.get());
            for (size_t bank = 0; bank < 4; ++bank)
            {
                // A bank's font pointer stays uninitialized until the bank has loaded
                auto* slot = service + 12 + bank * 5744;
                if (At<int>(slot, 5740) > 0) PatchFont(At<void*>(slot, 0));
            }
        }
        RefreshTextureOwners();
        for (auto& [texture, saved] : textures) Apply(texture);
    }
    void Attach(IDirect3DDevice9* device)
    {
        if (!ready || textureHook || !device) return;
        textureHook = safetyhook::create_inline((*reinterpret_cast<void***>(device))[65], SetTexture);
    }
    void Install()
    {
        FixSettings().Availability("MAIN.GamepadIcons", [](std::string_view) { return ready && textureHook && !textureFailed; });
        const auto names = GetSecondaryInputNameAddress();
        auto axis = hook::pattern("8B 44 24 04 83 F8 0B 53 56 8B 35 ? ? ? ? 57 0F 87 ? ? ? ? FF 24 85");
        auto load = hook::pattern("8B 44 24 0C 56 8B F1 8B 4C 24 0C 8B 84 81 08 11 00 00 81 38 3E 8F 21 40");
        auto release = hook::pattern("33 C0 88 41 08 89 41 14 89 41 18 C3 CC CC CC CC 8B C1 C7 40 04");
        auto manager = hook::pattern("6A 01 53 BA ? ? ? ? E8 ? ? ? ? 85 C0 74 36 83 78 04 00");
        auto glyph = hook::pattern("D9 5A 2C 83 45 00 01 81 7D 00 80 00 00 00 75");
        std::ifstream file(GetThisModulePath() / L"Driv3r.FusionFix" / L"buttons.dds", std::ios::binary);
        artwork.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        const bool valid = artwork.size() == 131200 && !std::memcmp(artwork.data(), "DDS ", 4) &&
            !std::memcmp(artwork.data() + 84, "DXT3", 4) && *reinterpret_cast<const uint32_t*>(artwork.data() + 12) == 512 &&
            *reinterpret_cast<const uint32_t*>(artwork.data() + 16) == 256;
        if (!valid)
        {
            OutputDebugStringA("Driv3r: native controller artwork contracts unavailable.\n");
            return;
        }
        // Short shoulder symbols use larger geometry; artwork cells retain their original UV size.
        glyphHook = safetyhook::create_mid(glyph.get_first(3), [](SafetyHookContext& regs)
        {
            auto* font = reinterpret_cast<void*>(regs.esi);
            if (fonts.find(font) == fonts.end()) return;
            auto* position = *reinterpret_cast<const uint8_t**>(regs.esp + 0x18);
            const auto index = *reinterpret_cast<const uint16_t*>(position - 4);
            if (index >= At<uint32_t>(font, 32)) return;
            const auto code = At<Glyph*>(font, 24)[index].code;
            if (code < 0x2460 || code > 0x2470) return;
            auto* quad = reinterpret_cast<float*>(regs.edx);
            quad[10] = quad[8] + 32.0f / 256.0f;
            quad[11] = quad[9] + 32.0f / 256.0f;
        });
        nameHook = safetyhook::create_inline(reinterpret_cast<void*>(names), Name);
        axisHook = safetyhook::create_inline(axis.get_first(), AxisName);
        loadHook = safetyhook::create_inline(load.get_first(), Load);
        releaseHook = safetyhook::create_inline(release.get_first(), Release);
        TextureManager.SetAddress(*manager.get_first<uintptr_t>(4));
        ready = nameHook && axisHook && loadHook && releaseHook && glyphHook;
        if (!ready)
        {
            glyphHook.reset(); releaseHook.reset(); loadHook.reset(); axisHook.reset(); nameHook.reset();
            OutputDebugStringA("Driv3r: native controller artwork hook transaction failed.\n");
        }
    }
}
