module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3dx9.h>
#include <array>
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
    IDirect3DTexture9* atlas = nullptr;
    std::vector<uint8_t> original;
    HMODULE module = nullptr;
    int applied = 0;
    int failedStyle = -1;
    bool dirty = false;

    template<typename T> T Read(const void* p, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(p) + offset);
    }

    bool CopyAtlas(IDirect3DTexture9* texture, bool restore)
    {
        D3DSURFACE_DESC desc{};
        if (FAILED(texture->GetLevelDesc(0, &desc)) || desc.Width != 512 || desc.Height != 256 ||
            desc.Format != D3DFMT_DXT5 || desc.Pool != D3DPOOL_MANAGED || texture->GetLevelCount() != 1)
            return false;
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, restore ? 0 : D3DLOCK_READONLY))) return false;
        if (!lock.pBits || lock.Pitch < 2048)
        {
            texture->UnlockRect(0);
            return false;
        }
        if (!restore) original.resize(131072);
        for (size_t y = 0; y < 64; ++y)
        {
            auto row = static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch;
            if (restore) std::memcpy(row, original.data() + y * 2048, 2048);
            else std::memcpy(original.data() + y * 2048, row, 2048);
        }
        return SUCCEEDED(texture->UnlockRect(0));
    }

    bool DropAtlas()
    {
        if (atlas)
        {
            if (dirty && !original.empty())
            {
                applied = 0;
                if (!CopyAtlas(atlas, true)) return false;
            }
            atlas->Release();
        }
        atlas = nullptr;
        original.clear();
        applied = 0;
        failedStyle = -1;
        dirty = false;
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

    bool Apply(IDirect3DDevice9* device, int style)
    {
        IDirect3DSurface9* destination = nullptr;
        if (FAILED(atlas->GetSurfaceLevel(0, &destination))) return false;
        bool ok = true;
        for (const auto& icon : icons)
        {
            RECT rect{};
            IDirect3DTexture9* image = nullptr;
            IDirect3DSurface9* source = nullptr;
            const int id = style == 2 ? icon.playstation : icon.nintendo;
            if (!FindGlyph(font, icon.glyph, rect) ||
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

    void Frame(IDirect3DDevice9* device)
    {
        const std::lock_guard guard(mutex);
        const int style = std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 0, 3);
        if (!font || style < 2) { if (atlas) DropAtlas(); return; }
        const auto resources = Read<uint8_t**>(font, 0xB4);
        if (!resources || !resources[0]) { if (atlas) DropAtlas(); return; }
        const auto wrapper = Read<uint8_t*>(resources[0], 0xD4);
        if (!wrapper) { if (atlas) DropAtlas(); return; }
        const auto texture = Read<IDirect3DTexture9*>(wrapper, 0x34);
        if (!texture) { if (atlas) DropAtlas(); return; }
        if (atlas != texture)
        {
            if (!DropAtlas()) return;
            texture->AddRef();
            atlas = texture;
            if (!CopyAtlas(atlas, false)) { DropAtlas(); return; }
        }
        if (applied == style) return;
        if (dirty)
        {
            applied = 0;
            if (!CopyAtlas(atlas, true)) return;
        }
        dirty = false;
        applied = 0;
        if (failedStyle == style) return;
        dirty = true;
        if (Apply(device, style)) { applied = style; failedStyle = -1; }
        else { if (CopyAtlas(atlas, true)) dirty = false; failedStyle = style; }
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
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(&Capture), &module);
                cleanup = safetyhook::create_inline(hook::pattern("53 56 8B F1 E8 ? ? ? ? 8B 86 AC 00 00 00 33 DB 3B C3 74 10 8B 16 50 8B 42 44 8B CE FF").get_first(), Cleanup);
                loadedExternal = safetyhook::create_mid(hook::pattern("E8 ? ? ? ? 5F 5D 5B B0 01 5E 83 C4 38 C2 04 00 8B 8E 9C").get_first(5), Capture);
                loadedEmbedded = safetyhook::create_mid(hook::pattern("E8 ? ? ? ? 5F 5D 5B B0 01 5E 83 C4 38 C2 04 00 8B 16 8B").get_first(5), Capture);
                if (!loadedExternal || !loadedEmbedded || !cleanup)
                    throw std::runtime_error("Resident Evil Revelations: controller icon hooks could not be installed");
            };
        }
    } controllerIcons;
}
