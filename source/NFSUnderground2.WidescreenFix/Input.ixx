module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>
#include "TexturePack.hpp"
#include "FixSettings.hpp"
#include "../Shared/NFS/StickDeadzone.hpp"

export module Input;

import ComVars;

// Gamepads are handled by Xidi (Xidi.32.dll, loaded by the ASI loader, with the SDL3 plugin for PlayStation,
// Switch and other non-XInput pads). Xidi presents each pad as a "Xidi virtual controller" using its
// XInputNative layout: X/Y = left stick, RotX/RotY = right stick, Z = LT, RotZ = RT, POV = d-pad,
// buttons 1-10 = A B X Y LB RB Back Start LS RS. This module gives such pads modern default bindings,
// proper names in the controls menu, controller button prompts in the front end, front end actions for
// the PC-only hotkeys and the right stick for the camera. Other devices (wheels and so on) keep the
// game's own behaviour.

namespace
{
    // Game input action IDs. A channel's 64-bit action mask has a bit cleared for every pressed action.
    enum Action : uint32_t
    {
        ActionNone = 0x00,
        ActionHandbrake = 0x05,
        ActionNitrous = 0x06,
        ActionShiftUp = 0x07,
        ActionShiftDown = 0x08,
        ActionLookBack = 0x09,
        ActionCamera = 0x0A,
        ActionHUD = 0x0B,
        ActionPause = 0x0C,
        ActionRespot = 0x0E,
        ActionEngage = 0x0F,
        ActionWorldMap = 0x10,
        ActionSMS = 0x11,
        ActionSteerLeft = 0x03,
        ActionSteerRight = 0x04,
        ActionThrottle = 0x01,
        ActionBrake = 0x02,
        ActionMenuSelect = 0x1F,
        ActionMenuHelp = 0x21,          // H
        ActionMenuBack = 0x22,
        ActionMenuCustomise = 0x23,     // C
        ActionMenuButton2 = 0x24,       // ,
        ActionMenuL1 = 0x26,            // [
        ActionMenuR1 = 0x27,            // ]
        ActionMenuPreset = 0x2E,        // P
        ActionMenuQuit = 0x2F,          // Q
    };

    // Per-channel input mapping
    namespace Channel
    {
        constexpr size_t ActionMaskLow = 0x08;
        constexpr size_t ActionMaskHigh = 0x0C;
        constexpr size_t SteerByte = 0x12;
        constexpr size_t PedalsByte = 0x13;
        constexpr size_t ButtonGameAction = 0x18;           // [16]
        constexpr size_t PovGameAction = 0x58;              // [4][4]: down, up, left, right
        constexpr size_t ButtonMenuAction = 0x98;           // [16]
        constexpr size_t PovMenuAction = 0xD8;              // [4][4]
        constexpr size_t ButtonGameActionSecondary = 0x118; // [16]
        constexpr size_t PovGameActionSecondary = 0x158;    // [4][4]
        constexpr size_t ButtonMenuActionSecondary = 0x198; // [16]
        constexpr size_t Index = 0x230;
        constexpr size_t DeviceType = 0x234;
        constexpr size_t ConfigMode = 0x33C;
        constexpr size_t AxisOffset = 0x344;                // [4]: steer, throttle, brake, combined pedals
        constexpr size_t AxisSign = 0x354;                  // [4]
        constexpr size_t AxisEnabled = 0x364;               // [4] bytes
        constexpr size_t CombinedPedals = 0x368;

        constexpr int32_t KeyboardType = 0x13;

        template<typename T> T& At(uintptr_t channel, size_t offset) { return *reinterpret_cast<T*>(channel + offset); }
    }

    constexpr size_t JoyStateSize = 0x110;
    constexpr int MaxJoysticks = 2;

    // DIJOYSTATE2 offsets of the XInputNative mapper
    constexpr DWORD OfsLeftX = 0x00, OfsLeftY = 0x04, OfsLT = 0x08;
    constexpr DWORD OfsRightX = 0x0C, OfsRightY = 0x10, OfsRT = 0x14;
    constexpr DWORD OfsButton0 = 0x30;

    enum PadButton { PadA, PadB, PadX, PadY, PadLB, PadRB, PadBack, PadStart, PadLS, PadRS, PadButtonCount };

    IDirectInputDevice8A** Joysticks = nullptr;
    uint8_t** JoyStateCopy = nullptr;

    enum class IconStyle { Xbox, PlayStation };
    const int* pGamepadIcons = nullptr;   // 0 = auto, 1 = Xbox, 2 = PlayStation; also changed in the options menu
    std::atomic<IconStyle> CurrentIconStyle = IconStyle::Xbox;
    std::atomic<bool> bDevicesChanged = false;     // rescan for new game controllers
    std::atomic<bool> bIconStyleOutdated = false;  // detect the icon style again

    bool IsXidiDevice(IDirectInputDevice8A* device)
    {
        // Xidi virtual controller instance GUIDs end with "XIDIVC"
        static std::map<IDirectInputDevice8A*, bool> cache;
        if (!device)
            return false;

        if (auto it = cache.find(device); it != cache.end())
            return it->second;

        DIDEVICEINSTANCEA info = { sizeof(DIDEVICEINSTANCEA) };
        bool isXidi = SUCCEEDED(device->GetDeviceInfo(&info)) && memcmp(info.guidInstance.Data4, "XIDIVC", 6) == 0;
        cache[device] = isXidi;
        return isXidi;
    }

    IDirectInputDevice8A* GetJoystick(int index)
    {
        return (index >= 0 && index < MaxJoysticks) ? Joysticks[index] : nullptr;
    }

    // The stick dead zone from the options, applied by Xidi to an axis of the left or right stick
    void SetStickDeadzone(IDirectInputDevice8A* device, DWORD object, DWORD how, DWORD offset)
    {
        DIPROPDWORD deadzone = {};
        deadzone.diph.dwSize = sizeof(DIPROPDWORD);
        deadzone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        deadzone.diph.dwObj = object;
        deadzone.diph.dwHow = how;
        deadzone.dwData = WFPStickDeadzone::Property(FixSettings().Float(offset == OfsLeftX || offset == OfsLeftY ? "GAMEPAD.LeftStickDeadzone" : "GAMEPAD.RightStickDeadzone"));
        device->SetProperty(DIPROP_DEADZONE, &deadzone.diph);
    }

    DIJOYSTATE2* GetJoyState(int index)
    {
        return (JoyStateCopy && *JoyStateCopy) ? reinterpret_cast<DIJOYSTATE2*>(*JoyStateCopy + JoyStateSize * index) : nullptr;
    }

    void SetActionPressed(uintptr_t channel, uint32_t action, bool pressed)
    {
        auto& mask = Channel::At<uint32_t>(channel, action < 32 ? Channel::ActionMaskLow : Channel::ActionMaskHigh);
        uint32_t bit = 1u << (action % 32);
        mask = pressed ? (mask & ~bit) : (mask | bit);
    }
}

// Icon style detection
namespace
{
    IconStyle DetectIconStyle()
    {
        if (*pGamepadIcons == 1)
            return IconStyle::Xbox;
        if (*pGamepadIcons == 2)
            return IconStyle::PlayStation;

        // An XInput pad wins, otherwise a connected Sony gamepad selects PlayStation names
        using XInputGetStateFn = DWORD(WINAPI*)(DWORD, void*);
        static auto XInputGetStateFunc = []() -> XInputGetStateFn
        {
            for (auto dll : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" })
            {
                if (auto module = LoadLibraryA(dll))
                    return reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"));
            }
            return nullptr;
        }();

        if (XInputGetStateFunc)
        {
            uint8_t state[16] = {};
            for (DWORD i = 0; i < 4; ++i)
            {
                if (XInputGetStateFunc(i, state) == ERROR_SUCCESS)
                    return IconStyle::Xbox;
            }
        }

        UINT count = 0;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count)
        {
            std::vector<RAWINPUTDEVICELIST> devices(count);
            if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) != (UINT)-1)
            {
                for (UINT i = 0; i < count; ++i)
                {
                    if (devices[i].dwType != RIM_TYPEHID)
                        continue;

                    RID_DEVICE_INFO info = { sizeof(RID_DEVICE_INFO) };
                    UINT size = sizeof(info);
                    if (GetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICEINFO, &info, &size) == (UINT)-1)
                        continue;

                    constexpr DWORD SonyVendorId = 0x054C;
                    constexpr USHORT GenericDesktopPage = 0x01, Joystick = 0x04, Gamepad = 0x05;
                    if (info.hid.dwVendorId == SonyVendorId && info.hid.usUsagePage == GenericDesktopPage &&
                        (info.hid.usUsage == Gamepad || info.hid.usUsage == Joystick))
                        return IconStyle::PlayStation;
                }
            }
        }

        return IconStyle::Xbox;
    }
}

// Input device tracking: prompts follow the last device used
namespace
{
    using SteadyClock = std::chrono::steady_clock;
    SteadyClock::time_point LastPadInput = {};
    SteadyClock::time_point LastKeyboardMouseInput = {};

    bool LastInputWasPad()
    {
        return LastPadInput > LastKeyboardMouseInput;
    }

    struct PadSnapshot
    {
        LONG axes[6] = {};
        DWORD pov = 0xFFFFFFFF;
        BYTE buttons[PadButtonCount] = {};
        bool valid = false;
    } PreviousPad[MaxJoysticks];

    void TrackPadActivity(int index, const DIJOYSTATE2& state)
    {
        constexpr LONG AxisActivity = 2500;
        PadSnapshot now;
        now.valid = true;
        memcpy(now.axes, &state.lX, sizeof(now.axes));
        now.pov = state.rgdwPOV[0];
        memcpy(now.buttons, state.rgbButtons, sizeof(now.buttons));

        auto& before = PreviousPad[index];
        if (!before.valid)
        {
            // The first state is the reference, not input
            before = now;
            return;
        }

        bool active = now.pov != before.pov || memcmp(now.buttons, before.buttons, sizeof(now.buttons)) != 0;
        for (int i = 0; i < 6 && !active; ++i)
            active = std::abs(now.axes[i] - before.axes[i]) > AxisActivity;

        if (active)
        {
            before = now;
            LastPadInput = SteadyClock::now();
        }
    }
}

// Right stick and mouse, used by the camera
export namespace Input
{
    // The stick dead zones changed in the options menu
    void ApplyStickDeadzone()
    {
        if (!Joysticks)
            return;
        for (int i = 0; i < MaxJoysticks; ++i)
        {
            auto device = GetJoystick(i);
            if (!IsXidiDevice(device))
                continue;
            for (DWORD offset : { OfsLeftX, OfsLeftY, OfsRightX, OfsRightY })
                SetStickDeadzone(device, offset, DIPH_BYOFFSET, offset);
        }
    }

    // The icon style setting changed
    void RefreshIconStyle()
    {
        bIconStyleOutdated = true;
    }

    struct LookInput
    {
        float stickX = 0.0f;    // -1..1, right is positive
        float stickY = 0.0f;    // -1..1, down is positive
        int mouseX = 0;
        int mouseY = 0;
    };

    LookInput Look;

    // Called once per camera update; mouse deltas accumulate between calls
    LookInput ConsumeLook()
    {
        LookInput result = Look;
        Look.mouseX = 0;
        Look.mouseY = 0;
        return result;
    }

    bool UsingGamepad()
    {
        return LastInputWasPad();
    }
}

// Controller button names
namespace
{
    struct PadNames
    {
        const char* buttons[PadButtonCount];
        const char* leftTrigger;
        const char* rightTrigger;
    };

    constexpr PadNames XboxNames = { { "A", "B", "X", "Y", "LB", "RB", "View", "Menu", "LS", "RS" }, "LT", "RT" };
    constexpr PadNames PlayStationNames = { { "Cross", "Circle", "Square", "Triangle", "L1", "R1", "Share", "Options", "L3", "R3" }, "L2", "R2" };

    const PadNames& CurrentNames()
    {
        return CurrentIconStyle == IconStyle::PlayStation ? PlayStationNames : XboxNames;
    }

    const char* ObjectName(DWORD offset)
    {
        const auto& names = CurrentNames();
        switch (offset)
        {
            case OfsLeftX: return "Left Stick";
            case OfsLeftY: return "Left Stick Y";
            case OfsRightX: return "Right Stick";
            case OfsRightY: return "Right Stick Y";
            case OfsLT: return names.leftTrigger;
            case OfsRT: return names.rightTrigger;
        }

        if (offset >= OfsButton0 && offset < OfsButton0 + PadButtonCount)
            return names.buttons[offset - OfsButton0];

        return nullptr;
    }
}

// Front end button prompts: when a pad was used last, prompts get the controller button in front,
// like on consoles. Matched by text-bible hash, or by the localized text for strings set by code, so
// this works in every language. The button is a "$TEXTURE$" token: the PC font renderer still parses
// these tokens from the console versions, which draw them as images, but skips them. ButtonIcons
// draws them again.
namespace
{
    constexpr int PromptQuit = -1; // both stick clicks

    struct Prompt
    {
        uint32_t hash;
        int button;
    };

    constexpr Prompt Prompts[] =
    {
        { 0x1C101778, PromptQuit }, // Quit Game
        { 0x9F1B2C7A, PadA },       // Continue
        { 0xDE02CF84, PadA },
        { 0x1335F0, PadB },         // Back
        { 0x1C0787E6, PadB },
        { 0x933DD695, PadB },
        { 0xB7D0D273, PadY },       // Reset Keys To Default
        { 0x43512519, PadX },       // Activate GPS
        { 0x629B0D0F, PadX },
        { 0x11159F62, PadX },       // Deactivate GPS
        { 0x2A795CD8, PadX },
        { 0x9E067677, PadA },       // Install Package
        { 0x7EFF16C2, PadA },       // Install Part
        { 0x5E0AB884, PadA },       // Install Decal
        { 0xEFDDC126, PadX },       // Delete Tuned Car
        { 0x545C5292, PadB },       // Logoff
        { 0x18127BD, PadX },        // Cancel Changes
        { 0x8D5ABF58, PadX },       // Customize
        { 0xA53039A3, PadX },
        { 0x80AF4FC7, PadX },       // Host LAN Server
        { 0x3B550C71, PadA },       // Read Message
        { 0xE24B4BD5, PadA },
        { 0x4100ADA8, PadX },       // Delete
        { 0x56B00632, PadX },
        { 0x66516470, PadX },
        { 0xBA0E1020, PadX },       // Test N2O Purge
        { 0x4F876B6F, PadA },       // Accept
        { 0xAD89EA7B, PadX },       // Reset to default
        { 0x79AD657F, PadA },       // Select
        { 0x8202C364, PadA },
        { 0x3E9431DC, PadX },       // Open/Close Doors
        { 0xEAA1D5DF, PadX },       // Open/Close Hood
    };

    // Button textures, see ButtonIcons
    constexpr const char* XboxIcons[PadButtonCount] = { "BUTTON_A", "BUTTON_B", "BUTTON_X", "BUTTON_Y", "BUTTON_LB", "BUTTON_RB", "BUTTON_VIEW", "BUTTON_MENU", "BUTTON_LS", "BUTTON_RS" };
    constexpr const char* PlayStationIcons[PadButtonCount] = { "BUTTON_CROSS", "BUTTON_CIRCLE", "BUTTON_SQUARE", "BUTTON_TRIANGLE", "BUTTON_L1", "BUTTON_R1", "BUTTON_SHARE", "BUTTON_OPTIONS", "BUTTON_L3", "BUTTON_R3" };

    // GetLocalizedWideString(buffer, size in bytes, hash)
    bool (__cdecl* GetLocalizedWideString)(wchar_t* buffer, int size, uint32_t hash) = nullptr;

    const Prompt* FindPrompt(uint32_t hash)
    {
        auto it = std::find_if(std::begin(Prompts), std::end(Prompts), [hash](const Prompt& p) { return p.hash == hash; });
        return it != std::end(Prompts) ? &*it : nullptr;
    }

    // Dialog buttons and other strings set by code have no text hash, they're matched by their text
    const Prompt* FindPrompt(const wchar_t* text)
    {
        static std::vector<std::pair<std::wstring, const Prompt*>> texts;
        if (texts.empty() && GetLocalizedWideString)
        {
            for (const auto& prompt : Prompts)
            {
                wchar_t buffer[256] = {};
                if (GetLocalizedWideString(buffer, sizeof(buffer), prompt.hash) && buffer[0])
                    texts.emplace_back(buffer, &prompt);
            }
        }

        auto it = std::find_if(texts.begin(), texts.end(), [text](const auto& entry) { return entry.first == text; });
        return it != texts.end() ? it->second : nullptr;
    }

    // The text of a front end string with the button in front, or nullptr
    const wchar_t* PromptText(uintptr_t feString, const wchar_t* text)
    {
        if (!text || text[0] == L'$' || !LastInputWasPad())
            return nullptr;

        constexpr size_t LanguageHash = 0x60, Flags = 0x1C;
        const Prompt* prompt = nullptr;
        if ((*reinterpret_cast<uint8_t*>(feString + Flags) & 2) == 0)
            prompt = FindPrompt(*reinterpret_cast<uint32_t*>(feString + LanguageHash));
        if (!prompt)
            prompt = FindPrompt(text);
        if (!prompt)
            return nullptr;

        const auto& icons = CurrentIconStyle == IconStyle::PlayStation ? PlayStationIcons : XboxIcons;
        const char* icon = prompt->button == PromptQuit ? "BUTTON_QUIT" : icons[prompt->button];

        static std::wstring result;
        result = L"$";
        for (const char* c = icon; *c; ++c)
            result += wchar_t(*c);
        result += L"$ ";
        result += text;
        return result.c_str();
    }

    // Strings are only laid out again when they change. When the input switches between pad and
    // keyboard, all of them are laid out again for a moment.
    bool bLastPromptsForPad = false;
    SteadyClock::time_point RefreshStringsUntil = {};

    void UpdatePromptMode()
    {
        if (bIconStyleOutdated.exchange(false))
            CurrentIconStyle = DetectIconStyle();

        bool pad = LastInputWasPad();
        static IconStyle lastStyle = CurrentIconStyle;
        if (pad != bLastPromptsForPad || lastStyle != CurrentIconStyle)
        {
            bLastPromptsForPad = pad;
            lastStyle = CurrentIconStyle;
            RefreshStringsUntil = SteadyClock::now() + std::chrono::milliseconds(250);
        }
    }
}

// Button images in front end text, as on consoles: "$BUTTON_A$" draws the BUTTON_A texture in
// place of the token, as big as a line of text. The textures are the DDS files in the "TPKDDS"
// resources, packed into a texture pack in memory and loaded with the game's chunk loader.
namespace ButtonIcons
{
    void* (__cdecl* GetTextureInfo)(uint32_t hash, int returnDefault, int includeUnloaded) = nullptr;
    bool (__cdecl* LoadChunks)(void* data, int size) = nullptr;
    void* AddPoly = nullptr;    // FERenderObject, thiscall (x0, y0, x1, y1, z, u0, v0, u1, v1, color[4], renderInfo, flag) -> poly
    constexpr size_t PolyTexture = 36 * 4;  // texture of a poly, used instead of the render object's texture

    void LoadTextures()
    {
        static bool bLoaded = false;
        if (bLoaded || !LoadChunks)
            return;
        bLoaded = true;

        auto textures = TexturePack::LoadResourceTextures();
        if (textures.empty())
            return;

        // The texture pack keeps pointing into this memory, so it's never freed
        auto pack = TexturePack::Build(textures, "WFP_NFSU2_BUTTONS");
        auto buffer = static_cast<uint8_t*>(_aligned_malloc(pack.size(), 0x80));
        if (!buffer)
            return;
        memcpy(buffer, pack.data(), pack.size());
        LoadChunks(buffer, int(pack.size()));
    }

    // The texture of a "$NAME$" token, the text pointing at the first '$'
    void* TokenTexture(const wchar_t* text)
    {
        char name[64] = {};
        size_t length = 0;
        for (++text; *text && *text != L'$' && length < sizeof(name) - 1; ++text)
            name[length++] = char(towupper(*text));

        if (length < 7 || strncmp(name, "BUTTON_", 7) != 0)
            return nullptr;

        LoadTextures();
        return GetTextureInfo ? GetTextureInfo(bStringHash(name), 0, 0) : nullptr;
    }

    // Icons are square and as high as a line of text
    float IconSize(uintptr_t font)
    {
        return *reinterpret_cast<float*>(font + 0x24);
    }
}

// Controls menu labels for Xidi pads
namespace
{
    SafetyHookInline shGetJoystickBindingName = {};
    const char* __fastcall GetJoystickBindingName(uintptr_t channel, void* edx, int action, int secondary)
    {
        auto index = Channel::At<int32_t>(channel, Channel::Index);
        if (!IsXidiDevice(GetJoystick(index)))
            return shGetJoystickBindingName.unsafe_fastcall<const char*>(channel, edx, action, secondary);

        // Same search as the game: axis bindings first, then buttons, then d-pad directions.
        // A secondary binding is the second match.
        bool skipFirst = secondary != 0;
        auto match = [&]() -> bool
        {
            if (skipFirst)
            {
                skipFirst = false;
                return false;
            }
            return true;
        };

        auto enabled = [&](int i) { return Channel::At<uint8_t>(channel, Channel::AxisEnabled + i) != 0; };
        bool combined = Channel::At<uint8_t>(channel, Channel::CombinedPedals) != 0;
        int axis = -1;
        if (enabled(0) && (action == ActionSteerLeft || action == ActionSteerRight)) axis = 0;
        if (enabled(1) && !combined && action == ActionThrottle) axis = 1;
        if (enabled(2) && !combined && action == ActionBrake) axis = 2;
        if (enabled(3) && combined && (action == ActionThrottle || action == ActionBrake)) axis = 3;

        if (axis >= 0 && match())
        {
            if (auto name = ObjectName(Channel::At<DWORD>(channel, Channel::AxisOffset + axis * 4)))
                return name;
        }

        for (int i = 0; i < 16; ++i)
        {
            if (Channel::At<int32_t>(channel, Channel::ButtonGameAction + i * 4) == action && match())
            {
                if (auto name = ObjectName(OfsButton0 + i))
                    return name;
            }
        }

        static constexpr const char* DpadNames[] = { "D-pad Down", "D-pad Up", "D-pad Left", "D-pad Right" };
        for (int pov = 0; pov < 4; ++pov)
        {
            for (int direction = 0; direction < 4; ++direction)
            {
                if (Channel::At<int32_t>(channel, Channel::PovGameAction + (pov * 4 + direction) * 4) == action && match())
                    return DpadNames[direction];
            }
        }

        return "";
    }
}

// Default bindings for Xidi pads, used when there's no saved configuration and by "Reset Keys To Default"
namespace
{
    SafetyHookInline shSetDefaultJoystickBindings = {};
    char __fastcall SetDefaultJoystickBindings(uintptr_t channel, void* edx)
    {
        char result = shSetDefaultJoystickBindings.unsafe_fastcall<char>(channel, edx);

        auto index = Channel::At<int32_t>(channel, Channel::Index);
        if (!IsXidiDevice(GetJoystick(index)))
            return result;

        // Left stick steers, RT accelerates, LT brakes and reverses. Triggers rest at the axis minimum.
        Channel::At<DWORD>(channel, Channel::AxisOffset + 0) = OfsLeftX;
        Channel::At<DWORD>(channel, Channel::AxisOffset + 4) = OfsRT;
        Channel::At<DWORD>(channel, Channel::AxisOffset + 8) = OfsLT;
        Channel::At<DWORD>(channel, Channel::AxisOffset + 12) = OfsLeftY;
        for (int i = 0; i < 4; ++i)
            Channel::At<int32_t>(channel, Channel::AxisSign + i * 4) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 0) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 1) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 2) = 1;
        Channel::At<uint8_t>(channel, Channel::AxisEnabled + 3) = 0;
        Channel::At<uint8_t>(channel, Channel::CombinedPedals) = 0;

        constexpr uint32_t GameActions[PadButtonCount] =
        {
            ActionHandbrake,    // A
            ActionNitrous,      // B
            ActionEngage,       // X: enter events, shops and garages
            ActionCamera,       // Y
            ActionShiftDown,    // LB
            ActionShiftUp,      // RB
            ActionWorldMap,     // Back
            ActionPause,        // Start
            ActionLookBack,     // LS
            ActionRespot,       // RS
        };

        constexpr uint32_t MenuActions[PadButtonCount] =
        {
            ActionMenuSelect,       // A
            ActionMenuBack,         // B
            ActionMenuCustomise,    // X
            ActionMenuPreset,       // Y
            ActionMenuL1,           // LB
            ActionMenuR1,           // RB
            ActionMenuHelp,         // Back
            ActionMenuButton2,      // Start
            ActionNone,             // LS
            ActionNone,             // RS
        };

        for (int i = 0; i < 16; ++i)
        {
            Channel::At<uint32_t>(channel, Channel::ButtonGameAction + i * 4) = i < PadButtonCount ? GameActions[i] : ActionNone;
            Channel::At<uint32_t>(channel, Channel::ButtonMenuAction + i * 4) = i < PadButtonCount ? MenuActions[i] : ActionNone;
            Channel::At<uint32_t>(channel, Channel::ButtonGameActionSecondary + i * 4) = ActionNone;
            Channel::At<uint32_t>(channel, Channel::ButtonMenuActionSecondary + i * 4) = ActionNone;
        }

        // D-pad: up for messages, down for the HUD, left/right steer. Menu navigation is set up by the game.
        for (int pov = 0; pov < 4; ++pov)
        {
            uint32_t* directions = &Channel::At<uint32_t>(channel, Channel::PovGameAction + pov * 16);
            directions[0] = ActionHUD;
            directions[1] = ActionSMS;
            directions[2] = ActionSteerLeft;
            directions[3] = ActionSteerRight;

            uint32_t* secondary = &Channel::At<uint32_t>(channel, Channel::PovGameActionSecondary + pov * 16);
            std::fill(secondary, secondary + 4, ActionNone);
        }

        return result;
    }
}

// Stick dead zones for Xidi pads, set when the game enumerates the device's axes
namespace
{
    SafetyHookInline shEnumJoystickObject = {};
    BOOL __stdcall EnumJoystickObject(LPCDIDEVICEOBJECTINSTANCEA object, LPVOID ref)
    {
        BOOL result = shEnumJoystickObject.unsafe_stdcall<BOOL>(object, ref);

        auto device = GetJoystick(int(reinterpret_cast<intptr_t>(ref)));
        if (object && (object->dwType & DIDFT_AXIS) && IsXidiDevice(device))
        {
            auto ofs = object->dwOfs;
            if (ofs == OfsLeftX || ofs == OfsLeftY || ofs == OfsRightX || ofs == OfsRightY)
                SetStickDeadzone(device, object->dwType, DIPH_BYID, ofs);
        }

        return result;
    }
}

// Per-frame processing of a joystick channel, after the game built its action mask and analog bytes
namespace
{
    bool bSeenAtRest[MaxJoysticks] = {};
    IDirectInputDevice8A* LastDevice[MaxJoysticks] = {};

    uint8_t AxisToByte(LONG value)
    {
        // -10000..10000 -> 0..254, matching the game's own conversion
        return uint8_t(std::clamp(127 + int(value * 0.0127), 0, 254));
    }

    void ProcessJoystickChannel(uintptr_t channel)
    {
        if (Channel::At<int32_t>(channel, Channel::DeviceType) == Channel::KeyboardType || Channel::At<int32_t>(channel, Channel::ConfigMode) != 0)
            return;

        int index = Channel::At<int32_t>(channel, Channel::Index);
        auto device = GetJoystick(index);
        auto state = GetJoyState(index);
        if (!device || !state)
            return;

        if (LastDevice[index] != device)
        {
            LastDevice[index] = device;
            bSeenAtRest[index] = false;
            PreviousPad[index].valid = false;
        }

        if (!IsXidiDevice(device))
        {
            // Some devices report a stick or pedal that never returns to the centre. In the front end that
            // reads as the left stick being held up and the menus keep scrolling. Ignore the analog
            // navigation of such a device until it has been seen at rest.
            if (IsFrontEnd() && !bSeenAtRest[index])
            {
                auto& steer = Channel::At<uint8_t>(channel, Channel::SteerByte);
                auto& pedals = Channel::At<uint8_t>(channel, Channel::PedalsByte);
                if (std::abs(int(steer) - 0x80) < 16 && std::abs(int(pedals) - 0x80) < 16)
                    bSeenAtRest[index] = true;
                else
                {
                    steer = 0x80;
                    pedals = 0x80;
                }
            }
            return;
        }

        TrackPadActivity(index, *state);

        // The right stick of the pad in use drives the camera
        Input::Look.stickX = std::clamp(state->lRx / 10000.0f, -1.0f, 1.0f);
        Input::Look.stickY = std::clamp(state->lRy / 10000.0f, -1.0f, 1.0f);

        if (IsFrontEnd())
        {
            // Left stick up/down navigates menus the same way combined pedals do
            Channel::At<uint8_t>(channel, Channel::PedalsByte) = AxisToByte(state->lY);

            // Both stick clicks quit, like the Q key
            bool ls = (state->rgbButtons[PadLS] & 0x80) != 0;
            bool rs = (state->rgbButtons[PadRS] & 0x80) != 0;
            if (ls && rs)
                SetActionPressed(channel, ActionMenuQuit, true);
        }
    }
}

class InputModule
{
public:
    InputModule()
    {
        WFP::onInitEvent() += []()
        {
            pGamepadIcons = &FixSettings().Int("MAIN.GamepadIcons");
            FixSettings().OnChange("MAIN.GamepadIcons", [] { Input::RefreshIconStyle(); });
            FixSettings().OnChange("GAMEPAD.LeftStickDeadzone", [] { Input::ApplyStickDeadzone(); });
            FixSettings().OnChange("GAMEPAD.RightStickDeadzone", [] { Input::ApplyStickDeadzone(); });
            CurrentIconStyle = DetectIconStyle();

            auto pattern = hook::pattern("83 EC 18 56 8B 74 24 24 8B 04 B5 ? ? ? ? 85 C0 57 74");
            auto joyState = hook::pattern("A3 ? ? ? ? B0 80 89 0D");
            if (pattern.empty() || joyState.empty())
                return;

            Joysticks = *pattern.get_first<IDirectInputDevice8A**>(11);
            JoyStateCopy = *joyState.get_first<uint8_t**>(1);
            shEnumJoystickObject = safetyhook::create_inline(pattern.get_first(), EnumJoystickObject);

            pattern = hook::pattern("81 EC AC 03 00 00 55 8B E9 8B 85 30 02 00 00");
            if (!pattern.empty())
                shSetDefaultJoystickBindings = safetyhook::create_inline(pattern.get_first(), SetDefaultJoystickBindings);

            pattern = hook::pattern("83 EC 0C 53 55 8B 6C 24 18 8B D9 57 33 C0 B9 4F 00 00 00 BF");
            if (!pattern.empty())
                shGetJoystickBindingName = safetyhook::create_inline(pattern.get_first(), GetJoystickBindingName);

            pattern = hook::pattern("E8 ? ? ? ? 8B 46 08 8B 0D");
            if (!pattern.empty())
            {
                static auto JoystickChannelHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    ProcessJoystickChannel(regs.esi);
                });
            }

            // While the controls menu is open the game enumerates all game controllers every 32 frames to
            // find new ones. That's slow with many HID devices and makes the menu stutter. Only do it after
            // Windows reports a device change.
            pattern = hook::pattern("8B C8 83 E1 1F 40 84 C9");
            if (!pattern.empty())
            {
                static auto RescanDevicesHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    regs.ecx = bDevicesChanged.exchange(false) ? 0 : 1;
                });
            }

            onWindowMessage() += [](HWND, UINT msg, WPARAM wParam, LPARAM)
            {
                if (msg == WM_DEVICECHANGE && (wParam == DBT_DEVNODES_CHANGED || wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
                {
                    bDevicesChanged = true;
                    bIconStyleOutdated = true;
                }
            };

            // FEString rendering, the call to FEngFont::RenderString: text at esp+4, the FEString in esi
            pattern = hook::pattern("8B 4D 10 51 8B 4C 24 ? 57 8D 54 24 ? 52 56 50 8D 44 24 ? 50 E8");
            auto localized = hook::pattern("68 00 08 00 00 52 E8 ? ? ? ? 83 C4 0C 84 C0");
            if (!pattern.empty() && !localized.empty())
            {
                GetLocalizedWideString = reinterpret_cast<decltype(GetLocalizedWideString)>(injector::GetBranchDestination(localized.get_first(6)).as_int());
                static auto RenderStringHook = safetyhook::create_mid(pattern.get_first(21), [](SafetyHookContext& regs)
                {
                    auto& text = *reinterpret_cast<const wchar_t**>(regs.esp + 4);
                    if (auto prompt = PromptText(regs.esi, text))
                        text = prompt;
                });

                // FE object rendering: lay strings out again while RefreshStringsUntil hasn't passed
                pattern = hook::pattern("8B 44 24 04 56 57 8B F9 83 78 18 07 75 07 81 48 1C 00 00 00 02");
                if (!pattern.empty())
                {
                    static auto RenderObjectHook = safetyhook::create_mid(pattern.get_first(21), [](SafetyHookContext& regs)
                    {
                        constexpr int32_t TypeString = 2;
                        constexpr uint32_t FlagDirty = 0x2000000;
                        auto object = regs.eax;
                        if (*reinterpret_cast<int32_t*>(object + 0x18) == TypeString && SteadyClock::now() < RefreshStringsUntil)
                            *reinterpret_cast<uint32_t*>(object + 0x1C) |= FlagDirty;
                    });
                }

                WFP::onEndScene() += []()
                {
                    UpdatePromptMode();
                };
            }

            // Button images in front end text
            {
                auto addPoly = hook::pattern("52 50 E8 ? ? ? ? 83 C4 1C C2 1C 00");
                auto textureInfo = hook::pattern("8B 4F 24 6A 00 6A 01 51 E8 ? ? ? ? 8B 5D 0C");
                auto loadChunks = hook::pattern("8B 4C 24 28 51 53 56 E8 ? ? ? ? 83 C4 0C 5F 5E 5D 32");
                auto render = hook::pattern("8B 4C 24 14 51 55 E8 ? ? ? ? 83 C4 08 8B E8 C6 44 24 54 01");
                auto width = hook::pattern("8B 4C 24 2C 51 57 E8 ? ? ? ? 83 C4 08 8B F8 B3 01");
                if (!addPoly.empty() && !textureInfo.empty() && !loadChunks.empty() && !render.empty() && !width.empty())
                {
                    ButtonIcons::AddPoly = injector::GetBranchDestination(addPoly.get_first(2)).get();
                    ButtonIcons::GetTextureInfo = reinterpret_cast<decltype(ButtonIcons::GetTextureInfo)>(injector::GetBranchDestination(textureInfo.get_first(8)).as_int());
                    ButtonIcons::LoadChunks = reinterpret_cast<decltype(ButtonIcons::LoadChunks)>(injector::GetBranchDestination(loadChunks.get_first(7)).as_int());

                    // FEngFont::RenderString at a "$...$" token: ebp = text, ebx = font
                    static auto RenderTokenHook = safetyhook::create_mid(render.get_first(), [](SafetyHookContext& regs)
                    {
                        auto texture = ButtonIcons::TokenTexture(reinterpret_cast<const wchar_t*>(regs.ebp));
                        if (!texture)
                            return;

                        auto& x = *reinterpret_cast<float*>(regs.esp + 0x10);
                        float y = *reinterpret_cast<float*>(regs.esp + 0x18);
                        uint32_t flags = *reinterpret_cast<uint32_t*>(regs.esp + 0x14);
                        uint32_t textColor = *reinterpret_cast<uint32_t*>(regs.esp + 0x38);
                        uintptr_t renderObject = *reinterpret_cast<uintptr_t*>(regs.esp + 0x5C);
                        uintptr_t renderInfo = *reinterpret_cast<uintptr_t*>(regs.esp + 0x60);

                        // Same depth as the glyphs
                        bool flag = ((flags >> 8) & 1) != 0;
                        uintptr_t z = 0;
                        if (flag)
                            z = *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(renderObject + 8) + 0x2C) + 0x24);

                        // Colours are halved, 0x80 is full brightness; keep the text's alpha
                        uint32_t color[4];
                        std::fill(std::begin(color), std::end(color), (textColor & 0xFF000000) | 0x808080);

                        float size = ButtonIcons::IconSize(regs.ebx);
                        using AddPolyFn = uintptr_t(__thiscall*)(uintptr_t, float, float, float, float, uintptr_t, float, float, float, float, uint32_t*, uintptr_t, bool);
                        auto poly = reinterpret_cast<AddPolyFn>(ButtonIcons::AddPoly)(renderObject, x, y, x + size, y + size, z, 0.0f, 0.0f, 1.0f, 1.0f, color, renderInfo, flag);
                        if (poly)
                            *reinterpret_cast<void**>(poly + ButtonIcons::PolyTexture) = texture;
                        x += size;
                    });

                    // FEngFont::GetLineWidth at a token: edi = text, width at esp+0x28, font at esp+0x10
                    static auto TokenWidthHook = safetyhook::create_mid(width.get_first(), [](SafetyHookContext& regs)
                    {
                        if (ButtonIcons::TokenTexture(reinterpret_cast<const wchar_t*>(regs.edi)))
                            *reinterpret_cast<float*>(regs.esp + 0x28) += ButtonIcons::IconSize(*reinterpret_cast<uintptr_t*>(regs.esp + 0x10));
                    });
                }
            }

            // Keyboard and mouse activity switches the prompts back
            pattern = hook::pattern("E8 ? ? ? ? 8B 54 24 ? 8A 4E");
            static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (hWnd != GetForegroundWindow())
                    return;

                struct MouseState { int dx, dy, wheel; };
                auto state = reinterpret_cast<MouseState*>(regs.esp + 0x8);
                Input::Look.mouseX += state->dx;
                Input::Look.mouseY += state->dy;
                if (state->dx || state->dy || state->wheel)
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            pattern = hook::pattern("85 C0 7D ? 3D ? ? ? ? 74 ? 3D ? ? ? ? 0F 85 ? ? ? ? A1 ? ? ? ? ? ? 50 FF 52 ? 5F 5E 5D 5B 8B E5 5D C3 A0");
            static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                // Every device's channel comes through here: only the keyboard's actions are keyboard input
                if (Channel::At<int32_t>(regs.esi, Channel::DeviceType) != Channel::KeyboardType)
                    return;
                if (*reinterpret_cast<int32_t*>(regs.esi + Channel::ActionMaskLow) != -1 || *reinterpret_cast<int32_t*>(regs.esi + Channel::ActionMaskHigh) != -1)
                    LastKeyboardMouseInput = SteadyClock::now();
            });
        };
    }
} InputModule;
