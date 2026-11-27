module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/ActivePad.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "Shared/Overlay/D3D9UpdateNotice.hpp"

export module Overlay;

import ComVars;
import WidescreenHUD;

// Ctrl+F10 overlay (ImGui)
namespace TCOverlay
{
    static asi_overlay::D3D9Overlay overlay;
    static SafetyHookInline inputHook, resetHook;
    static IDirect3DDevice9* resetDevice = nullptr;
    static HMODULE hostModule = nullptr;
    static bool host = false;
    static std::atomic<uint32_t> inputGeneration{ 0 };
    static asi_overlay::D3D9UpdateNotice notice;
    static const char* gameLanguage = "en";

    export bool BlocksInput()
    {
        return overlay.BlocksInput();
    }

    export uint32_t InputGeneration()
    {
        return inputGeneration.load(std::memory_order_relaxed);
    }

    static const char* ResolveNativeLanguage()
    {
        static constexpr const char* languages[] = { "en", "en-GB", "fr", "de", "es", "it", "ru" };
        const auto configured = FixSettings().Int("MAIN.Language");
        if (configured >= 0 && static_cast<size_t>(configured) < std::size(languages))
            return languages[configured];

        constexpr auto key = L"Software\\Aspyr\\True Crime New York City\\Install";
        for (const auto hive : { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE })
        {
            DWORD language{}, size = sizeof(language);
            if (RegGetValueW(hive, key, L"language", RRF_RT_REG_DWORD, nullptr, &language, &size) == ERROR_SUCCESS)
                return language < std::size(languages) ? languages[language] : "en";
        }
        return "en";
    }

    export std::wstring FormatUpdateText(const std::string& latest, bool hub)
    {
        return asi_overlay::D3D9UpdateNotice::Format(FixSettings(), gameLanguage, latest, hub);
    }

    export void ShowUpdate(const std::wstring& text)
    {
        notice.Show(text);
    }

    static void UpdateInputGeneration(bool wasBlocking)
    {
        if (wasBlocking != BlocksInput())
            inputGeneration.fetch_add(1, std::memory_order_relaxed);
    }

    export bool Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
    {
        const bool wasBlocking = BlocksInput();
        const bool handled = overlay.Message(window, message, wparam, lparam, result);
        UpdateInputGeneration(wasBlocking);
        return handled;
    }

    // The game's input update: no arguments, it reads its state from globals and keeps the callee-saved registers
    static int __cdecl Input()
    {
        const bool wasBlocking = BlocksInput();
        static PadInput::ActivePad controller;
        XINPUT_STATE state{};
        const bool connected = controller.Read(state);
        PadInput::ToOverlay(overlay, connected ? state.Gamepad : XINPUT_GAMEPAD{});
        UpdateInputGeneration(wasBlocking);

        const auto result = inputHook.unsafe_ccall<int>();
        if ((wasBlocking || BlocksInput()) && pMappedPad)
            std::memset(pMappedPad.get() + 4, 0, 8); // held and edge bits, all four sticks
        return result;
    }

    static HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device == resetDevice)
        {
            notice.BeforeReset();
            overlay.BeforeReset();
            WFP::onBeforeReset().executeAll();
        }
        return resetHook.unsafe_stdcall<HRESULT>(device, parameters);
    }

    static void Draw()
    {
        WFP::onGameProcessEvent().executeAll();
        if (!pD3DDevice) return;
        IDirect3DDevice9* device = nullptr;
        if (FAILED(pD3DDevice->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&device)))) return;
        if (device != resetDevice)
        {
            if (resetDevice)
            {
                notice.BeforeReset();
                overlay.BeforeReset();
                WFP::onBeforeReset().executeAll();
            }
            resetHook.reset();
            auto vtable = *reinterpret_cast<void***>(device);
            resetHook = safetyhook::create_inline(vtable[16], Reset);
            resetDevice = device;
        }
        notice.Present(device, hostModule, HudPlaying() && !bCutscene && !BlocksInput());
        if (WFP::DeveloperOverlay && host)
        {
            const bool wasBlocking = BlocksInput();
            overlay.Present(device, hostModule);
            UpdateInputGeneration(wasBlocking);
        }
        device->Release();
    }

    // Installed from dllmain before the other hooks
    export void Init()
    {
        host = asi_settings::ClaimHostRole();
        gameLanguage = ResolveNativeLanguage();
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &hostModule);
        if (WFP::DeveloperOverlay && host)
            overlay.fallbackLanguage = gameLanguage;
        auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 8D 4C 24 ? 51 50");
        pD3DDevice.SetAddress(*pattern.get_first<IUnknown**>(1));
        // Capture locations before installing any hooks
        auto present = hook::pattern("8B 76 ? 8B 16 53 53 53 53 56 FF 52 3C");
        // The overlay's pad and its input block exist only in Debug builds
        if (WFP::DeveloperOverlay && host)
        {
            auto input = hook::pattern("80 3D ? ? ? ? 00 75 21 80 3D ? ? ? ? 00 75 18 8B 0D");
            pMappedPad.SetAddress(*input.get_first<uint8_t**>(20));
            inputHook = safetyhook::create_inline(input.get_first(), Input);
        }
        // Immediately before IDirect3DDevice8::Present, after EndScene and the final UI copies
        static auto presentHook = safetyhook::create_mid(present.get_first(10), [](SafetyHookContext&) { Draw(); });
    }
}
