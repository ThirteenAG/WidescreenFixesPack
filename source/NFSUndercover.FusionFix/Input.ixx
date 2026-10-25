module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>
#include <xinput.h>
#include "TexturePack.hpp"
#include "FixSettings.hpp"
#include "../Shared/NFS/StickDeadzone.hpp"

export module Input;

import ComVars;

// The game reads controllers through DirectInput (EA's RealCore input library) and shows Xbox buttons in the front
// end and in the HUD. This module replaces those small button images with sharp ones, Xbox or PlayStation style,
// shows the controller's back button and d-pad instead of keyboard keys while a controller is used, applies the
// stick dead zones, gives newer XInput controllers the game's Xbox 360 controls, and reads the right stick (through
// XInput) and the mouse for the camera.

namespace
{
    enum class IconStyle { Xbox, PlayStation };
    const int* pGamepadIcons = nullptr;   // 0 = auto, 1 = Xbox, 2 = PlayStation; also changed in the options menu
    std::atomic<IconStyle> CurrentIconStyle = IconStyle::Xbox;
    std::atomic<bool> bIconStyleOutdated = false;

    using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
    XInputGetStateFn pXInputGetState = nullptr;

    void LoadXInput()
    {
        for (auto name : { "xinput1_3.dll", "xinput1_4.dll", "xinput9_1_0.dll" })
        {
            if (auto module = LoadLibraryA(name))
            {
                pXInputGetState = reinterpret_cast<XInputGetStateFn>(GetProcAddress(module, "XInputGetState"));
                if (pXInputGetState)
                    return;
            }
        }
    }

    IconStyle DetectIconStyle()
    {
        if (*pGamepadIcons == 1)
            return IconStyle::Xbox;
        if (*pGamepadIcons == 2)
            return IconStyle::PlayStation;

        // Sony controllers are usually turned into XInput controllers by Steam Input or DS4Windows, so
        // they're looked for among the connected HID devices
        UINT count = 0;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count > 0)
        {
            std::vector<RAWINPUTDEVICELIST> devices(count);
            if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) != UINT(-1))
            {
                for (const auto& device : devices)
                {
                    if (device.dwType != RIM_TYPEHID)
                        continue;

                    RID_DEVICE_INFO info = { sizeof(RID_DEVICE_INFO) };
                    UINT size = sizeof(info);
                    if (GetRawInputDeviceInfoA(device.hDevice, RIDI_DEVICEINFO, &info, &size) != UINT(-1) &&
                        info.hid.dwVendorId == 0x054C && info.hid.usUsagePage == 0x01 && (info.hid.usUsage == 0x04 || info.hid.usUsage == 0x05))
                        return IconStyle::PlayStation;
                }
            }
        }

        return IconStyle::Xbox;
    }

    using SteadyClock = std::chrono::steady_clock;
    SteadyClock::time_point LastPadInput = {};
    SteadyClock::time_point LastKeyboardMouseInput = {};

    bool LastInputWasPad()
    {
        return LastPadInput > LastKeyboardMouseInput;
    }
}

export namespace Input
{
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

// Controller polling: the right stick for the camera, and whether the controller was used last
namespace
{
    void PollController()
    {
        if (!pXInputGetState || hWnd != GetForegroundWindow())
        {
            Input::Look.stickX = Input::Look.stickY = 0.0f;
            return;
        }

        for (DWORD i = 0; i < 4; ++i)
        {
            XINPUT_STATE state = {};
            if (pXInputGetState(i, &state) != ERROR_SUCCESS)
                continue;

            const auto& pad = state.Gamepad;
            auto stick = [](SHORT value, SHORT deadzone) -> float
            {
                if (std::abs(int(value)) <= deadzone)
                    return 0.0f;
                float sign = value < 0 ? -1.0f : 1.0f;
                return sign * std::clamp((std::abs(float(value)) - deadzone) / (32767.0f - deadzone), 0.0f, 1.0f);
            };

            static const float& rightDeadzone = FixSettings().Float("GAMEPAD.RightStickDeadzone");
            auto deadzone = SHORT(std::clamp(rightDeadzone, 0.0f, 90.0f) / 100.0f * 32767.0f);
            Input::Look.stickX = stick(pad.sThumbRX, deadzone);
            Input::Look.stickY = -stick(pad.sThumbRY, deadzone);

            bool active = pad.wButtons != 0 || pad.bLeftTrigger > 64 || pad.bRightTrigger > 64 ||
                std::abs(int(pad.sThumbLX)) > 16000 || std::abs(int(pad.sThumbLY)) > 16000 ||
                std::abs(int(pad.sThumbRX)) > 16000 || std::abs(int(pad.sThumbRY)) > 16000;
            if (active)
                LastPadInput = SteadyClock::now();
            return;
        }

        Input::Look.stickX = Input::Look.stickY = 0.0f;
    }
}

// Button images. The game asks for its textures by name hash; the controller buttons are swapped for the
// embedded images here.
namespace
{
    struct ButtonImage
    {
        const char* game;       // texture of the game
        const char* image;      // XBOX_ and PS_ textures of this fix
        bool padOnly;           // keyboard images, only replaced while a controller is used
    };

    constexpr ButtonImage ButtonImages[] =
    {
        { "A",                  "A",            false },
        { "B",                  "B",            false },
        { "B_FRENCH",           "B",            false },
        { "X",                  "X",            false },
        { "Y",                  "Y",            false },
        { "LB",                 "LB",           false },
        { "RB",                 "RB",           false },
        { "LT",                 "LT",           false },
        { "RT",                 "RT",           false },
        { "L3",                 "LS",           false },
        { "BUTTON_C",           "LS",           false },
        { "LEFT_ANALOG",        "LEFT_ANALOG",  false },
        { "RIGHT_ANALOG",       "RIGHT_ANALOG", false },
        { "HUD_RACE_NOW_XENON", "DPAD_DOWN",    false },
        { "HUD_RACE_NOW_PS3",   "DPAD_DOWN",    false },
        { "ESC",                "B",            true  },
        { "ESC_FRENCH",         "B",            true  },
        { "HUD_RACE_NOW_PC",    "DPAD_DOWN",    true  },
    };

    struct ButtonHashes
    {
        uint32_t xbox, playStation;
        bool padOnly;
    };

    std::unordered_map<uint32_t, ButtonHashes> ButtonImageHashes;
    std::atomic<bool> bTexturesLoaded = false;

    SafetyHookInline shGetTextureInfo = {};
    void* __cdecl GetTextureInfo(uint32_t hash, int returnDefault, int includeUnloaded)
    {
        if (bTexturesLoaded)
        {
            if (auto it = ButtonImageHashes.find(hash); it != ButtonImageHashes.end() && (!it->second.padOnly || LastInputWasPad()))
                hash = CurrentIconStyle == IconStyle::PlayStation ? it->second.playStation : it->second.xbox;
        }
        return shGetTextureInfo.unsafe_ccall<void*>(hash, returnDefault, includeUnloaded);
    }

    int (__cdecl* LoadChunks)(void* data, int size, int flags) = nullptr;

    void LoadButtonTextures()
    {
        static bool bLoaded = false;
        if (bLoaded || !LoadChunks)
            return;
        bLoaded = true;

        auto textures = TexturePack::LoadResourceTextures();
        if (textures.empty())
            return;

        // The texture pack keeps pointing into this memory, so it's never freed
        auto pack = TexturePack::Build(textures, "WFP_NFSUC_BUTTONS");
        auto buffer = static_cast<uint8_t*>(_aligned_malloc(pack.size(), 0x80));
        if (!buffer)
            return;
        memcpy(buffer, pack.data(), pack.size());
        LoadChunks(buffer, int(pack.size()), 0);
        bTexturesLoaded = true;
    }
}

// DirectInput devices that are gamepads, as opposed to wheels and other controllers. The game reads all controllers,
// Xbox ones too, through DirectInput (EA's RealCore input library); XInput is only used by the Logitech wheel library.
namespace
{
    struct PadKind
    {
        bool gamepad = false;   // DI8DEVTYPE_GAMEPAD
        bool xinput = false;    // an XInput controller: right stick on Rx/Ry, triggers together on Z
    };

    PadKind GetPadKind(IDirectInputDevice8A* device)
    {
        static std::mutex mutex;
        static std::map<IDirectInputDevice8A*, PadKind> cache;
        if (!device)
            return {};

        std::lock_guard lock(mutex);
        if (auto it = cache.find(device); it != cache.end())
            return it->second;

        PadKind kind;
        DIDEVICEINSTANCEA info = {};
        info.dwSize = sizeof(info);
        kind.gamepad = SUCCEEDED(device->GetDeviceInfo(&info)) && GET_DIDEVICE_TYPE(info.dwDevType) == DI8DEVTYPE_GAMEPAD;

        // XInput controllers have "IG_" in their device path
        DIPROPGUIDANDPATH path = {};
        path.diph.dwSize = sizeof(path);
        path.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        path.diph.dwHow = DIPH_DEVICE;
        if (SUCCEEDED(device->GetProperty(DIPROP_GUIDANDPATH, &path.diph)))
        {
            std::wstring devicePath = path.wszPath;
            std::transform(devicePath.begin(), devicePath.end(), devicePath.begin(), ::towupper);
            kind.xinput = devicePath.find(L"IG_") != std::wstring::npos;
        }

        cache[device] = kind;
        return kind;
    }

    // The game's case-insensitive name hash of a controller (sub_4B3F30 steam)
    uint32_t DeviceNameHash(const char* name)
    {
        uint32_t result = uint32_t(-1);
        for (; *name; ++name)
        {
            uint8_t c = uint8_t(*name);
            if (c >= 'a' && c <= 'z')
                c &= 0x5F;
            result = c + 33 * result;
        }
        return result;
    }

    // The type of a controller (sub_6821F0 steam, stdcall(type, index)): Logitech wheels by the wheel library,
    // everything else by the hash of its DirectInput name, which picks its default controls and its saved ones. The
    // game knows Xbox 360 controllers by name; newer XInput controllers (Xbox One and Series, third-party pads), named
    // "Controller (...)" by their driver, got no default controls. They're taken for an Xbox 360 controller, which has
    // the same DirectInput layout.
    constexpr uint32_t Xbox360ControllerHash = 0x2A5E19E0;    // "Controller (XBOX 360 For Windows)"
    uintptr_t* JoystickDevices = nullptr;                      // the game's joystick devices, name at +0x114

    SafetyHookInline shGetDeviceType = {};
    uint32_t __stdcall GetDeviceType(int32_t type, int32_t index)
    {
        auto result = shGetDeviceType.unsafe_stdcall<uint32_t>(type, index);
        constexpr int32_t Joystick = 3;
        if (type != Joystick || index < 0 || !JoystickDevices)
            return result;

        auto device = JoystickDevices[index];
        if (!device || device == uintptr_t(-4))
            return result;

        auto name = reinterpret_cast<const char*>(device + 0x114);
        if (result == DeviceNameHash(name) && strncmp(name, "Controller (", 12) == 0)
            return Xbox360ControllerHash;
        return result;
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
            CurrentIconStyle = DetectIconStyle();
            LoadXInput();

            for (const auto& image : ButtonImages)
            {
                std::string name = image.image;
                ButtonImageHashes[bStringHash(image.game)] = { uint32_t(bStringHash(("XBOX_" + name).c_str())), uint32_t(bStringHash(("PS_" + name).c_str())), image.padOnly };
            }

            auto pattern = hook::pattern("53 56 57 8B 7C 24 10 85 FF 74 ? 8B C7 25 FF 00 00 00 8B 04 85");
            if (!pattern.empty())
                shGetTextureInfo = safetyhook::create_inline(pattern.get_first(), GetTextureInfo);

            pattern = hook::pattern("8B 44 24 0C 56 8B 74 24 08 57 8B 7C 24 10 50 57 56 E8");
            if (!pattern.empty() && shGetTextureInfo)
                LoadChunks = reinterpret_cast<decltype(LoadChunks)>(pattern.get_first());

            // Mouse: deltas at ebp-0x14
            pattern = hook::pattern("8B 45 ? 3B C7 75 ? 3B D7");
            if (!pattern.empty())
            {
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (hWnd != GetForegroundWindow())
                        return;

                    struct MouseState { int dx, dy, wheel; };
                    auto state = reinterpret_cast<MouseState*>(regs.ebp - 0x14);
                    Input::Look.mouseX += state->dx;
                    Input::Look.mouseY += state->dy;
                    if (state->dx || state->dy || state->wheel)
                        LastKeyboardMouseInput = SteadyClock::now();
                });
            }

            // Joystick state, read by the game's input library (RealCore): esi = device, its IDirectInputDevice8 at
            // +0x468, the DIJOYSTATE2 from GetDeviceState at ebp-0x13C. The game compares it with the last state and
            // sends its events after this point. Gamepads get the left stick's dead zone on X/Y; the right stick's goes
            // on Rx/Ry only for XInput controllers, where those are the right stick (on other pads, such as a
            // DualShock 4, Rx/Ry are the triggers and Z/Rz the right stick). Wheels are left alone.
            pattern = hook::pattern("8B 85 ? ? ? ? 3B 86");
            if (!pattern.empty())
            {
                static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto kind = GetPadKind(*reinterpret_cast<IDirectInputDevice8A**>(regs.esi + 0x468));
                    if (!kind.gamepad)
                        return;

                    auto state = reinterpret_cast<DIJOYSTATE2*>(regs.ebp - 0x13C);
                    static const float& leftDeadzone = FixSettings().Float("GAMEPAD.LeftStickDeadzone");
                    static const float& rightDeadzone = FixSettings().Float("GAMEPAD.RightStickDeadzone");
                    WFPStickDeadzone::Apply(state->lX, state->lY, leftDeadzone);
                    if (kind.xinput)
                        WFPStickDeadzone::Apply(state->lRx, state->lRy, rightDeadzone);
                });
            }

            // The controller type, for the default controls of newer XInput controllers
            pattern = hook::pattern("E8 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87");
            if (!pattern.empty())
            {
                auto getDeviceType = injector::GetBranchDestination(pattern.get_first()).as_int();
                // mov eax, [esp+arg_4]; mov eax, ds:devices[eax*4]
                auto table = reinterpret_cast<uint8_t*>(getDeviceType + 0x42);
                if (table[0] == 0x8B && table[1] == 0x44 && table[4] == 0x8B && table[5] == 0x04 && table[6] == 0x85)
                {
                    JoystickDevices = *reinterpret_cast<uintptr_t**>(table + 7);
                    shGetDeviceType = safetyhook::create_inline(getDeviceType, GetDeviceType);
                }
            }

            // Keyboard: key states at ebp-0x130
            pattern = hook::pattern("8B 83 64 04 00 00 8B 80 B0 00 00 00");
            if (!pattern.empty())
            {
                static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto keys = reinterpret_cast<const uint8_t*>(regs.ebp - 0x130);
                    if (std::any_of(keys, keys + 256, [](uint8_t key) { return (key & 0x80) != 0; }))
                        LastKeyboardMouseInput = SteadyClock::now();
                });
            }

            WFP::onGameProcessEvent() += []()
            {
                LoadButtonTextures();
                PollController();

                // Controllers can be plugged in at any time; checked every few seconds
                static auto lastCheck = SteadyClock::now();
                if (SteadyClock::now() - lastCheck > std::chrono::seconds(3))
                {
                    lastCheck = SteadyClock::now();
                    bIconStyleOutdated = true;
                }
                if (bIconStyleOutdated.exchange(false))
                    CurrentIconStyle = DetectIconStyle();
            };
        };
    }
} InputModule;
