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

export namespace RevIcons
{
    struct Icon { uint32_t glyph; int playstation, nintendo; };
    constexpr std::array icons{
        Icon{ 'b', 1000, 1020 }, Icon{ 'a', 1001, 1021 }, Icon{ 'd', 1002, 1022 }, Icon{ 'c', 1003, 1023 },
        Icon{ 'e', 1004, 1024 }, Icon{ 'w', 1005, 1025 }, Icon{ 'K', 1006, 1026 }, Icon{ 'L', 1007, 1027 },
        Icon{ 'G', 1008, 1028 }, Icon{ 'H', 1009, 1029 }
    };

    std::mutex mutex;
    SafetyHookMid loadedExternal, loadedEmbedded;
    SafetyHookInline cleanup;
    uint8_t* font = nullptr;
    uint8_t* atlasFont = nullptr;
    IDirect3DTexture9* atlas = nullptr;
    std::vector<uint8_t> original;
    HMODULE module = nullptr;
    int applied = 0;
    std::atomic<int> iconStyle{};
    int unsupportedStyles{};
    bool unsupportedAtlas{};
    bool dirty{};
    ULONGLONG retryAt{};

    template<typename T> T Read(const void* p, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(p) + offset);
    }

    void RetryLater()
    {
        retryAt = GetTickCount64() + 250;
    }

    bool CopyAtlas(bool restore)
    {
        constexpr size_t rowBytes = 2048;
        constexpr size_t rowCount = 64;
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

    bool Restore()
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

    bool DropAtlas()
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

    bool FindGlyph(uint8_t* object, uint32_t code, RECT& rect)
    {
        const auto glyphs = Read<uint8_t*>(object, 0xAC);
        const auto count = Read<uint32_t>(object, 0xA0);
        if (!glyphs) return false;
        for (uint32_t i = 0; i < count; ++i)
        {
            const auto entry = glyphs + i * 16;
            if (Read<uint32_t>(entry, 0) != code) continue;
            const auto position = Read<uint32_t>(entry, 4), size = Read<uint32_t>(entry, 8);
            const LONG x = (position >> 8) & 4095, y = position >> 20;
            const LONG width = (size >> 8) & 4095, height = size >> 20;
            rect = { x, y, x + width, y + height };
            return !(position & 255) && width > 0 && height > 0 && x + width <= 512 && y + height <= 256 &&
                !(x % 4) && !(y % 4) && !(width % 4) && !(height % 4);
        }
        return false;
    }

    bool Apply(IDirect3DDevice9* device, int style, const std::array<RECT, icons.size()>& rects)
    {
        IDirect3DSurface9* destination{};
        if (FAILED(atlas->GetSurfaceLevel(0, &destination)))
            return false;
        bool ok = true;
        for (size_t i = 0; i < icons.size(); ++i)
        {
            const int id = style == 2 ? icons[i].playstation : icons[i].nintendo;
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

    void Frame(IDirect3DDevice9* device)
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
            if (desc.Width != 512 || desc.Height != 256 || desc.Format != D3DFMT_DXT5 ||
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
            const int id = style == 2 ? icons[i].playstation : icons[i].nintendo;
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

    void BeforeReset()
    {
        const std::lock_guard guard(mutex);
        DropAtlas();
    }

    void Capture(SafetyHookContext& regs)
    {
        auto object = reinterpret_cast<uint8_t*>(regs.esi);
        const auto name = Read<const char*>(object, 0xB0);
        if (!name || _stricmp(name, "id_bhr\\00_font\\BTN_win") != 0 || Read<uint32_t>(object, 0xA0) != 163)
            return;
        const std::lock_guard guard(mutex);
        font = object;
    }

    int __fastcall Cleanup(void* object, void*)
    {
        const std::lock_guard guard(mutex);
        if (font == object) font = nullptr;
        return cleanup.unsafe_thiscall<int>(object);
    }

    class ControllerIcons
    {
    public:
        ControllerIcons()
        {
            WFP::onInitEvent() += []()
            {
                const auto updateStyle = []()
                {
                    iconStyle.store(std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3), std::memory_order_relaxed);
                };
                updateStyle();
                FixSettings().OnChange("MAIN.GamepadIcons", updateStyle);
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(&Capture), &module);
                cleanup = safetyhook::create_inline(hook::pattern("53 56 8B F1 E8 ? ? ? ? 8B 86 AC 00 00 00 33 DB 3B C3 74 10 8B 16 50 8B 42 44 8B CE FF").get_first(), Cleanup);
                loadedExternal = safetyhook::create_mid(hook::pattern("E8 ? ? ? ? 5F 5D 5B B0 01 5E 83 C4 38 C2 04 00 8B 8E 9C").get_first(5), Capture);
                loadedEmbedded = safetyhook::create_mid(hook::pattern("E8 ? ? ? ? 5F 5D 5B B0 01 5E 83 C4 38 C2 04 00 8B 16 8B").get_first(5), Capture);
            };
        }
    } controllerIcons;
}
