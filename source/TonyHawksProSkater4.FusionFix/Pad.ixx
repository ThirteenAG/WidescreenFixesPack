module;

#include <stdafx.h>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "Shared/TonyHawk/PadIconArt.hpp"

export module Pad;

import ComVars;

// The game reads pads through DirectInput (the bundled Xidi). The fix picks the pad when none is chosen yet and
// shows pad buttons in the prompts while the pad is the last device used.
namespace THPS4Pad
{
    struct Device
    {
        DWORD size;
        GUID instance, product;
        DWORD type;
    };

    constexpr size_t PlayerSize = 192;
    constexpr size_t BindingCount = 22;

    SafetyHookInline EnumerateHook;
    SafetyHookMid StateHook, DrawKeyNameHook, WidthKeyNameHook, TextureHook;
    std::array<std::array<uint8_t, 0x110>, 2> Previous{};
    const int* Family = nullptr;
    int (__cdecl* Digit)(const char*) = nullptr;
    int Pending = -1;
    IDirect3DDevice8* AtlasDevice = nullptr;
    IDirect3DTexture8* Atlas[3][TonyHawkPadIcons::Count]{};

    GUID* Selected(int player)
    {
        return reinterpret_cast<GUID*>(PlayerControls.get_ptr() + PlayerSize * player);
    }

    uint32_t* Bindings(int player)
    {
        return reinterpret_cast<uint32_t*>(Selected(player) + 1);
    }

    bool IsXidi(const GUID& product)
    {
        return (product.Data1 & 0xFFFFu) == 0xFFFFu && product.Data2 == 0 && product.Data3 == 0 &&
               product.Data4[0] == 'X' && product.Data4[1] == 'I' && product.Data4[2] == 'D' &&
               product.Data4[3] == 'I' && product.Data4[4] == 'V' && product.Data4[5] == 'C';
    }

    BOOL __stdcall Enumerate(const Device* device, void* context)
    {
        // Without a chosen pad (no launcher setup) the first Xidi pad goes to player 1, the next to player 2, with the
        // layout of the console game: A ollie, X flip, B grab, Y grind, bumpers spin, right trigger revert
        if (IsXidi(device->product))
        {
            for (int player = 0; player < 2; ++player)
            {
                if (IsEqualGUID(*Selected(player), device->instance))
                    break;
                if (!IsEqualGUID(*Selected(player), GUID_NULL))
                    continue;
                *Selected(player) = device->instance;
                auto* bindings = Bindings(player);
                if (std::all_of(bindings, bindings + BindingCount, [](uint32_t code) { return code == 0; }))
                {
                    constexpr std::array<uint32_t, BindingCount> defaults{
                        2, 2, 1, 1, 11, 14, 12, 13, 15, 16, 18, 20, 8, 8, 3, 3, 0, 0, 0, 0, 0, 0
                    };
                    std::copy(defaults.begin(), defaults.end(), bindings);
                }
                break;
            }
        }
        return EnumerateHook.stdcall<BOOL>(device, context);
    }

    // DIJOYSTATE2 as read by the game for one player
    void Inspect(int player, const uint8_t* state)
    {
        if (player < 0 || player > 1)
            return;
        auto& previous = Previous[player];
        const auto window = WindowedModeWrapper::GameHWND;
        bool active = false;
        if (window && GetForegroundWindow() == window)
        {
            for (int i = 48; i < 80 && !active; ++i)
                active = (state[i] & 0x80) && !(previous[i] & 0x80);
            const auto* axes = reinterpret_cast<const int32_t*>(state);
            const auto* old = reinterpret_cast<const int32_t*>(previous.data());
            for (int i : { 0, 1, 2, 5 })
                active = active || (std::abs(axes[i]) > 500 && std::abs(axes[i] - old[i]) > 200);
            active = active || (axes[8] != -1 && axes[8] != old[8]);
        }
        std::memcpy(previous.data(), state, previous.size());
        if (active)
        {
            ActivePadPlayer.store(player, std::memory_order_relaxed);
            UsingPad.store(true, std::memory_order_relaxed);
        }
    }

    // Button font glyphs: 0-3 the face buttons (grind, flip, grab, ollie), 4-7 down/right/left/up, 8 pause, 9 back,
    // 10-13 diagonals, 14-17 the shoulder actions. Each maps to the binding it stands for.
    int GlyphIcon(int glyph)
    {
        if (!UsingPad.load(std::memory_order_relaxed) || glyph < 0 || glyph >= 18)
            return -1;
        const int player = std::clamp(ActivePadPlayer.load(std::memory_order_relaxed), 0, 1);
        const auto* bindings = Bindings(player);
        if (glyph >= 10 && glyph <= 13)
            return TonyHawkPadIcons::Diagonal(bindings, glyph - 10);
        constexpr int actions[18]{ 5, 7, 6, 4, 1, 3, 2, 0, 11, 6, -1, -1, -1, -1, 8, 9, 10, -1 };
        const int action = actions[glyph];
        if (action < 0)
            return -1;
        const bool directional = action <= 3 || (action >= 12 && action <= 19);
        return TonyHawkPadIcons::Icon(bindings[action], directional, (action % 2) == 0);
    }

    int __cdecl ParseButton(const char* text)
    {
        const int glyph = Digit(text);
        Pending = GlyphIcon(glyph);
        return glyph;
    }

    IDirect3DTexture8* GetAtlas(uint8_t* font, int icon)
    {
        auto* native = *reinterpret_cast<IDirect3DTexture8**>(font + 372);
        if (!native)
            return nullptr;
        IDirect3DDevice8* device = nullptr;
        if (FAILED(native->GetDevice(&device)))
            return nullptr;
        if (device != AtlasDevice)
        {
            for (auto& family : Atlas)
            {
                for (auto*& texture : family)
                {
                    if (texture)
                    {
                        texture->Release();
                        texture = nullptr;
                    }
                }
            }
            if (AtlasDevice)
                AtlasDevice->Release();
            AtlasDevice = device;
        }
        else
        {
            device->Release();
        }
        const int family = std::clamp(*Family, 0, 3);
        const int index = family == 0 ? 0 : family - 1;
        auto*& result = Atlas[index][icon];
        if (result)
            return result;
        D3DSURFACE_DESC desc{};
        if (FAILED(native->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height)
            return nullptr;
        IDirect3DTexture8* texture = nullptr;
        if (FAILED(AtlasDevice->CreateTexture(desc.Width, desc.Height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture)))
            return nullptr;
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, 0)))
        {
            texture->Release();
            return nullptr;
        }
        for (UINT y = 0; y < desc.Height; ++y)
            std::memset(static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch, 0, desc.Width * 4);
        const auto* glyphs = *reinterpret_cast<const uint8_t* const*>(font + 8);
        const auto* map = font + 268;
        for (int i = 0; i < 18; ++i)
        {
            const auto* uv = reinterpret_cast<const float*>(glyphs + 24 * map[i]);
            TonyHawkPadIcons::Rect r{ static_cast<int>(uv[0] * desc.Width), static_cast<int>(uv[1] * desc.Height),
                static_cast<int>((uv[2] - uv[0]) * desc.Width + 0.5f), static_cast<int>((uv[3] - uv[1]) * desc.Height + 0.5f) };
            if (r.x >= 0 && r.y >= 0 && r.x + r.w <= static_cast<int>(desc.Width) && r.y + r.h <= static_cast<int>(desc.Height))
                TonyHawkPadIcons::Paint(static_cast<uint32_t*>(lock.pBits), lock.Pitch / 4, r, icon, index + 1);
        }
        texture->UnlockRect(0);
        return result = texture;
    }
}

export namespace THPS4Pad
{
    void Install()
    {
        Family = &FixSettings().Int("MAIN.GamepadIcons");

        auto enumerate = hook::pattern("53 55 8B 6C 24 0C 56 57 8B 5D 24 81 E3 FF 00 00 00 83 FB 15 74 09 83 FB 14");
        PlayerControls.SetAddress(*enumerate.get_first<uint8_t*>(42));
        EnumerateHook = safetyhook::create_inline(enumerate.get_first(), Enumerate);

        auto state = hook::pattern("8D 3C 76 C6 45 10 01 C1 E7 06 81 C7");
        StateHook = safetyhook::create_mid(state.get_first(), [](SafetyHookContext& regs)
        {
            Inspect(static_cast<int>(regs.esi), reinterpret_cast<const uint8_t*>(regs.esp + 0x28));
        });

        // Button glyphs (\b and \d) of the text renderer; \d also appends the bound key's name, left out for the pad
        auto drawButton = hook::pattern("43 53 E8 ? ? ? ? 8B 2D ? ? ? ? 33 C9 83 C4 04 C6 44 24 13 01");
        auto drawKey = hook::pattern("43 53 E8 ? ? ? ? 8B 2D ? ? ? ? 33 C9 89 44 24 38");
        auto drawKeyName = hook::pattern("83 F8 FF 74 0C C1 E0 05 05 ? ? ? ? 89 44 24 24");
        auto widthKeyName = hook::pattern("83 F8 FF 74 0C C1 E0 05 05 ? ? ? ? 89 44 24 14");
        auto flush = hook::pattern("A1 ? ? ? ? 3B C3 75 03 8B 41 14 8B 80 74 01 00 00 53 50 53 E8");
        ButtonFont.SetAddress(*drawButton.get_first<uint8_t**>(9));
        BatchFont.SetAddress(*flush.get_first<uint8_t**>(1));
        Digit = reinterpret_cast<decltype(Digit)>(injector::GetBranchDestination(drawButton.get_first(2)).as_int());
        injector::MakeCALL(drawButton.get_first(2), ParseButton, true);
        injector::MakeCALL(drawKey.get_first(2), ParseButton, true);
        DrawKeyNameHook = safetyhook::create_mid(drawKeyName.get_first(), [](SafetyHookContext& regs)
        {
            if (UsingPad.load(std::memory_order_relaxed))
                regs.eax = static_cast<uintptr_t>(-1);
        });
        WidthKeyNameHook = safetyhook::create_mid(widthKeyName.get_first(), [](SafetyHookContext& regs)
        {
            if (UsingPad.load(std::memory_order_relaxed))
                regs.eax = static_cast<uintptr_t>(-1);
        });
        // A button glyph is drawn as a batch of its own; it gets the icon's copy of the font texture
        TextureHook = safetyhook::create_mid(flush.get_first(18), [](SafetyHookContext& regs)
        {
            if (!ButtonFont.get() || BatchFont.get() != ButtonFont.get())
                return;
            const int icon = std::exchange(Pending, -1);
            if (icon < 0)
                return;
            if (auto* atlas = GetAtlas(ButtonFont.get(), icon))
                regs.eax = reinterpret_cast<uintptr_t>(atlas);
        });
    }
}
