module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <unordered_set>
#include <vector>

export module ButtonIcons;

import EngineText;
import IconBindings;

export namespace RE0Icons
{
    inline std::atomic<int> family{ 0 };
    inline std::vector<uint8_t> artwork;
    inline std::array<uint8_t, 65536> original{};
    inline std::array<bool, 4096> blocks{};
    inline std::mutex mutex;
    inline std::unordered_set<void*> fonts;
    inline SafetyHookInline loadHook, releaseHook;
    inline void* savedFont = nullptr;
    inline IDirect3DTexture9* savedTexture = nullptr;
    inline IDirect3DDevice9* savedDevice = nullptr;
    inline int applied = 0;
    inline bool managed = false, assetsReady = false, ready = false;
    inline std::atomic<bool> textureFailed{ false }, hasTexture{ false };

    struct Glyph
    {
        uint32_t code;
        uint16_t x, y;
    };
    inline constexpr std::array<Glyph, 10> buttons{
        Glyph{ 'A', 0, 0 }, { 'B', 30, 0 }, { 'X', 60, 0 }, { 'Y', 90, 0 },
        { 'l', 120, 0 }, { 'r', 150, 0 }, { 'L', 180, 0 }, { 'R', 210, 0 },
        { 'b', 180, 90 }, { 's', 150, 90 }
    };

    inline bool PadFont(void* font)
    {
        if (!font || RE0Text::Read<uint32_t>(font, 152) != 68614 ||
            RE0Text::Read<uint32_t>(font, 156) != 2 || RE0Text::Read<uint32_t>(font, 160) != 134 ||
            (RE0Text::Read<uint32_t>(font, 192) & 1)) return false;
        const auto glyphs = RE0Text::Read<uint8_t*>(font, 172);
        if (!glyphs) return false;
        uint32_t found = 0;
        for (size_t i = 0; i < 134; ++i)
        {
            const auto record = glyphs + i * 16;
            const auto code = RE0Text::Read<uint32_t>(record, 0);
            for (size_t j = 0; j < buttons.size(); ++j)
            {
                const auto& button = buttons[j];
                if (code != button.code) continue;
                const auto position = RE0Text::Read<uint32_t>(record, 4);
                const auto size = RE0Text::Read<uint32_t>(record, 8);
                if ((found & (1u << j)) || (position & 255) || ((position >> 8) & 4095) != button.x ||
                    (position >> 20) != button.y || ((size >> 8) & 4095) != 30 || (size >> 20) != 30) return false;
                found |= 1u << j;
            }
        }
        return found == (1u << buttons.size()) - 1;
    }

    inline bool MakeBlocks(void* font)
    {
        if (!PadFont(font)) return false;
        std::array<bool, 4096> selected{}, protectedBlocks{};
        const auto glyphs = RE0Text::Read<uint8_t*>(font, 172);
        for (size_t i = 0; i < 134; ++i)
        {
            const auto record = glyphs + i * 16;
            const auto code = RE0Text::Read<uint32_t>(record, 0);
            const auto position = RE0Text::Read<uint32_t>(record, 4);
            const auto size = RE0Text::Read<uint32_t>(record, 8);
            if (position & 255) continue;
            const auto x = (position >> 8) & 4095, y = position >> 20;
            const auto width = (size >> 8) & 4095, height = size >> 20;
            if (!width || !height) continue;
            if (x + width > 256 || y + height > 256) return false;
            const bool button = std::any_of(buttons.begin(), buttons.end(),
                [code](const auto& entry) { return entry.code == code; });
            auto& target = button ? selected : protectedBlocks;
            for (size_t row = y / 4; row < (y + height + 3) / 4; ++row)
                for (size_t column = x / 4; column < (x + width + 3) / 4; ++column)
                    target[row * 64 + column] = true;
        }
        for (size_t i = 0; i < blocks.size(); ++i) blocks[i] = selected[i] && !protectedBlocks[i];
        return std::any_of(blocks.begin(), blocks.end(), [](bool selected) { return selected; });
    }

    inline IDirect3DTexture9* NativeTexture(void* font)
    {
        const auto resources = RE0Text::Read<uint8_t**>(font, 180);
        if (!resources || !resources[0]) return nullptr;
        const auto texture = RE0Text::Read<uint8_t*>(resources[0], 212);
        if (!texture || RE0Text::Read<uint32_t>(texture, 48) != D3DFMT_DXT5) return nullptr;
        const auto native = RE0Text::Read<IDirect3DTexture9*>(texture, 52);
        return native;
    }

    inline bool Copy(IDirect3DTexture9* texture, uint8_t* bytes, bool read)
    {
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, read ? D3DLOCK_READONLY : 0))) return false;
        if (!lock.pBits || lock.Pitch < 1024)
        {
            texture->UnlockRect(0);
            return false;
        }
        for (size_t row = 0; row < 64; ++row)
        {
            auto* pixels = static_cast<uint8_t*>(lock.pBits) + row * lock.Pitch;
            if (read) std::memcpy(bytes + row * 1024, pixels, 1024);
            else std::memcpy(pixels, bytes + row * 1024, 1024);
        }
        return SUCCEEDED(texture->UnlockRect(0));
    }

    inline void Forget()
    {
        if (savedTexture) savedTexture->Release();
        savedFont = nullptr;
        savedTexture = nullptr;
        savedDevice = nullptr;
        managed = false;
        applied = 0;
        hasTexture.store(false);
    }

    inline bool Restore(bool resetting = false)
    {
        applied = 0;
        if (savedTexture && !Copy(savedTexture, original.data(), false) && (!resetting || managed)) return false;
        Forget();
        return true;
    }

    inline void Compose(uint8_t* output, const uint8_t* replacement)
    {
        std::memcpy(output, original.data(), original.size());
        for (size_t i = 0; i < blocks.size(); ++i)
            if (blocks[i]) std::memcpy(output + i * 16, replacement + i * 16, 16);
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        if (!family.load() && !hasTexture.load()) return;
        const std::lock_guard lock(mutex);
        const auto selected = family.load();
        if (!selected || textureFailed)
        {
            Restore();
            return;
        }
        if (!ready || !assetsReady || selected < 1 || selected > 2) return;
        void* font = nullptr;
        IDirect3DTexture9* texture = nullptr;
        for (auto candidate : fonts)
            if ((texture = NativeTexture(candidate))) { font = candidate; break; }
        if (!texture)
        {
            if (savedTexture) Restore();
            return;
        }
        if (font != savedFont || texture != savedTexture || device != savedDevice)
        {
            if (!Restore() || !MakeBlocks(font)) return;
            D3DSURFACE_DESC desc{};
            IDirect3DDevice9* owner = nullptr;
            if (FAILED(texture->GetDevice(&owner))) return;
            const bool own = owner == device;
            owner->Release();
            if (!own || FAILED(texture->GetLevelDesc(0, &desc)) || desc.Width != 256 || desc.Height != 256 ||
                desc.Format != D3DFMT_DXT5 || !Copy(texture, original.data(), true)) return;
            texture->AddRef();
            savedFont = font;
            savedTexture = texture;
            savedDevice = device;
            managed = desc.Pool == D3DPOOL_MANAGED;
            hasTexture.store(true);
        }
        if (applied == selected) return;
        std::array<uint8_t, 65536> pixels{};
        Compose(pixels.data(), artwork.data() + 128 + (selected - 1) * original.size());
        if (Copy(texture, pixels.data(), false)) applied = selected;
        else
        {
            textureFailed = true;
            Restore();
            OutputDebugStringA("Resident Evil 0: controller artwork update failed.\n");
        }
    }

    inline char __fastcall Load(void* font, void*, void* stream)
    {
        const auto result = loadHook.thiscall<char>(font, stream);
        if (result && PadFont(font))
        {
            const std::lock_guard lock(mutex);
            fonts.insert(font);
        }
        return result;
    }

    inline uintptr_t __fastcall Release(void* font, void*)
    {
        const std::lock_guard lock(mutex);
        // The native cleanup owns destruction; it must not race a render-thread texture update.
        if (savedFont == font) savedFont = nullptr;
        fonts.erase(font);
        return releaseHook.thiscall<uintptr_t>(font);
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        const std::lock_guard lock(mutex);
        if (device == savedDevice) Restore(true);
        textureFailed = false;
    }

    inline void Disable()
    {
        ready = false;
        loadHook.reset();
        releaseHook.reset();
    }

    inline void Install(HMODULE module)
    {
        const auto update = []()
        {
            const auto selected = FixSettings().Int("MAIN.GamepadIcons");
            family.store(selected == 2 ? 1 : selected == 3 ? 2 : 0);
        };
        update();
        FixSettings().OnChange("MAIN.GamepadIcons", update);
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(module, path, DWORD(std::size(path)));
        std::ifstream file(std::filesystem::path(path).parent_path() / L"ResidentEvil0.FusionFix" / L"buttons.dds", std::ios::binary | std::ios::ate);
        if (file && file.tellg() == 131200)
        {
            artwork.resize(131200);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(artwork.data()), artwork.size());
            assetsReady = file && !std::memcmp(artwork.data(), "DDS ", 4) &&
                RE0Text::Read<uint32_t>(artwork.data(), 12) == 512 && RE0Text::Read<uint32_t>(artwork.data(), 16) == 256 &&
                !std::memcmp(artwork.data() + 84, "DXT5", 4);
        }
        if (assetsReady && Bind())
        {
            loadHook = safetyhook::create_inline(fontLoad, Load);
            if (loadHook) releaseHook = safetyhook::create_inline(fontRelease, Release);
            ready = loadHook && releaseHook;
            if (!ready) Disable();
        }
        FixSettings().Availability("MAIN.GamepadIcons", [](std::string_view value)
        {
            return value == "0" || value == "1" || (ready && assetsReady && !textureFailed);
        });
    }
}
