module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include <fstream>


export module ButtonIcons;

import ComVars;
import NativeMenu;

export namespace DPLIcons
{
    struct Glyph
    {
        uint16_t code;
        uint8_t top, left, height, width, y, x, variant, bearing, page, reserved;
    };
    static_assert(sizeof(Glyph) == 12);
    struct Font
    {
        Glyph* original = nullptr;
        uint32_t count = 0;
        std::vector<Glyph> glyphs;
    };
    struct Texture
    {
        std::array<uint8_t, 24576> original{};
        int family = -1;
    };
    SafetyHookInline nameHook, loadHook, releaseHook, textureHook;
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
        const auto pages = At<uint32_t>(pointer, 56), height = At<uint32_t>(pointer, 36);
        if (!count || count > 4096 || !pages || pages > 16 || height < 32 || height > 255 || At<uint32_t>(pointer, 60) != 1 ||
            !original) return false;
        for (size_t index = 0; index < count; ++index)
        {
            const auto& glyph = original[index];
            if (glyph.code >= 0x2460 && glyph.code <= 0x2470) return false;
            if (glyph.page == pages - 1 && static_cast<unsigned int>(glyph.y) + glyph.height > 160) return false;
        }
        Font font;
        font.original = original;
        font.count = count;
        font.glyphs.assign(original, original + count);
        for (uint16_t icon = 0; icon < 17; ++icon)
        {
            const auto size = static_cast<uint8_t>(icon >= 4 && icon <= 7 ? height : height * 3 / 4);
            font.glyphs.push_back({ static_cast<uint16_t>(0x2460 + icon), static_cast<uint8_t>((height - size) / 2),
                0, size, size, static_cast<uint8_t>(160 + icon / 8 * 32), static_cast<uint8_t>(icon % 8 * 32),
                0, 0, static_cast<uint8_t>(pages - 1), 0 });
        }
        std::sort(font.glyphs.begin(), font.glyphs.end(), [](const Glyph& left, const Glyph& right) { return left.code < right.code; });
        auto& owned = fonts.emplace(pointer, std::move(font)).first->second;
        At<Glyph*>(pointer, 24) = owned.glyphs.data();
        At<uint32_t>(pointer, 32) = static_cast<uint32_t>(owned.glyphs.size());
        return true;
    }
    void CorrectGlyphUv(void* font, uint16_t index, float* quad)
    {
        auto found = fonts.find(font);
        if (found == fonts.end() || index >= found->second.glyphs.size()) return;
        const auto code = found->second.glyphs[index].code;
        if (code >= 0x2460 && code <= 0x2470)
        {
            quad[10] = quad[8] + 32.0f / 256.0f;
            quad[11] = quad[9] + 32.0f / 256.0f;
        }
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
        RECT rectangle{ 0, 160, 256, 256 };
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, &rectangle, read ? D3DLOCK_READONLY : 0))) return false;
        if (lock.Pitch < 1024)
        {
            texture->UnlockRect(0);
            return false;
        }
        for (size_t row = 0; row < 24; ++row)
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
    void* Resource(uint32_t low, uint32_t high, uint32_t type)
    {
        auto* bucket = reinterpret_cast<uint8_t*>(TextureManager.get_ptr() + 1572880 + 4 * (low % 65537));
        auto* node = At<uint8_t*>(bucket, 0);
        for (size_t count = 0; node != bucket && node != reinterpret_cast<uint8_t*>(TextureManager.get_ptr() + 1835032) && count < 1024; ++count)
        {
            if (!node) break;
            if (At<uint32_t>(node, 8) == low && At<uint32_t>(node, 12) == high)
                return At<uint32_t>(node, 16) == type ? At<void*>(node, 20) : nullptr;
            node = At<uint8_t*>(node, 0);
        }
        return nullptr;
    }
    void RefreshTextureOwners()
    {
        fontTextures.clear();
        auto* hashPointer = DiffuseHash.get_ptr();
        if (!*hashPointer) return;
        const uint32_t diffuse = *hashPointer;
        for (const auto& [font, saved] : fonts)
        {
            const auto bank = At<uint32_t>(font, 64), pages = At<uint32_t>(font, 56);
            if (bank > 65535 || !pages || pages > 16) continue;
            auto* material = static_cast<uint8_t*>(Resource((bank << 16) | (pages - 1), 0, 3));
            if (!material || At<uint32_t>(material, 24) != 1) continue;
            // Each native shader profile stores its diffuse sampler in the material's parameter set.
            for (size_t profile = 0; profile < 6; ++profile)
            {
                auto* parameters = material + 40 + profile * 936;
                auto* bucket = parameters + 784 + 4 * (diffuse % 17);
                auto* node = At<uint8_t*>(bucket, 0);
                for (size_t count = 0; node != bucket && node != parameters + 856 && count < 16; ++count)
                {
                    if (!node) break;
                    if (At<uint32_t>(node, 8) == diffuse && At<uint32_t>(node, 40) == 8)
                    {
                        auto* resource = Resource(At<uint32_t>(node, 24), At<uint32_t>(node, 28), 4);
                        if (!resource) break;
                        auto* native = At<void*>(resource, 24);
                        if (!native) break;
                        auto* texture = At<IDirect3DBaseTexture9*>(native, 4);
                        if (texture && std::find(fontTextures.begin(), fontTextures.end(), texture) == fontTextures.end())
                            fontTextures.push_back(texture);
                        break;
                    }
                    node = At<uint8_t*>(node, 0);
                }
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
                OutputDebugStringA("DriverParallelLines: native controller font texture could not be updated.\n");
            }
        }
        return textureHook.stdcall<HRESULT>(device, stage, texture);
    }
    const wchar_t* __fastcall Name(void* self, void*, int bank, int id)
    {
        if (ready && textureHook && !textureFailed && !fonts.empty() && !fontTextures.empty() && bank == 3 && id >= 500 && id <= 515)
        {
            static const auto glyphs = []()
            {
                std::array<std::array<wchar_t, 2>, 16> result{};
                constexpr uint16_t icons[] = { 12, 13, 14, 15, 11, 10, 8, 9, 4, 5, 0, 1, 2, 3, 6, 7 };
                for (size_t i = 0; i < result.size(); ++i) result[i][0] = static_cast<wchar_t>(0x2460 + icons[i]);
                return result;
            }();
            return glyphs[id - 500].data();
        }
        return nameHook.unsafe_thiscall<const wchar_t*>(self, bank, id);
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
            for (size_t bank = 0; bank < 4; ++bank) PatchFont(At<void*>(service, 12 + bank * 328));
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
        auto names = hook::pattern("A1 ? ? ? ? 8B 10 56 8B 74 24 08 FF 74 24 0C 46 69 F6 08 01 00 00 FF 34 0E 8B C8 FF 52 10");
        auto load = hook::pattern("56 FF 74 24 10 8B F1 8B 4C 24 10 E8 ? ? ? ? 81 38 3F 8F 21 40 0F 85");
        auto release = hook::pattern("33 C0 88 41 08 89 41 14 89 41 18 C3 56 8B F1 33 C0 38 46 08");
        auto manager = hook::pattern("F6 05 ? ? ? ? 01 75 1C 83 0D ? ? ? ? 01 B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 59 B8 ? ? ? ? C3 8B 44 24 04 8B 10 8B 4C 24 08 3B 11 75 0D");
        auto diffuse = hook::pattern("85 DB 75 11 8D 45 E0 50 68 ? ? ? ? 8B 4D FC E8 ? ? ? ? A0");
        auto glyph = hook::pattern("FF 83 C8 00 00 00 81 BB C8 00 00 00 80 00 00 00 75 0A FF 75 08 8B CB E8");
        std::ifstream file(GetThisModulePath() / L"DriverParallelLines.FusionFix" / L"buttons.dds", std::ios::binary);
        artwork.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        const bool valid = artwork.size() == 131200 && !std::memcmp(artwork.data(), "DDS ", 4) &&
            !std::memcmp(artwork.data() + 84, "DXT3", 4) && *reinterpret_cast<const uint32_t*>(artwork.data() + 12) == 512 &&
            *reinterpret_cast<const uint32_t*>(artwork.data() + 16) == 256;
        if (!valid)
        {
            OutputDebugStringA("DriverParallelLines: native controller artwork contracts unavailable.\n");
            return;
        }
        glyphHook = safetyhook::create_mid(glyph.get_first(), [](SafetyHookContext& regs)
        {
            const auto position = *reinterpret_cast<const uintptr_t*>(regs.ebp - 0x0C);
            const auto index = *reinterpret_cast<const uint16_t*>(position - 4);
            CorrectGlyphUv(reinterpret_cast<void*>(regs.ebx), index, reinterpret_cast<float*>(regs.ecx));
        });
        nameHook = safetyhook::create_inline(names.get_first(), Name);
        loadHook = safetyhook::create_inline(load.get_first(), Load);
        releaseHook = safetyhook::create_inline(release.get_first(), Release);
        TextureManager.SetAddress(*manager.get_first<uintptr_t>(17));
        DiffuseHash.SetAddress(*diffuse.get_first<uintptr_t>(9));
        ready = glyphHook && nameHook && loadHook && releaseHook;
        if (!ready)
        {
            releaseHook.reset(); loadHook.reset(); nameHook.reset(); glyphHook.reset();
            OutputDebugStringA("DriverParallelLines: native controller artwork hook transaction failed.\n");
        }
    }
}
