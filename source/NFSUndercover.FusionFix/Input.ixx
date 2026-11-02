module;

#include <stdafx.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>
#include "Shared/PadInput.hpp"
#include "FixSettings.hpp"
#include "RawMouse.hpp"
#include "Shared/NFS/StickDeadzone.hpp"

export module Input;

import ComVars;
import TexturePack;

import PostFX;

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
        pXInputGetState = PadInput::XInput().getState;
    }

    IconStyle DetectIconStyle()
    {
        return std::clamp(*pGamepadIcons, 0, 2) == 2 ? IconStyle::PlayStation : IconStyle::Xbox;
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
        bool rawMouse = false;  // the mouse movement is raw input
    };

    LookInput Look;

    // Called once per camera update; mouse deltas accumulate between calls
    LookInput ConsumeLook()
    {
        LookInput result = Look;
        Look.mouseX = 0;
        Look.mouseY = 0;
        result.rawMouse = RawMouse::Consume(result.mouseX, result.mouseY);
        if (result.rawMouse && (result.mouseX || result.mouseY))
            LastKeyboardMouseInput = SteadyClock::now();
        if (DeveloperOverlay::BlocksInput())
            return {};
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
        if (!pXInputGetState || hWnd != GetForegroundWindow() || DeveloperOverlay::BlocksInput())
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
        static SteadyClock::time_point retryAfter{};
        if (bLoaded || !LoadChunks || SteadyClock::now() < retryAfter)
            return;
        bLoaded = true;

        auto textures = TexturePack::LoadResourceTextures();
        if (textures.empty())
            return;

        // The texture pack keeps pointing into this memory, so it's never freed
        auto pack = TexturePack::Build(textures, "WFP_NFSUC_BUTTONS");
        auto buffer = static_cast<uint8_t*>(_aligned_malloc(pack.size(), 0x80));
        if (!buffer)
        {
            bLoaded = false;
            retryAfter = SteadyClock::now() + std::chrono::seconds(5);
            return;
        }
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
        bool valid = SUCCEEDED(device->GetDeviceInfo(&info));
        bool xidi = valid && memcmp(info.guidInstance.Data4, "XIDIVC", 6) == 0;
        kind.gamepad = valid && (GET_DIDEVICE_TYPE(info.dwDevType) == DI8DEVTYPE_GAMEPAD || xidi);
        kind.xinput = xidi;

        // XInput controllers have "IG_" in their device path
        DIPROPGUIDANDPATH path = {};
        path.diph.dwSize = sizeof(path);
        path.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        path.diph.dwHow = DIPH_DEVICE;
        if (SUCCEEDED(device->GetProperty(DIPROP_GUIDANDPATH, &path.diph)))
        {
            std::wstring devicePath = path.wszPath;
            std::transform(devicePath.begin(), devicePath.end(), devicePath.begin(), ::towupper);
            kind.xinput = kind.xinput || devicePath.find(L"IG_") != std::wstring::npos;
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
        auto deviceName = std::string_view(name);
        if (result == DeviceNameHash(name) && (deviceName.starts_with("Controller (") ||
            deviceName.starts_with("Xidi Virtual Controller") || deviceName.starts_with("Xidi-")))
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
            // The controller name lookup can receive an action-source index beyond the eight controller slots.
            auto controllerName = hook::pattern("83 E8 02 75 ? 8B 44 24 10 8B 04 85 ? ? ? ? EB ? 5F B8 78 56 34 12");
            static auto emptyControllerName = reinterpret_cast<uintptr_t>(controllerName.get_first<uint8_t>(5) + *controllerName.get_first<int8_t>(4));
            static auto controllerNameHook = safetyhook::create_mid(controllerName.get_first(9), [](SafetyHookContext& regs)
            {
                if (regs.eax >= 8)
                    regs.eip = emptyControllerName;
            });

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
            shGetTextureInfo = safetyhook::create_inline(pattern.get_first(), GetTextureInfo);

            pattern = hook::pattern("8B 44 24 0C 56 8B 74 24 08 57 8B 7C 24 10 50 57 56 E8");
            LoadChunks = reinterpret_cast<decltype(LoadChunks)>(pattern.get_first());

            // Mouse: deltas at ebp-0x14
            pattern = hook::pattern("8B 45 ? 3B C7 75 ? 3B D7");
            static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (hWnd != GetForegroundWindow())
                    return;

                struct MouseState { int dx, dy, wheel; uint8_t button; };
                auto state = reinterpret_cast<MouseState*>(regs.ebp - 0x14);
                DeveloperOverlay::MouseButton((state->button & 0x80) != 0);
                if (DeveloperOverlay::BlocksInput())
                {
                    state->dx = state->dy = state->wheel = 0;
                    state->button = 0;
                    regs.edx = regs.ecx = 0;    // dy and the wheel, already loaded
                    RawMouse::Clear();
                    return;
                }
                Input::Look.mouseX += state->dx;
                Input::Look.mouseY += state->dy;
                if (state->dx || state->dy || state->wheel)
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            // Joystick state, read by the game's input library (RealCore): esi = device, its IDirectInputDevice8 at
            // +0x468, the DIJOYSTATE2 from GetDeviceState at ebp-0x13C. The game compares it with the last state and
            // sends its events after this point. Gamepads get the left stick's dead zone on X/Y; the right stick's goes
            // on Rx/Ry only for XInput controllers, where those are the right stick (on other pads, such as a
            // DualShock 4, Rx/Ry are the triggers and Z/Rz the right stick). Wheels are left alone.
            pattern = hook::pattern("8B 85 ? ? ? ? 3B 86");
            static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto kind = GetPadKind(*reinterpret_cast<IDirectInputDevice8A**>(regs.esi + 0x468));
                if (!kind.gamepad)
                    return;

                auto state = reinterpret_cast<DIJOYSTATE2*>(regs.ebp - 0x13C);

                // The overlay's pad: A B X Y LB RB Back Start LS RS and the d-pad in its own bits; while it's open
                // the game sees a pad at rest
                uint16_t overlayButtons = 0;
                constexpr uint16_t overlayMapping[] = { 0x4000, 0x2000, 0x8000, 0x1000, 0x400, 0x800, 0x1, 0x8, 0x2, 0x4 };
                for (size_t button = 0; button < std::size(overlayMapping); ++button)
                {
                    if (state->rgbButtons[button] & 0x80)
                        overlayButtons |= overlayMapping[button];
                }
                auto pov = state->rgdwPOV[0];
                if (LOWORD(pov) != 0xFFFF)
                {
                    if (pov >= 31500 || pov <= 4500) overlayButtons |= 0x10;
                    if (pov >= 4500 && pov <= 13500) overlayButtons |= 0x20;
                    if (pov >= 13500 && pov <= 22500) overlayButtons |= 0x40;
                    if (pov >= 22500 && pov <= 31500) overlayButtons |= 0x80;
                }
                DeveloperOverlay::Gamepad(overlayButtons, int8_t(std::clamp((state->lX - 32767) / 258, -127L, 127L)),
                    int8_t(std::clamp((state->lY - 32767) / 258, -127L, 127L)));
                if (DeveloperOverlay::BlocksInput())
                {
                    state->lX = state->lY = state->lZ = state->lRx = state->lRy = state->lRz = 32767;
                    std::fill(std::begin(state->rgbButtons), std::end(state->rgbButtons), uint8_t(0));
                    state->rgdwPOV[0] = DWORD(-1);
                    return;
                }

                static const float& leftDeadzone = FixSettings().Float("GAMEPAD.LeftStickDeadzone");
                static const float& rightDeadzone = FixSettings().Float("GAMEPAD.RightStickDeadzone");
                WFPStickDeadzone::Apply(state->lX, state->lY, leftDeadzone);
                if (kind.xinput)
                    WFPStickDeadzone::Apply(state->lRx, state->lRy, rightDeadzone);
            });

            // The controller type, for the default controls of newer XInput controllers
            pattern = hook::pattern("E8 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87");
            auto getDeviceType = injector::GetBranchDestination(pattern.get_first()).as_int();
            // mov eax, [esp+arg_4]; mov eax, ds:devices[eax*4]
            auto table = hook::range_pattern(getDeviceType + 0x42, getDeviceType + 0x4D,
                "8B 44 24 ? 8B 04 85 ? ? ? ?").get_first<uint8_t>();
            JoystickDevices = *reinterpret_cast<uintptr_t**>(table + 7);
            shGetDeviceType = safetyhook::create_inline(getDeviceType, GetDeviceType);

            // Keyboard: key states at ebp-0x130
            pattern = hook::pattern("8B 83 64 04 00 00 8B 80 B0 00 00 00");
            static auto KeyboardStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto keys = reinterpret_cast<uint8_t*>(regs.ebp - 0x130);
                if (DeveloperOverlay::BlocksInput())
                {
                    std::fill_n(keys, 256, uint8_t(0));
                    return;
                }
                if (std::any_of(keys, keys + 256, [](uint8_t key) { return (key & 0x80) != 0; }))
                    LastKeyboardMouseInput = SteadyClock::now();
            });

            // Raw mouse movement for the free look camera
            RawMouse::Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
            FixSettings().OnChange("MAIN.RawInputMouse", []()
            {
                RawMouse::Enabled.store(FixSettings().Bool("MAIN.RawInputMouse"));
                RawMouse::Clear();
            });
            WFP::onGameProcessEvent() += []() { RawMouse::Update(hWnd.get()); };
            WFP::onShutdownEvent() += []() { RawMouse::Release(); };
            onWindowMessage() += RawMouse::Message;

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
