module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3dx9.h>
#include <array>
#include <atomic>
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
    inline uint8_t* atlasFont{};
    inline IDirect3DTexture9* atlas{};
    inline std::vector<uint8_t> original;
    inline int applied{};
    inline std::atomic<int> iconStyle{};
    inline int unsupportedStyles{};
    inline bool unsupportedAtlas{};
    inline bool dirty{};
    inline ULONGLONG retryAt{};
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

    inline void RetryLater()
    {
        retryAt = GetTickCount64() + 250;
    }

    inline bool CopyAtlas(bool restore)
    {
        constexpr size_t rowBytes = 4096;
        constexpr size_t rowCount = 256;
        if (!restore)
            original.resize(rowBytes * rowCount);
        D3DLOCKED_RECT lock{};
        if (FAILED(atlas->LockRect(0, &lock, nullptr, restore ? 0 : D3DLOCK_READONLY)))
            return false;
        const bool usable = lock.pBits && lock.Pitch >= static_cast<INT>(rowBytes);
        if (usable)
        {
            for (size_t y = 0; y < rowCount; ++y)
            {
                auto row = static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch;
                if (restore)
                    std::memcpy(row, original.data() + y * rowBytes, rowBytes);
                else
                    std::memcpy(original.data() + y * rowBytes, row, rowBytes);
            }
        }
        return SUCCEEDED(atlas->UnlockRect(0)) && usable;
    }

    inline bool Restore()
    {
        if (dirty && !CopyAtlas(true))
        {
            RetryLater();
            return false;
        }
        dirty = false;
        applied = 0;
        return true;
    }

    inline bool DropAtlas()
    {
        if (!Restore())
            return false;
        if (atlas)
            atlas->Release();
        atlas = nullptr;
        atlasFont = nullptr;
        original.clear();
        unsupportedStyles = 0;
        unsupportedAtlas = false;
        retryAt = 0;
        return true;
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
    inline bool Apply(IDirect3DDevice9* device, int style, const std::array<RECT, icons.size()>& rects)
    {
        IDirect3DSurface9* destination{};
        if (FAILED(atlas->GetSurfaceLevel(0, &destination)))
            return false;
        bool ok = true;
        for (size_t i = 0; i < icons.size(); ++i)
        {
            const int id = style == 2 ? icons[i].ps : icons[i].nintendo;
            if (!id)
                continue;
            IDirect3DTexture9* image{};
            IDirect3DSurface9* source{};
            ok = SUCCEEDED(D3DXCreateTextureFromResourceW(device, module, MAKEINTRESOURCEW(id), &image)) &&
                SUCCEEDED(image->GetSurfaceLevel(0, &source)) &&
                SUCCEEDED(D3DXLoadSurfaceFromSurface(destination, nullptr, &rects[i], source, nullptr, nullptr, D3DX_FILTER_LINEAR, 0));
            if (source)
                source->Release();
            if (image)
                image->Release();
            if (!ok)
                break;
        }
        destination->Release();
        return ok;
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        // Xbox icons are the game's own: nothing to do until another style is chosen (the atlas is only
        // changed on the render thread).
        if (iconStyle.load(std::memory_order_relaxed) < 2 && !atlas)
            return;
        const std::lock_guard guard(mutex);
        if (retryAt && GetTickCount64() < retryAt)
            return;

        const int style = iconStyle.load(std::memory_order_relaxed);
        const auto resources = font && style >= 2 ? Read<uint8_t**>(font, 0xB4) : nullptr;
        const auto wrapper = resources && resources[0] ? Read<uint8_t*>(resources[0], 0xD4) : nullptr;
        const auto texture = wrapper ? Read<IDirect3DTexture9*>(wrapper, 0x34) : nullptr;
        if (atlas != texture || (atlas && atlasFont != font))
        {
            if (!DropAtlas())
                return;
            if (texture)
            {
                texture->AddRef();
                atlas = texture;
                atlasFont = font;
            }
        }
        if (!atlas || unsupportedAtlas || applied == style || !Restore())
            return;
        if (unsupportedStyles & (1 << style))
            return;

        if (original.empty())
        {
            D3DSURFACE_DESC desc{};
            if (FAILED(atlas->GetLevelDesc(0, &desc)))
            {
                RetryLater();
                return;
            }
            if (desc.Width != 1024 || desc.Height != 1024 || desc.Format != D3DFMT_DXT5 ||
                desc.Pool != D3DPOOL_MANAGED || atlas->GetLevelCount() != 1)
            {
                unsupportedAtlas = true;
                return;
            }
            if (!CopyAtlas(false))
            {
                original.clear();
                RetryLater();
                return;
            }
        }

        std::array<RECT, icons.size()> rects{};
        for (size_t i = 0; i < icons.size(); ++i)
        {
            const int id = style == 2 ? icons[i].ps : icons[i].nintendo;
            if (id && !FindGlyph(font, icons[i].glyph, rects[i]))
            {
                unsupportedStyles |= 1 << style;
                return;
            }
        }
        dirty = true;
        if (Apply(device, style, rects))
        {
            applied = style;
            retryAt = 0;
        }
        else
        {
            Restore();
            RetryLater();
        }
    }

    inline void BeforeReset()
    {
        std::lock_guard guard(mutex);
        DropAtlas();
    }
    inline void Init()
    {
        BindLiveInt("MAIN.GamepadIcons", [](int value)
        {
            iconStyle.store(std::clamp(value, 0, 3), std::memory_order_relaxed);
        });
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
