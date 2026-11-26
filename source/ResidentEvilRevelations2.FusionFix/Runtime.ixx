module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/WindowCursor.hpp"
#include <mutex>
#include <map>

export module Runtime;

import ComVars;
import FrameRate;
import Camera;
import DeveloperOverlay;
import NativeMenu;
import ControllerIcons;
import LightSync;

namespace Rev2Runtime
{
    namespace
    {
        SafetyHookInline inputDeviceHook, activationHook;
        std::atomic<int> windowedState{ -1 };

        void InstallResetRecovery()
        {
            auto uiReset = hook::pattern("8B 93 88 26 00 00 8B 35 ? ? ? ? 8B 02 68 ? ? ? ? FF B3 90 26 00 00 52 FF 50 40 50 8B CE E8 ? ? ? ? EB ?");
            using CheckResult = void (__thiscall*)(void*, HRESULT, const char*);
            static const auto checkResult = reinterpret_cast<CheckResult>(injector::GetBranchDestination(uiReset.get_first(32)).as_int());
            static const auto resetLabel = *uiReset.get_first<const char*>(15);
            static const auto complete = injector::ReadRelativeOffset(uiReset.get_first(38), 1).as_int();
            static const auto pending = complete + 10; // Skip clearing the UI thread's reset request.
            static auto hook = safetyhook::create_mid(uiReset.get_first(12), [](SafetyHookContext& regs)
            {
                static thread_local ULONGLONG retryAt{};
                const auto now = GetTickCount64();
                regs.eip = pending;
                if (now < retryAt)
                    return;

                const auto device = reinterpret_cast<IDirect3DDevice9*>(regs.edx);
                if (device->TestCooperativeLevel() == D3DERR_DEVICELOST)
                {
                    retryAt = now + 32;
                    return;
                }

                const auto parameters = *reinterpret_cast<D3DPRESENT_PARAMETERS**>(regs.ebx + 0x2690);
                const auto result = device->Reset(parameters);
                if (result == D3DERR_DEVICELOST)
                {
                    retryAt = now + 32;
                    return;
                }
                retryAt = 0;
                checkResult(reinterpret_cast<void*>(regs.esi), result, resetLabel);
                regs.eip = complete;
            });
        }

        BOOL __stdcall Activation(bool active)
        {
            if (!active && !Rev2Overlay::PauseOnFocusLoss.load() && windowedState.load() == 1)
                return TRUE;
            return activationHook.unsafe_stdcall<BOOL>(active);
        }

        std::mutex inputHooksMutex;
        std::map<void*, SafetyHookInline> inputStateHooks, inputDataHooks;
        HRESULT (WINAPI* inputCreate)(HINSTANCE, DWORD, const IID*, void**, void*) = nullptr;
        SHORT (WINAPI* gameKeyState)(int) = nullptr;
        DWORD (WINAPI* gamePadState)(DWORD, XINPUT_STATE*) = nullptr;

        SHORT WINAPI GameKeyState(int key)
        {
            return Rev2Overlay::BlocksInput() ? 0 : gameKeyState(key);
        }

        DWORD WINAPI GamePadState(DWORD index, XINPUT_STATE* state)
        {
            const auto result = gamePadState(index, state);
            if (result == ERROR_SUCCESS && state && Rev2Overlay::BlocksInput())
            {
                state->Gamepad = {};
                state->dwPacketNumber ^= 0x80000000u;
            }
            return result;
        }

        HRESULT WINAPI InputState(void* device, DWORD size, void* data)
        {
            SafetyHookInline* original;
            {
                std::lock_guard lock(inputHooksMutex);
                original = &inputStateHooks.at((*reinterpret_cast<void***>(device))[9]);
            }
            const auto result = original->unsafe_stdcall<HRESULT>(device, size, data);
            if (SUCCEEDED(result) && data && Rev2Overlay::BlocksInput())
                std::memset(data, 0, size);
            return result;
        }

        HRESULT WINAPI InputData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
        {
            SafetyHookInline* original;
            {
                std::lock_guard lock(inputHooksMutex);
                original = &inputDataHooks.at((*reinterpret_cast<void***>(device))[10]);
            }
            const bool blocked = Rev2Overlay::BlocksInput();
            const auto result = original->unsafe_stdcall<HRESULT>(device, size, data, count, blocked ? flags & ~1u : flags);
            if (SUCCEEDED(result) && count && blocked)
                *count = 0;
            return result;
        }

        HRESULT WINAPI InputDevice(void* input, const GUID* guid, void** device, void* outer)
        {
            const auto result = inputDeviceHook.unsafe_stdcall<HRESULT>(input, guid, device, outer);
            if (SUCCEEDED(result) && device && *device)
            {
                const auto table = *reinterpret_cast<void***>(*device);
                std::lock_guard lock(inputHooksMutex);
                if (auto [entry, added] = inputStateHooks.try_emplace(table[9]); added)
                    entry->second = safetyhook::create_inline(table[9], InputState);
                if (auto [entry, added] = inputDataHooks.try_emplace(table[10]); added)
                    entry->second = safetyhook::create_inline(table[10], InputData);
            }
            return result;
        }

        HRESULT WINAPI CreateInput(HINSTANCE instance, DWORD version, const IID* iid, void** output, void* outer)
        {
            const auto result = inputCreate(instance, version, iid, output, outer);
            if (SUCCEEDED(result) && output && *output && !inputDeviceHook)
                inputDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(*output))[3], InputDevice);
            return result;
        }
    }

    void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (!device || FAILED(device->GetCreationParameters(&parameters)))
            return;
        DWORD process = 0;
        GetWindowThreadProcessId(parameters.hFocusWindow, &process);
        if (process != GetCurrentProcessId())
            return;
        if (windowedState.load() < 0)
        {
            IDirect3DSwapChain9* chain = nullptr;
            if (SUCCEEDED(device->GetSwapChain(0, &chain)) && chain)
            {
                D3DPRESENT_PARAMETERS presentation{};
                if (SUCCEEDED(chain->GetPresentParameters(&presentation)))
                    windowedState.store(presentation.Windowed ? 1 : 0);
                chain->Release();
            }
        }
        IDirect3DSurface9* backBuffer = nullptr;
        if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) && backBuffer)
        {
            D3DSURFACE_DESC desc{};
            if (SUCCEEDED(backBuffer->GetDesc(&desc)))
            {
                SetBackBufferSize(desc.Width, desc.Height);
                const LONG width = static_cast<LONG>(desc.Width), height = static_cast<LONG>(desc.Height);
                const LONG side = (width - height * 16 / 9) / 2;
                if (side > 0 && height > 0 && Rev2Camera::ShowPillarbox())
                {
                    const RECT bars[] = {
                        { 0, 0, side, height },
                        { width - side, 0, width, height }
                    };
                    for (const auto& bar : bars)
                        device->ColorFill(backBuffer, &bar, D3DCOLOR_XRGB(0, 0, 0));
                }
            }
            backBuffer->Release();
        }
        Rev2Icons::Frame(device);
        Rev2Overlay::AttachWindow(parameters.hFocusWindow);
        if (Rev2Overlay::cursorWindow.load() != parameters.hFocusWindow)
            PostMessageW(parameters.hFocusWindow, WM_NULL, 0, 0);
        CursorConfinement::Suspend(Rev2Overlay::BlocksInput());
        Rev2Overlay::Keyboard(parameters.hFocusWindow);
        Rev2Overlay::Draw(device);
        Rev2FrameRate::Frame(parameters.hFocusWindow);
    }

    void BeforeReset(IDirect3DDevice9* device)
    {
        windowedState.store(-1);
        Rev2Icons::BeforeReset();
        Rev2Overlay::BeforeReset(device);
    }

    export void AttachDevice(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        static std::mutex mutex;
        std::lock_guard guard(mutex);
        D3D9Options::HookDevice(device);
    }

    export void Init()
    {
        InstallResetRecovery();
        auto update = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 07 8B CF FF 50 18 8B 0D ? ? ? ? 80 B9 ? 26 00 00 00");
        static auto gameUpdate = safetyhook::create_mid(update.get_first(18), [](SafetyHookContext&)
        {
            Rev2Camera::SampleCutscene();
            Rev2LightSync::Sample();
        });
        BindLiveInt("MAIN.ConfineCursor", [](int value)
        {
            CursorConfinement::SetEnabled(value != 0);
        });
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
        BindLiveInt("MAIN.PauseOnFocusLoss", [](int value)
        {
            Rev2Overlay::PauseOnFocusLoss.store(value != 0);
            const auto message = Rev2Overlay::FocusSettingMessage();
            if (const auto window = Rev2Overlay::cursorWindow.load(); window && message)
                PostMessageW(window, message, 0, 0);
        });
        auto activation = hook::pattern("80 7C 24 04 00 A1 ? ? ? ? 74 ? C6 80 51 25 00 00 01");
        activationHook = safetyhook::create_inline(activation.get_first(), Activation);
        Rev2Overlay::FocusSettingChanged = [](HWND window)
        {
            if (windowedState.load() == 1 && !Rev2Overlay::Active(window))
                activationHook.unsafe_stdcall<BOOL>(!Rev2Overlay::PauseOnFocusLoss.load());
        };
        Rev2Menu::Init();
        Rev2Icons::Init();
        auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.DLL",
            std::forward_as_tuple("DirectInput8Create", reinterpret_cast<void*>(CreateInput)));
        inputCreate = reinterpret_cast<decltype(inputCreate)>(imports.at("DirectInput8Create").get());
        auto userImports = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("GetAsyncKeyState", reinterpret_cast<void*>(GameKeyState)));
        gameKeyState = reinterpret_cast<decltype(gameKeyState)>(userImports.at("GetAsyncKeyState").get());
        auto padImports = IATHook::Replace(GetModuleHandleW(nullptr), "XINPUT1_3.dll",
            std::forward_as_tuple("XInputGetState@2", reinterpret_cast<void*>(GamePadState)));
        gamePadState = reinterpret_cast<decltype(gamePadState)>(padImports.at("XInputGetState@2").get());
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = BeforeReset;
        D3D9Options::HookCreation();
    }
}
