module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3dx9.h>
#include <array>
#include <mutex>
#include <vector>

export module ControllerIcons;

#pragma comment(lib, "d3dx9.lib")

// Controller glyphs live in the game's pad_win font atlas. Keep the original
// compressed blocks so changing styles never changes keyboard glyphs.
export namespace Rev2Icons
{
    inline std::mutex mutex;
    inline SafetyHookMid captureHook, releaseHook;
    inline uint8_t* font{};
    inline IDirect3DTexture9* atlas{};
    inline std::vector<uint8_t> original;
    inline int applied{};
    inline int failedStyle{-1};
    inline HMODULE module{};
    struct Icon { uint32_t glyph; int ps, nintendo; };
    inline constexpr std::array icons{
        Icon{'A',1000,1020}, Icon{'B',1001,1021}, Icon{'X',1002,1022}, Icon{'Y',1003,1023},
        Icon{'l',1004,1024}, Icon{'r',1005,1025}, Icon{'L',1006,1026}, Icon{'R',1007,1027},
        Icon{'s',1008,1028}, Icon{'b',1009,1029}, Icon{'1',1010,1030}, Icon{'2',1011,1031},
        Icon{'3',1012,1032}, Icon{'4',1013,1033}, Icon{'8',1014,0}, Icon{'9',1015,0}
    };
    template<class T> inline T Read(const void* p, size_t offset)
    { return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(p) + offset); }

    inline bool CopyAtlas(IDirect3DTexture9* texture, std::vector<uint8_t>& bytes, bool restore)
    {
        D3DSURFACE_DESC desc{};
        if (FAILED(texture->GetLevelDesc(0, &desc)) || desc.Width != 1024 || desc.Height != 1024 ||
            desc.Format != D3DFMT_DXT5 || desc.Pool != D3DPOOL_MANAGED || texture->GetLevelCount() != 1)
            return false;
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, restore ? 0 : D3DLOCK_READONLY))) return false;
        if (!restore) bytes.resize(4096 * 256);
        for (size_t y = 0; y != 256; ++y)
        {
            auto row = static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch;
            if (restore) std::memcpy(row, bytes.data() + y * 4096, 4096);
            else std::memcpy(bytes.data() + y * 4096, row, 4096);
        }
        texture->UnlockRect(0);
        return true;
    }
    inline void DropAtlas()
    {
        if (atlas)
        {
            if (applied && !original.empty()) CopyAtlas(atlas, original, true);
            atlas->Release();
        }
        atlas = nullptr; original.clear(); applied = 0; failedStyle = -1;
    }
    inline bool FindGlyph(uint8_t* object, uint32_t code, RECT& result)
    {
        auto glyphs = Read<uint8_t*>(object, 0xac);
        const auto count = Read<uint32_t>(object, 0xa0);
        if (!glyphs) return false;
        for (uint32_t i = 0; i < count; ++i)
        {
            auto entry = glyphs + i * 16;
            if (Read<uint32_t>(entry, 0) != code) continue;
            // The font stores X in pixels and Y in sixteen-pixel rows.
            const LONG x = Read<uint16_t>(entry, 5), y = Read<uint16_t>(entry, 7) * 16;
            const LONG width = entry[9], height = entry[12];
            result = { x, y, x + width, y + height };
            return width > 0 && height > 0 && x >= 0 && y >= 0 && x + width <= 1024 && y + height <= 1024 &&
                !(x % 4) && !(y % 4) && !(width % 4) && !(height % 4);
        }
        return false;
    }
    inline bool Apply(IDirect3DDevice9* device, uint8_t* object, int style)
    {
        IDirect3DSurface9* destination{};
        if (FAILED(atlas->GetSurfaceLevel(0, &destination))) return false;
        bool ok = true;
        for (const auto& icon : icons)
        {
            const int id = style == 2 ? icon.ps : icon.nintendo;
            if (!id) continue;
            RECT rect{}; IDirect3DTexture9* image{}; IDirect3DSurface9* source{};
            if (!FindGlyph(object, icon.glyph, rect) ||
                FAILED(D3DXCreateTextureFromResourceW(device, module, MAKEINTRESOURCEW(id), &image)) ||
                FAILED(image->GetSurfaceLevel(0, &source)) ||
                FAILED(D3DXLoadSurfaceFromSurface(destination, nullptr, &rect, source, nullptr, nullptr, D3DX_FILTER_LINEAR, 0)))
                ok = false;
            if (source) source->Release();
            if (image) image->Release();
            if (!ok) break;
        }
        destination->Release();
        return ok;
    }
    inline void Frame(IDirect3DDevice9* device)
    {
        std::lock_guard guard(mutex);
        const int style = std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3);
        if (!font || style < 2) { if (atlas) DropAtlas(); return; }
        auto resources = Read<uint8_t**>(font, 0xb4);
        if (!resources || !resources[0]) { if (atlas) DropAtlas(); return; }
        auto wrapper = Read<uint8_t*>(resources[0], 0xd4);
        if (!wrapper) { if (atlas) DropAtlas(); return; }
        auto texture = Read<IDirect3DTexture9*>(wrapper, 0x34);
        if (!texture) { if (atlas) DropAtlas(); return; }
        if (atlas != texture)
        {
            DropAtlas(); texture->AddRef(); atlas = texture;
            if (!CopyAtlas(atlas, original, false)) { DropAtlas(); return; }
        }
        if (applied == style || failedStyle == style) return;
        if (applied && !CopyAtlas(atlas, original, true)) return;
        applied = 0;
        if (Apply(device, font, style)) { applied = style; failedStyle = -1; }
        else { CopyAtlas(atlas, original, true); failedStyle = style; }
    }
    inline void BeforeReset()
    {
        std::lock_guard guard(mutex);
        DropAtlas();
    }
    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        auto capture = hook::pattern("8B 0D ? ? ? ? 53 8B 01 56 6A 01 68 ? ? ? ? 68 ? ? ? ? FF 50 30 8B F0 56 8B CF E8");
        captureHook = safetyhook::create_mid(capture.get_first(25), [](SafetyHookContext& r)
        { std::lock_guard guard(mutex); font = reinterpret_cast<uint8_t*>(r.eax); });
        auto release = hook::pattern("56 8B F1 E8 ? ? ? ? 8B 8E AC 00 00 00 85 C9 74 12 8B 06 51 8B CE FF 50 44");
        releaseHook = safetyhook::create_mid(release.get_first(), [](SafetyHookContext& r)
        { std::lock_guard guard(mutex); if (font == reinterpret_cast<uint8_t*>(r.ecx)) font = nullptr; });
    }
}
