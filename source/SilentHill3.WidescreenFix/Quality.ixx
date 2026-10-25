module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/SilentHill/Display.hpp"

export module Quality;

// Image quality beyond the PS2's: the soft shadows, the depth of field and blur effects and the blurred pause
// background get render targets sized for modern resolutions, the fog layer gets finer, and world textures get
// anisotropic filtering. 0 keeps the game's own; the render target sizes apply at the next start, the fog and the
// filtering at once. A flaw in a character texture is also cleaned up.

namespace
{
    // Father Vincent's glasses (#758): the lens patch of his hair texture (data/pcchr/ch/chvaa.mdl, texture 3, the
    // 32x32 texels at 224,54) has outlines drawn in at a higher alpha than the lens, which show as little green and
    // purple squares on his glasses. They are painted over with the lens colour of their row once the game filled
    // the texture (its copy turns the file's RGBA into BGRA and doubles the PS2 alpha: the lens is 0x62).
    using LockRectFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DTexture8*, UINT, D3DLOCKED_RECT*, const RECT*, DWORD);
    using UnlockRectFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DTexture8*, UINT);
    LockRectFn OriginalLockRect = nullptr;
    UnlockRectFn OriginalUnlockRect = nullptr;
    std::map<IDirect3DTexture8*, D3DLOCKED_RECT> Locked;
    std::mutex LockedMutex;

    uint32_t Crc32(uint32_t crc, const uint8_t* data, size_t size)
    {
        static uint32_t table[256] = {};
        if (!table[1])
        {
            for (uint32_t i = 0; i < 256; ++i)
            {
                uint32_t c = i;
                for (int k = 0; k < 8; ++k)
                    c = (c & 1) ? 0xEDB88320 ^ (c >> 1) : c >> 1;
                table[i] = c;
            }
        }
        crc = ~crc;
        for (size_t i = 0; i < size; ++i)
            crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        return ~crc;
    }

    void FixVincentLens(uint8_t* bits, int pitch)
    {
        uint32_t crc = 0;
        for (int y = 0; y < 256; ++y)
            crc = Crc32(crc, bits + y * pitch, 256 * 4);
        if (crc != 0x320ABBA0)
            return;
        for (int y = 54; y <= 85; ++y)
        {
            auto row = reinterpret_cast<uint32_t*>(bits + y * pitch);
            std::map<uint32_t, int> counts;     // the row's lens colour: its most common colour at the lens alpha
            for (int x = 224; x < 256; ++x)
            {
                uint32_t c = row[x];
                if ((c >> 24) == 0x62)
                    ++counts[c];
            }
            uint32_t lens = 0;
            int best = 0;
            for (auto [c, n] : counts)
            {
                if (n > best)
                {
                    best = n;
                    lens = c;
                }
            }
            if (!lens)
                continue;
            for (int x = 224; x < 256; ++x)
            {
                uint32_t& c = row[x];
                uint32_t a = c >> 24;
                if (a > 0x62 && a < 0xFF)
                    c = lens;
            }
        }
    }

    HRESULT STDMETHODCALLTYPE LockRect(IDirect3DTexture8* texture, UINT level, D3DLOCKED_RECT* locked, const RECT* rect, DWORD flags)
    {
        HRESULT result = OriginalLockRect(texture, level, locked, rect, flags);
        if (SUCCEEDED(result) && level == 0 && !rect && locked && !(flags & D3DLOCK_READONLY))
        {
            D3DSURFACE_DESC desc = {};
            if (SUCCEEDED(texture->GetLevelDesc(0, &desc)) && desc.Width == 256 && desc.Height == 256 && desc.Format == D3DFMT_A8R8G8B8)
            {
                std::lock_guard lock(LockedMutex);
                Locked[texture] = *locked;
            }
        }
        return result;
    }

    HRESULT STDMETHODCALLTYPE UnlockRect(IDirect3DTexture8* texture, UINT level)
    {
        if (level == 0)
        {
            D3DLOCKED_RECT locked = {};
            {
                std::lock_guard lock(LockedMutex);
                if (auto it = Locked.find(texture); it != Locked.end())
                {
                    locked = it->second;
                    Locked.erase(it);
                }
            }
            if (locked.pBits)
                FixVincentLens(static_cast<uint8_t*>(locked.pBits), locked.Pitch);
        }
        return OriginalUnlockRect(texture, level);
    }

    void HookTextureUploads(IDirect3DDevice8* device)
    {
        if (OriginalLockRect)
            return;
        IDirect3DTexture8* probe = nullptr;
        if (FAILED(device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &probe)) || !probe)
            return;
        auto vtable = *reinterpret_cast<void***>(probe);
        OriginalLockRect = reinterpret_cast<LockRectFn>(vtable[16]);
        OriginalUnlockRect = reinterpret_cast<UnlockRectFn>(vtable[17]);
        injector::WriteMemory(&vtable[16], &LockRect, true);
        injector::WriteMemory(&vtable[17], &UnlockRect, true);
        probe->Release();
    }
}

export namespace Quality
{
    void Init()
    {
        int shadows = FixSettings().Int("GRAPHICS.ShadowResolution");
        int blur = FixSettings().Int("GRAPHICS.DOFRes");
        int pause = FixSettings().Int("GRAPHICS.StatusScreenRes");

        SHDisplay::SetAnisotropy(FixSettings().Int("GRAPHICS.AnisotropicFiltering"));
        FixSettings().OnChange("GRAPHICS.AnisotropicFiltering", []() { SHDisplay::SetAnisotropy(FixSettings().Int("GRAPHICS.AnisotropicFiltering")); });
        SHDisplay::OnDeviceReady().push_back(&HookTextureUploads);

        // Post effect render target sizes: slot 0 = pause background, slots 2 and 3 = blur and depth of field
        auto slots = hook::pattern("8B 3C 85 ? ? ? ? 8B 04 85 ? ? ? ? 56 6A 00 6A 15 6A 01 6A 01");
        if (slots.size() == 1)
        {
            auto heights = *slots.get_first<int32_t*>(3);
            auto widths = *slots.get_first<int32_t*>(10);
            if (pause > 0)
            {
                injector::WriteMemory(&widths[0], pause, true);
                injector::WriteMemory(&heights[0], pause, true);
            }
            if (blur > 0)
            {
                for (int slot : { 2, 3 })
                {
                    injector::WriteMemory(&widths[slot], blur, true);
                    injector::WriteMemory(&heights[slot], blur, true);
                }
            }
        }

        // The pause background capture: its render target, copy loop and quad
        auto capture = hook::pattern("BE ? ? ? ? 56 56 89 5C 24 24 89 5C 24 28 89 5C 24 20 8B 08 50");
        auto copy = hook::pattern("BE ? ? ? ? 8D A4 24 00 00 00 00 8B 2C 01 89 28 83 C0 04 4E");
        if (pause > 0 && capture.size() == 1 && copy.size() == 1)
        {
            auto start = reinterpret_cast<uintptr_t>(capture.get_first());
            auto quad = hook::range_pattern(start, start + 0x400, "68 ? ? ? ? 6A 02 6A 05 50 FF 91 20 01 00 00");
            if (quad.size() == 1)
            {
                injector::WriteMemory(capture.get_first(1), pause, true);
                injector::WriteMemory(copy.get_first(1), pause, true);
                auto vertices = *quad.get_first<uint8_t*>(1);
                for (int offset : { 0x00, 0x30, 0x34, 0x4C })
                    injector::WriteMemory(vertices + offset, float(pause), true);
            }
        }

        // Soft shadows: the render target, its depth buffer and the quads of the shadow pass
        if (shadows > 0)
        {
            auto target = hook::pattern("68 00 01 00 00 68 00 01 00 00 50 FF 51 50 85 C0 7D 04 33 C0 5E C3");
            auto depth = hook::pattern("6A 00 6A 4B 68 00 01 00 00 68 00 01 00 00 50 FF 51 68 C3");
            auto depthReset = hook::pattern("6A 00 6A 4B 68 00 01 00 00 68 00 01 00 00 50 FF 52 68 6A 00 68 ? ? ? ? E8");
            auto pass = hook::pattern("64 A1 ? ? ? ? 6A FF 68 ? ? ? ? 50 8B 44 24 10");
            if (target.size() == 1 && depth.size() == 1 && depthReset.size() == 1 && pass.size() == 1)
            {
                injector::WriteMemory(target.get_first(1), shadows, true);
                injector::WriteMemory(target.get_first(6), shadows, true);
                for (auto site : { depth.get_first<uint8_t>(), depthReset.get_first<uint8_t>() })
                {
                    injector::WriteMemory(site + 5, shadows, true);
                    injector::WriteMemory(site + 10, shadows, true);
                }
                auto start = reinterpret_cast<uintptr_t>(pass.get_first());
                auto stack = hook::range_pattern(start, start + 0x1200, "C7 44 24 ? 00 00 80 43");
                for (size_t i = 0; i < stack.size(); ++i)
                    injector::WriteMemory(stack.get(i).get<void>(4), float(shadows), true);
                auto vertices = hook::range_pattern(start, start + 0x1200, "C7 05 ? ? ? ? 00 00 80 43");
                for (size_t i = 0; i < vertices.size(); ++i)
                    injector::WriteMemory(vertices.get(i).get<void>(6), float(shadows), true);
            }
        }

        // Finer fog layer: the game divides by its own constant (25), the fix by its value, 0 being the game's
        auto fogScale = hook::pattern("D8 3D ? ? ? ? D9 1D ? ? ? ? 83 C4 08 C3");
        if (fogScale.size() == 1)
        {
            static float Native = **fogScale.get_first<float*>(2);
            static float Fog = Native;
            auto update = []()
            {
                int value = FixSettings().Int("GRAPHICS.FogComplexity");
                Fog = value > 0 ? float(value) : Native;
            };
            update();
            FixSettings().OnChange("GRAPHICS.FogComplexity", update);
            injector::WriteMemory(fogScale.get_first(2), &Fog, true);
        }
    }
}
