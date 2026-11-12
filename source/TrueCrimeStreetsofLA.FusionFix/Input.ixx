module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <dinput.h>


export module Input;

import ComVars;
import DeveloperOverlay;
import FreeCamera;

// The game reads keyboard, mouse and pad through DirectInput device states. Its mouse state already carries the raw
// counts (no pointer acceleration, nothing lost between reads). While the overlay has input nothing reaches the game;
// while the mouse turns the camera, the game gets no mouse movement.
export namespace TrueCrimeInput
{
    inline SafetyHookInline CreateDeviceHook, DeviceStateHook, DeviceDataHook;
    inline HRESULT (WINAPI* NativeCreate)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN) = nullptr;
    inline void* Mouse = nullptr;

    inline HRESULT WINAPI DeviceState(void* device, DWORD size, void* data)
    {
        const auto result = DeviceStateHook.stdcall<HRESULT>(device, size, data);
        if (FAILED(result) || !data)
            return result;
        if (TrueCrimeOverlay::BlocksInput())
        {
            if (size == sizeof(DIMOUSESTATE))
            {
                TrueCrimeFreeCamera::CountsX.store(0, std::memory_order_relaxed);
                TrueCrimeFreeCamera::CountsY.store(0, std::memory_order_relaxed);
            }
            std::memset(data, 0, size);
            return result;
        }
        if (size == sizeof(DIMOUSESTATE))
        {
            Mouse = device;
            if (TrueCrimeFreeCamera::TakesMouse())
            {
                auto& state = *static_cast<DIMOUSESTATE*>(data);
                TrueCrimeFreeCamera::CountsX.fetch_add(state.lX, std::memory_order_relaxed);
                TrueCrimeFreeCamera::CountsY.fetch_add(state.lY, std::memory_order_relaxed);
                state.lX = state.lY = 0;
            }
            else
            {
                TrueCrimeFreeCamera::CountsX.store(0, std::memory_order_relaxed);
                TrueCrimeFreeCamera::CountsY.store(0, std::memory_order_relaxed);
            }
        }
        return result;
    }

    inline HRESULT WINAPI DeviceData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        const auto result = DeviceDataHook.stdcall<HRESULT>(device, size, data, count, flags);
        if (SUCCEEDED(result) && count && TrueCrimeOverlay::BlocksInput())
            *count = 0;
        return result;
    }

    inline HRESULT WINAPI CreateDevice(void* input, const GUID* guid, void** device, void* outer)
    {
        const auto result = CreateDeviceHook.stdcall<HRESULT>(input, guid, device, outer);
        if (SUCCEEDED(result) && device && *device && !DeviceStateHook)
        {
            auto table = *reinterpret_cast<void***>(*device);
            DeviceStateHook = safetyhook::create_inline(table[9], DeviceState);
            DeviceDataHook = safetyhook::create_inline(table[10], DeviceData);
        }
        return result;
    }

    inline HRESULT WINAPI Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* output, LPUNKNOWN outer)
    {
        const auto result = NativeCreate(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !CreateDeviceHook)
        {
            auto table = *reinterpret_cast<void***>(*output);
            CreateDeviceHook = safetyhook::create_inline(table[3], CreateDevice);
        }
        return result;
    }

    // The game's message pump removes messages until the queue is empty. Every legacy mouse move it removes makes
    // Windows process the input that queued up meanwhile, so with thousands of reports a second one pass can take tens
    // of milliseconds. Windows keeps only the newest mouse move, so the pass ends after one; DirectInput's movement is
    // separate and stays complete.
    inline BOOL WINAPI PeekMessageOnePass(LPMSG message, HWND window, UINT first, UINT last, UINT remove)
    {
        static thread_local bool moved = false;
        if (moved)
        {
            moved = false;
            return FALSE;
        }
        const BOOL result = PeekMessageA(message, window, first, last, remove);
        moved = result && (remove & PM_REMOVE) && message->message == WM_MOUSEMOVE;
        return result;
    }

    // The game keeps its Mouse Steering option in TrueCrime.INI; the fix keeps it in its own settings instead (off by
    // default, so the mouse turns the camera in cars), and the game's options change that setting.
    inline bool SteeringKey(LPCSTR section, LPCSTR key)
    {
        return section && key && !_stricmp(section, "Game") && !_stricmp(key, "MouseSteering");
    }

    inline UINT WINAPI ReadIniInt(LPCSTR section, LPCSTR key, INT fallback, LPCSTR file)
    {
        if (SteeringKey(section, key))
            return FixSettings().Bool("MAIN.MouseSteering") ? 1 : 0;
        return GetPrivateProfileIntA(section, key, fallback, file);
    }

    inline BOOL WINAPI WriteIniString(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR file)
    {
        if (SteeringKey(section, key))
        {
            const bool on = value && std::atoi(value) == 1;
            if (on != FixSettings().Bool("MAIN.MouseSteering"))
                FixSettings().Set("MAIN.MouseSteering", on ? "1" : "0");
            return TRUE;
        }
        return WritePrivateProfileStringA(section, key, value, file);
    }

    // Precision aim on foot (the slow-motion crosshair): the game turns the aim by the move axes, one degree per 1024
    // units an update, wrapped to +-180 units, and its mouse input sets them to counts / (gun sensitivity x 1000) x 1024
    // cut to +-1024, so fast moves turn less. The mouse leaves those axes alone there; its counts go straight into the
    // turn, at the free camera's scale, and the turn is no longer wrapped. Keyboard and pad keep the game's path; invert
    // and the pitch limits are the game's.
    inline std::atomic<int32_t> AimX{ 0 }, AimY{ 0 };
    inline int (__fastcall* AxisValue)(void*, void*, int, int) = nullptr;
    inline std::vector<SafetyHookMid> AimHooks;
    inline float AimRest[2]{};

    inline bool OnFootAim()
    {
        auto player = PlayerObject.get();
        const bool aiming = player && player[9] != 5 && *reinterpret_cast<int*>(player + 0xD68) == 3;
        if (!aiming)
        {
            AimX.store(0, std::memory_order_relaxed);
            AimY.store(0, std::memory_order_relaxed);
        }
        return aiming;
    }

    inline void TakeAim(int32_t x, int32_t y)
    {
        AimX.fetch_add(x, std::memory_order_relaxed);
        AimY.fetch_add(y, std::memory_order_relaxed);
    }

    template<bool Pitch> int __fastcall AimAxis(void* state, void* edx, int axis, int deadzone)
    {
        const int game = AxisValue(state, edx, axis, deadzone);
        const int32_t counts = (Pitch ? AimY : AimX).exchange(0, std::memory_order_relaxed);
        // 0.002 rad a count, in the game's 1/1024 degree units
        const float configuredSensitivity = FixSettings().Float("MAIN.MouseSensitivity");
        const float sensitivity = std::isfinite(configuredSensitivity) ? std::clamp(configuredSensitivity, 0.1f, 5.0f) : 1.0f;
        const float scale = 0.002f * 57.29578f * 1024.0f * sensitivity;
        float& rest = AimRest[Pitch];
        const float direction = Pitch && FixSettings().Bool("MAIN.InvertMouseY") ? -1.0f : 1.0f;
        const double movement = double(rest) + double(counts) * scale * direction;
        const int part = static_cast<int>(std::clamp(movement, double(INT32_MIN), double(INT32_MAX)));
        rest = movement > INT32_MIN && movement < INT32_MAX ? static_cast<float>(movement - part) : 0.0f;
        return static_cast<int>(std::clamp(int64_t(game) + part, int64_t(INT32_MIN), int64_t(INT32_MAX)));
    }

    inline float __cdecl Unwrapped(float value)
    {
        return value;
    }

    // Guns mode on foot (no precision aim): the game turns its camera by the camera axes, read as signed bytes, 1/2048
    // rad a unit an update, and its mouse input sets them to counts / (gun sensitivity x 1000) x 127 cut to +-127. The
    // mouse leaves those axes alone there; its counts turn the camera directly, at the free camera's scale. The game's
    // mouse direction options and invert apply.
    inline std::atomic<int32_t> GunX{ 0 }, GunY{ 0 };
    inline uint8_t* ControllerFlags = nullptr;     // bit 4: mouse x direction, bit 8: mouse y direction
    inline SafetyHookMid GunCameraHook;

    inline bool GunsOnFoot()
    {
        auto player = PlayerObject.get();
        return player && player[9] == 2 && *reinterpret_cast<int*>(player + 0xD68) != 3;
    }

    inline void TakeGun(int32_t x, int32_t y)
    {
        GunX.fetch_add(x, std::memory_order_relaxed);
        GunY.fetch_add(y, std::memory_order_relaxed);
    }

    inline void Init()
    {
        WFP::onGameProcessEvent() += []()
        {
            const bool blocked = TrueCrimeOverlay::BlocksInput() || GameMode.get() >= 2;
            if (blocked || !OnFootAim())
            {
                AimX.store(0, std::memory_order_relaxed);
                AimY.store(0, std::memory_order_relaxed);
            }
            if (blocked || !GunsOnFoot())
            {
                GunX.store(0, std::memory_order_relaxed);
                GunY.store(0, std::memory_order_relaxed);
            }
        };
        // Mouse part of the aim axes, where the game's input builds them
        auto gun = hook::pattern("8B 0D ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 10 0D ? ? ? ? 89 0D");
        if (!gun.empty()) // TrueCrime_.exe: x at esp+1C, y at esp+28, counts at esp+2C/30; both ways out of the y part
        {
            auto aimSites = hook::pattern("F7 D8 89 44 24 28 E9");
            if (aimSites.empty())
            {
                (void)hook::pattern(aimSites).get_first();
                std::abort();
            }
            if (aimSites.size() != 2)
            {
                aimSites.count(2);
                std::abort();
            }
            aimSites.for_each_result([](hook::pattern_match match)
            {
                AimHooks.push_back(safetyhook::create_mid(match.get<void>(6), [](SafetyHookContext& context)
                {
                    if (!OnFootAim())
                        return;
                    TakeAim(*reinterpret_cast<int32_t*>(context.esp + 0x2C), *reinterpret_cast<int32_t*>(context.esp + 0x30));
                    *reinterpret_cast<int32_t*>(context.esp + 0x1C) = 0;
                    *reinterpret_cast<int32_t*>(context.esp + 0x28) = 0;
                }));
            });
            // The precision aim's turn: x and y axis reads
            auto turn = hook::pattern("6A 01 6A 02 8B CB F3 0F 11 05 ? ? ? ? E8 ? ? ? ? 6A 01 6A 03 8B CB 8B F8 E8");
            AxisValue = reinterpret_cast<decltype(AxisValue)>(injector::GetBranchDestination(turn.get_first(14)).as_int());
            injector::MakeCALL(turn.get_first(14), AimAxis<false>, true);
            injector::MakeCALL(turn.get_first(27), AimAxis<true>, true);
            // The yaw and pitch turns, without their wrap
            injector::MakeCALL(hook::get_pattern("F3 0F 59 05 ? ? ? ? 51 F3 0F 11 04 24 E8 ? ? ? ? D8 0D ? ? ? ? 8D 84 24", 14), Unwrapped, true);
            injector::MakeCALL(hook::get_pattern("F7 DB F3 0F 2A C3 F3 0F 59 05 ? ? ? ? 51 F3 0F 11 04 24 E8", 20), Unwrapped, true);
        }
        else // TrueCrimeMB.exe: x at esp+20, y in eax before it is negated, counts at esp+38/3C
        {
            AimHooks.push_back(safetyhook::create_mid(hook::pattern("F7 D8 89 44 24 2C DD D8").get_first(), [](SafetyHookContext& context)
            {
                if (!OnFootAim())
                    return;
                TakeAim(*reinterpret_cast<int32_t*>(context.esp + 0x38), *reinterpret_cast<int32_t*>(context.esp + 0x3C));
                *reinterpret_cast<int32_t*>(context.esp + 0x20) = 0;
                context.eax = 0;
            }));
            // The on-foot one of the two precision aim turns (the other is from the car)
            auto turn = hook::pattern("6A 01 6A 02 8B CF E8 ? ? ? ? 6A 01 6A 03 8B CF 89 44 24 ? E8").count(2).get(1);
            AxisValue = reinterpret_cast<decltype(AxisValue)>(injector::GetBranchDestination(turn.get<void>(6)).as_int());
            injector::MakeCALL(turn.get<void>(6), AimAxis<false>, true);
            injector::MakeCALL(turn.get<void>(21), AimAxis<true>, true);
            // The yaw and pitch turns, without their wrap
            injector::MakeCALL(hook::get_pattern("D8 0D ? ? ? ? 51 D9 1C 24 E8 ? ? ? ? D8 0D ? ? ? ? 8D 84 24", 10), Unwrapped, true);
            injector::MakeCALL(hook::get_pattern("F7 DB 89 5C 24 18 DB 44 24 18 68 00 00 00 3F 68 00 00 00 BF 51 D8 0D", 30), Unwrapped, true);
        }
        // Guns mode: the mouse part of the camera axes, where the game's input builds them (x stored, y about to be)
        auto flags = hook::pattern("8A 0D ? ? ? ? 74 ? 80 3D");
        if (!flags.empty()) // TrueCrime_.exe: x at esp+20, y in eax, counts at esp+2C/30
        {
            ControllerFlags = *flags.get_first<uint8_t*>(2);
            AimHooks.push_back(safetyhook::create_mid(hook::pattern("F3 0F 2C C1 F6 C1 08 89 44 24 24").get_first(4), [](SafetyHookContext& context)
            {
                if (!GunsOnFoot())
                    return;
                TakeGun(*reinterpret_cast<int32_t*>(context.esp + 0x2C), *reinterpret_cast<int32_t*>(context.esp + 0x30));
                *reinterpret_cast<int32_t*>(context.esp + 0x20) = 0;
                context.eax = 0;
            }));
        }
        else // TrueCrimeMB.exe: x at esp+24, y in eax, counts at esp+38/3C
        {
            ControllerFlags = *hook::pattern("8A 1D ? ? ? ? 74").get_first<uint8_t*>(2);
            AimHooks.push_back(safetyhook::create_mid(hook::pattern("F6 C3 08 DD D8 89 44 24 28").get_first(), [](SafetyHookContext& context)
            {
                if (!GunsOnFoot())
                    return;
                TakeGun(*reinterpret_cast<int32_t*>(context.esp + 0x38), *reinterpret_cast<int32_t*>(context.esp + 0x3C));
                *reinterpret_cast<int32_t*>(context.esp + 0x24) = 0;
                context.eax = 0;
            }));
        }
        // The guns camera turn, after the game's invert (al holds the invert option, esi the player)
        GunCameraHook = safetyhook::create_mid(hook::pattern("BA ? ? ? ? 85 D2 75 ? B8 ? ? ? ? 85 C0 75 ? 0F 28 46 30").get_first(), [](SafetyHookContext& context)
        {
            const int32_t x = GunX.exchange(0, std::memory_order_relaxed);
            const int32_t y = GunY.exchange(0, std::memory_order_relaxed);
            if (!GunsOnFoot())
                return;
            auto camera = *reinterpret_cast<uint8_t**>(context.esi + 0xE54);
            auto& yaw = *reinterpret_cast<float*>(camera + 356);
            auto& pitch = *reinterpret_cast<float*>(camera + 352);
            const float configuredSensitivity = FixSettings().Float("MAIN.MouseSensitivity");
            const float sensitivity = std::isfinite(configuredSensitivity) ? std::clamp(configuredSensitivity, 0.1f, 5.0f) : 1.0f;
            const float scale = 0.002f * sensitivity;
            const float turn = float(x) * scale * ((*ControllerFlags & 4) ? -1.0f : 1.0f);
            yaw = std::remainder(yaw + turn, 6.2831855f);
            const float vertical = FixSettings().Bool("MAIN.InvertMouseY") ? -1.0f : 1.0f;
            pitch += float(y) * scale * vertical * ((*ControllerFlags & 8) ? -1.0f : 1.0f) * ((context.eax & 0xFF) ? -1.0f : 1.0f);
        });
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("GetPrivateProfileIntA", ReadIniInt),
            std::forward_as_tuple("WritePrivateProfileStringA", WriteIniString));
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.dll", std::forward_as_tuple("DirectInput8Create", Create));
        NativeCreate = reinterpret_cast<decltype(NativeCreate)>(original.at("DirectInput8Create").get());
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("PeekMessageA", PeekMessageOnePass));
        // The camera reads the mouse once per drawn frame (immediate data: the movement since the last read)
        TrueCrimeFreeCamera::ReadMouse = []()
        {
            DIMOUSESTATE state{};
            if (Mouse && SUCCEEDED(DeviceStateHook.stdcall<HRESULT>(Mouse, DWORD(sizeof(state)), &state)) && !TrueCrimeOverlay::BlocksInput())
            {
                TrueCrimeFreeCamera::CountsX.fetch_add(state.lX, std::memory_order_relaxed);
                TrueCrimeFreeCamera::CountsY.fetch_add(state.lY, std::memory_order_relaxed);
            }
        };
    }
}
