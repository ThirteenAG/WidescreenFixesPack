module;

#include <stdafx.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <d3d9.h>
#include <optional>
#include <unordered_map>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/TonyHawk/PadIconArt.hpp"

// Out-of-the-box controller support. Player 1 starts on the keyboard, and the game reads a pad only after it's
// chosen under Options > Controls. Every action has a second binding that the controls screen doesn't show; it's
// filled with the bundled Xidi controller, so keyboard and pad both work without setup. Choosing the pad in the menu
// still gives the game's own pad layout.
// Prompts: the menu fonts have pictures of the Enter, Space, Backspace, Esc and arrow keys. While a pad is the last
// device used, those glyphs are drawn from a copy of the font texture that holds controller icons at their places.

export module Gamepad;

export namespace SRSPad
{
    // Binding record: device GUID, input code, analog flag, enabled flag, current and previous value
    struct Binding
    {
        GUID device;
        int32_t code;
        uint8_t analog;
        uint8_t enabled;
        uint8_t pad[2];
        float value;
        float previous;
    };
    static_assert(sizeof(Binding) == 32);

    constexpr size_t BindingsOffset = 916;
    constexpr size_t DeviceOffset = 5652;
    constexpr int32_t KeyboardDevice = 0;

    // Codes 0-31 are buttons; from 32 on each axis has a positive and a negative half
    enum Code : int32_t
    {
        PadA = 0, PadB = 1, PadX = 2, PadY = 3, PadLB = 4, PadRB = 5, PadBack = 6, PadLS = 8, PadRS = 9,
        StickRight = 32, StickLeft = 33, StickDown = 34, StickUp = 35, TriggerLeft = 36, TriggerRight = 42,
    };

    enum Action : int32_t
    {
        ChangeView = 1, RearView = 4, SteerLeft = 7, SteerRight = 8, Accelerate = 9, HandBrake = 11, Brake = 13,
        GearUp = 15, GearDown = 16, Nos = 17, Clutch = 18, HighBeam = 19,
        MenuUp = 22, MenuDown = 23, MenuLeft = 24, MenuRight = 25, MenuAccept = 26, MenuOption = 27, MenuBack = 28,
        MenuExtra = 29, ViewMap = 73,
    };

    struct DefaultBinding
    {
        Action action;
        Code code;
    };

    // The layout of the bundled Xidi.ini: triggers on their own half axes, d-pad on the left stick's axes
    constexpr DefaultBinding Defaults[] =
    {
        { Accelerate, TriggerRight }, { Brake, TriggerLeft }, { SteerLeft, StickLeft }, { SteerRight, StickRight },
        { HandBrake, PadX }, { Nos, PadA }, { GearUp, PadRB }, { GearDown, PadLB }, { Clutch, PadLS },
        { ChangeView, PadY }, { RearView, PadB }, { HighBeam, PadRS }, { ViewMap, PadBack },
        { MenuUp, StickUp }, { MenuDown, StickDown }, { MenuLeft, StickLeft }, { MenuRight, StickRight },
        { MenuAccept, PadA }, { MenuBack, PadB }, { MenuOption, PadX }, { MenuExtra, PadY },
    };

    void* (*GetInputAdapter)() = nullptr;
    void* PlayerOne = nullptr;
    SafetyHookInline ControllerHook, KeyboardDefaultsHook;

    // The first Xidi controller among the game's joysticks (devices 2 and up)
    std::optional<GUID> XidiController()
    {
        auto* adapter = static_cast<uint8_t*>(GetInputAdapter());
        if (!adapter)
            return std::nullopt;
        const int32_t devices = *reinterpret_cast<int32_t*>(adapter + 124);
        for (int32_t i = 0; i < devices - 2; ++i)
        {
            auto* device = *reinterpret_cast<IDirectInputDevice8A**>(adapter + 48 + 20 * i);
            if (!device)
                continue;
            DIDEVICEINSTANCEA info{};
            info.dwSize = sizeof(info);
            if (FAILED(device->GetDeviceInfo(&info)))
                continue;
            if (std::string_view(info.tszProductName).find("Xidi") != std::string_view::npos)
                return info.guidInstance;
        }
        return std::nullopt;
    }

    void AddPadBindings(void* controller)
    {
        auto* base = static_cast<uint8_t*>(controller);
        if (controller != PlayerOne || *reinterpret_cast<int32_t*>(base + DeviceOffset) != KeyboardDevice)
            return;
        const auto pad = XidiController();
        if (!pad)
            return;
        auto* bindings = reinterpret_cast<Binding*>(base + BindingsOffset);
        for (const auto& entry : Defaults)
        {
            auto& second = bindings[entry.action * 2 + 1];
            if (second.enabled)
                continue;
            second = {};
            second.device = *pad;
            second.code = entry.code;
            second.analog = entry.code >= StickRight;
            second.enabled = 1;
        }
    }

    void* __fastcall Controller(void* self, void*)
    {
        auto result = ControllerHook.unsafe_thiscall<void*>(self);
        // The first controller the game creates is player 1's
        if (!PlayerOne)
            PlayerOne = self;
        AddPadBindings(self);
        return result;
    }

    void __fastcall KeyboardDefaults(void* self, void*)
    {
        KeyboardDefaultsHook.unsafe_thiscall<void>(self);
        AddPadBindings(self);
    }


    // Controller icons in prompts

    // The engine's texture object: +4 references, +76 width, +80 height, +88 format, +92 the D3D texture
    constexpr size_t TextureObjectSize = 256;
    struct GlyphRecord
    {
        uint8_t* texture;
        int32_t width;
        float left, right, top, bottom;
        void* kerning;
    };
    static_assert(sizeof(GlyphRecord) == 28);

    struct IconTexture
    {
        uint8_t* object = nullptr;                  // a copy of the font's texture object drawing the icon texture
        IDirect3DTexture9* texture = nullptr;
        uint32_t painted = 0;                       // key glyphs already painted
    };
    struct FontTexture
    {
        std::array<IconTexture, 3> families;
    };

    inline std::atomic<bool> UsingPad{ false };
    inline std::atomic<int> IconFamily{ 0 };
    inline std::unordered_map<uint8_t*, FontTexture> FontTextures;   // original texture object -> icon copies
    inline std::unordered_map<uint8_t*, uint8_t*> Originals;         // icon copy -> original texture object
    inline SafetyHookInline GlyphHook;

    // Enter, Space, Backspace and Esc, then the four arrow keys
    inline int Icon(int character)
    {
        switch (character)
        {
            case 0x83: return 0;    // A: accept
            case 0x84: return 2;    // X
            case 0x85: return 3;    // Y
            case 0x86: return 1;    // B: back
            default: return 38;     // D-pad
        }
    }

    inline IconTexture* Icons(uint8_t* original, int family)
    {
        auto& entry = FontTextures[original].families[family - 1];
        if (entry.object)
            return &entry;
        auto* source = *reinterpret_cast<IDirect3DTexture9**>(original + 92);
        const auto width = *reinterpret_cast<uint32_t*>(original + 76);
        const auto height = *reinterpret_cast<uint32_t*>(original + 80);
        if (!source || !width || !height || width > 4096 || height > 4096)
            return nullptr;
        IDirect3DDevice9* device = nullptr;
        if (FAILED(source->GetDevice(&device)) || !device)
            return nullptr;
        IDirect3DTexture9* texture = nullptr;
        const auto created = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr);
        device->Release();
        if (FAILED(created) || !texture)
            return nullptr;
        D3DLOCKED_RECT pixels{};
        if (SUCCEEDED(texture->LockRect(0, &pixels, nullptr, 0)))
        {
            for (uint32_t row = 0; row < height; ++row)
                std::memset(static_cast<uint8_t*>(pixels.pBits) + row * pixels.Pitch, 0, width * 4);
            texture->UnlockRect(0);
        }
        // One copy per font texture and icon family, kept for the whole session; the engine never releases it
        auto* object = new uint8_t[TextureObjectSize];
        std::memcpy(object, original, TextureObjectSize);
        *reinterpret_cast<int32_t*>(object + 4) = 0x10000;
        *reinterpret_cast<uint32_t*>(object + 88) = D3DFMT_A8R8G8B8;
        *reinterpret_cast<IDirect3DTexture9**>(object + 92) = texture;
        entry.object = object;
        entry.texture = texture;
        Originals[object] = original;
        return &entry;
    }

    inline void Paint(IconTexture& icons, uint8_t* original, const GlyphRecord& glyph, int character, int family)
    {
        const uint32_t bit = 1u << (character - 0x83);
        if (icons.painted & bit)
            return;
        icons.painted |= bit;
        const int width = int(*reinterpret_cast<uint32_t*>(original + 76));
        const int height = int(*reinterpret_cast<uint32_t*>(original + 80));
        const int x0 = std::clamp(int(glyph.left * width + 0.5f), 0, width);
        const int x1 = std::clamp(int(glyph.right * width + 0.5f), 0, width);
        const int y0 = std::clamp(int(glyph.top * height + 0.5f), 0, height);
        const int y1 = std::clamp(int(glyph.bottom * height + 0.5f), 0, height);
        if (x1 <= x0 || y1 <= y0)
            return;
        D3DLOCKED_RECT pixels{};
        if (FAILED(icons.texture->LockRect(0, &pixels, nullptr, 0)))
            return;
        TonyHawkPadIcons::Paint(static_cast<uint32_t*>(pixels.pBits), pixels.Pitch / 4, { x0, y0, x1 - x0, y1 - y0 },
            Icon(character), family);
        icons.texture->UnlockRect(0);
    }

    // The font's glyph drawing: key glyphs take the icon texture while a pad is in use, their own texture otherwise
    inline void __fastcall DrawGlyph(uint8_t* font, void*, int character, float x, float y, int depth, int color)
    {
        if (character >= 0x83 && character <= 0x8A)
        {
            const auto index = font[60 + character];
            auto* records = *reinterpret_cast<GlyphRecord**>(font + 52);
            if (index < 0xFE && records)
            {
                auto& glyph = records[index];
                auto* original = glyph.texture;
                if (auto found = Originals.find(original); found != Originals.end())
                    original = found->second;
                auto* shown = original;
                if (UsingPad.load(std::memory_order_relaxed) && original)
                {
                    const int family = static_cast<int>(PadInput::ResolveFamily(std::clamp(IconFamily.load(std::memory_order_relaxed), 0, 3)));
                    if (auto* icons = Icons(original, family))
                    {
                        Paint(*icons, original, glyph, character, family);
                        shown = icons->object;
                    }
                }
                glyph.texture = shown;
            }
        }
        GlyphHook.unsafe_thiscall<void>(font, character, x, y, depth, color);
    }

    // Keyboard or mouse input in the game window brings back the keys
    inline void InputMessage(UINT message, LPARAM position)
    {
        static LPARAM lastPosition = -1;
        // Windows also sends a move when the window or the clip area changes; only a new position counts
        if (message == WM_MOUSEMOVE && std::exchange(lastPosition, position) == position)
            return;
        if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN
            || message == WM_RBUTTONDOWN || message == WM_MOUSEWHEEL)
            UsingPad.store(false, std::memory_order_relaxed);
    }

    // Last device used: pad input switches the prompts to the pad
    inline void UpdateDevice()
    {
        static DWORD packets[XUSER_MAX_COUNT]{};
        static ULONGLONG retry[XUSER_MAX_COUNT]{};
        auto getState = PadInput::XInput().getState;
        if (!getState)
            return;
        const auto now = GetTickCount64();
        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            if (now < retry[slot])
                continue;
            XINPUT_STATE state{};
            if (getState(slot, &state) != ERROR_SUCCESS)
            {
                // Asking an empty slot is slow: look again a second later
                retry[slot] = now + 1000;
                packets[slot] = 0;
                continue;
            }
            const auto& pad = state.Gamepad;
            const bool active = pad.wButtons || pad.bLeftTrigger > 30 || pad.bRightTrigger > 30
                || std::abs(int(pad.sThumbLX)) > 12000 || std::abs(int(pad.sThumbLY)) > 12000
                || std::abs(int(pad.sThumbRX)) > 12000 || std::abs(int(pad.sThumbRY)) > 12000;
            if (packets[slot] && state.dwPacketNumber != packets[slot] && active)
                UsingPad.store(true, std::memory_order_relaxed);
            packets[slot] = state.dwPacketNumber;
        }
    }

    void Init()
    {
        auto adapter = hook::pattern("E8 ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 39 68 7C");
        GetInputAdapter = reinterpret_cast<void* (*)()>(injector::GetBranchDestination(adapter.get_first()).as_int());
        auto controller = hook::pattern("53 55 56 57 8B F1 E8 ? ? ? ? 8D 7E 1C 8B CF C7 06 ? ? ? ? E8 ? ? ? ? C7 07 ? ? ? ? 8D 86 A8 03 00 00 B9");
        ControllerHook = safetyhook::create_inline(controller.get_first(), Controller);
        auto keyboardDefaults = hook::pattern("83 EC 10 53 55 56 57 8B D9 E8 ? ? ? ? 8B 03 8B CB FF 50 28 A1");
        KeyboardDefaultsHook = safetyhook::create_inline(keyboardDefaults.get_first(), KeyboardDefaults);
        BindLiveSetting("MAIN.GamepadIcons", IconFamily);
        auto glyph = hook::pattern("51 8B 44 24 08 53 56 8B D9 0F B6 74 18 3C 81 FE FF 00 00 00 57 75 04 0F B6 73 3D");
        GlyphHook = safetyhook::create_inline(glyph.get_first(), DrawGlyph);
    }
}
