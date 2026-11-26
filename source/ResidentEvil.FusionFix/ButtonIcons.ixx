module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <unordered_set>

export module ButtonIcons;

import EngineText;
import IconBindings;

export namespace REIcons
{
    inline std::atomic<int> family{ 0 };
    inline std::vector<uint8_t> artwork;
    inline std::array<uint8_t, 131072> original{};
    inline IDirect3DTexture9* savedTexture = nullptr;
    inline IDirect3DDevice9* savedDevice = nullptr;
    inline int applied = 0;
    inline bool managedTexture = false;
    inline bool assetsReady = false;
    inline bool dirty = false;
    inline ULONGLONG retryAt{};
    inline std::atomic<bool> hasTexture{ false };
    inline std::mutex mutex;
    inline SafetyHookInline loadHook;
    inline SafetyHookInline releaseHook;
    inline std::unordered_set<void*> fonts;

    inline bool Copy(IDirect3DTexture9* texture, uint8_t* bytes, bool read)
    {
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, read ? D3DLOCK_READONLY : 0))) return false;
        if (!lock.pBits || lock.Pitch < 1024)
        {
            texture->UnlockRect(0);
            return false;
        }
        for (size_t row = 0; row < 128; ++row)
        {
            auto* pixels = static_cast<uint8_t*>(lock.pBits) + row * lock.Pitch;
            if (read) std::memcpy(bytes + row * 1024, pixels, 1024);
            else std::memcpy(pixels, bytes + row * 1024, 1024);
        }
        return SUCCEEDED(texture->UnlockRect(0));
    }

    inline void RetryLater()
    {
        retryAt = GetTickCount64() + 250;
    }

    inline bool Restore()
    {
        if (dirty && !Copy(savedTexture, original.data(), false))
        {
            RetryLater();
            return false;
        }
        dirty = false;
        applied = 0;
        return true;
    }

    inline bool DropTexture(bool resetting = false)
    {
        if (!Restore() && (!resetting || managedTexture)) return false;
        if (savedTexture) savedTexture->Release();
        savedTexture = nullptr;
        savedDevice = nullptr;
        applied = 0;
        dirty = false;
        retryAt = 0;
        hasTexture.store(false);
        managedTexture = false;
        return true;
    }

    inline bool PadFont(void* font)
    {
        if (!font || REText::Read<uint32_t>(font, 152) != 68614 ||
            REText::Read<uint32_t>(font, 156) != 1 || REText::Read<uint32_t>(font, 160) != 116 ||
            (REText::Read<uint32_t>(font, 192) & 1)) return false;
        const auto glyphs = REText::Read<uint8_t*>(font, 172);
        if (!glyphs) return false;
        unsigned valid = 0;
        for (size_t i = 0; i < 116; ++i)
        {
            const auto record = glyphs + i * 16;
            const auto code = REText::Read<uint32_t>(record, 0);
            if (code < 'a' || code > 'm' || code == 'i') continue;
            const auto position = REText::Read<uint32_t>(record, 4);
            const auto size = REText::Read<uint32_t>(record, 8);
            const auto index = code - 'a';
            if ((position & 255) || ((position >> 8) & 4095) != (index % 8) * 30 ||
                (position >> 20) != (index / 8) * 30 || ((size >> 8) & 4095) != 30 || (size >> 20) != 30) return false;
            valid |= 1u << index;
        }
        return valid == 0x1EFF;
    }

    inline IDirect3DTexture9* NativeTexture(void* font)
    {
        const auto resources = REText::Read<uint8_t**>(font, 180);
        if (!resources || !resources[0]) return nullptr;
        const auto texture = REText::Read<uint8_t*>(resources[0], 212);
        if (!texture || REText::Read<uint32_t>(texture, 48) != MAKEFOURCC('D', 'X', 'T', '5')) return nullptr;
        const auto native = REText::Read<IDirect3DTexture9*>(texture, 52);
        return native;
    }

    inline void Compose(uint8_t* output, const uint8_t* native, const uint8_t* replacement)
    {
        std::memcpy(output, native, original.size());
        for (size_t row = 0; row < 15; ++row)
            for (size_t column = 0; column < 60; ++column)
                if (row < 8 || (column >= 7 && column < 38))
                {
                    const auto offset = (row * 64 + column) * 16;
                    std::memcpy(output + offset, replacement + offset, 16);
                }
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        if (!family.load() && !hasTexture.load()) return;
        const std::lock_guard lock(mutex);
        if (retryAt && GetTickCount64() < retryAt) return;
        retryAt = 0;
        const auto selected = family.load();
        if (!selected)
        {
            DropTexture();
            return;
        }
        if (!assetsReady) return;
        IDirect3DTexture9* texture = nullptr;
        for (const auto candidate : fonts)
            if ((texture = NativeTexture(candidate))) break;
        if (!texture)
        {
            if (savedTexture) DropTexture();
            return;
        }
        if (texture != savedTexture || device != savedDevice)
        {
            if (!DropTexture()) return;
            D3DSURFACE_DESC desc{};
            IDirect3DDevice9* owner = nullptr;
            if (FAILED(texture->GetDevice(&owner)))
            {
                RetryLater();
                return;
            }
            const bool own = owner == device;
            owner->Release();
            if (!own || FAILED(texture->GetLevelDesc(0, &desc)) || desc.Width != 256 || desc.Height != 512 ||
                desc.Format != D3DFMT_DXT5 || !Copy(texture, original.data(), true))
            {
                RetryLater();
                return;
            }
            texture->AddRef();
            savedTexture = texture;
            savedDevice = device;
            managedTexture = desc.Pool == D3DPOOL_MANAGED;
            hasTexture.store(true);
        }
        if (applied == selected) return;
        if (dirty && !applied && !Restore()) return;
        std::vector<uint8_t> pixels(original.size());
        Compose(pixels.data(), original.data(), artwork.data() + 128 + (selected - 1) * original.size());
        // Unlock can fail after the texture has already been written.
        dirty = true;
        applied = 0;
        if (Copy(texture, pixels.data(), false))
            applied = selected;
        else
        {
            Restore();
            RetryLater();
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
        fonts.erase(font);
        return releaseHook.thiscall<uintptr_t>(font);
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        const std::lock_guard lock(mutex);
        retryAt = 0;
        if (device == savedDevice) DropTexture(true);
    }

    inline void Install(HMODULE module)
    {
        BindLiveInt("MAIN.GamepadIcons", [](int value)
        {
            family.store(value == 2 ? 1 : value == 3 ? 2 : 0);
        });
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(module, path, DWORD(std::size(path)));
        std::ifstream file(std::filesystem::path(path).parent_path() / L"ResidentEvil.FusionFix" / L"buttons.dds", std::ios::binary | std::ios::ate);
        if (file && file.tellg() == 262272)
        {
            artwork.resize(262272);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(artwork.data()), artwork.size());
            assetsReady = file && !std::memcmp(artwork.data(), "DDS ", 4) &&
                REText::Read<uint32_t>(artwork.data(), 12) == 1024 && REText::Read<uint32_t>(artwork.data(), 16) == 256 &&
                !std::memcmp(artwork.data() + 84, "DXT5", 4);
        }
        if (assetsReady)
        {
            Bind();
            loadHook = safetyhook::create_inline(fontLoad, Load);
            releaseHook = safetyhook::create_inline(fontRelease, Release);
        }
        else OutputDebugStringA("Resident Evil: controller artwork file unavailable.\n");
        FixSettings().Availability("MAIN.GamepadIcons", [](std::string_view value)
        {
            return value == "0" || value == "1" || assetsReady;
        });
    }
}
