#include "stdafx.h"
#include "FixSettings.hpp"
#include "OverlayInput.hpp"
#include "../../external/FusionFixUtils/overlay/OverlayD3D9.hpp"

namespace TCOverlay
{
    static asi_overlay::D3D9Overlay overlay;
    static IUnknown** gameDevice = nullptr;
    static uint8_t** mappedPad = nullptr;
    static SafetyHookInline inputHook, resetHook;
    static IDirect3DDevice9* resetDevice = nullptr;
    static HMODULE hostModule = nullptr;
    static int (__cdecl* gameLanguage)() = nullptr;

    bool BlocksInput() { return overlay.BlocksInput(); }
    bool Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result)
    {
        return overlay.Message(window, message, wparam, lparam, result);
    }

    static void __cdecl ReadMappedInput()
    {
        if (!mappedPad || !*mappedPad) return;
        auto pad = *mappedPad;
        // This is the single game-owned state consumed by keyboard, mouse, Xidi and gamepad actions.
        overlay.Gamepad(*reinterpret_cast<uint16_t*>(pad + 4), int8_t(pad[8]), int8_t(pad[9]));
        if (BlocksInput()) std::memset(pad + 4, 0, 8); // held + edge bits, all four sticks
    }

    // The game's input update: no arguments, it reads its state from globals and keeps the callee-saved registers
    static int __cdecl Input()
    {
        auto result = inputHook.unsafe_ccall<int>();
        ReadMappedInput();
        return result;
    }

    static HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device == resetDevice) overlay.BeforeReset();
        return resetHook.unsafe_stdcall<HRESULT>(device, parameters);
    }

    static void Draw()
    {
        if (!gameDevice || !*gameDevice) return;
        IDirect3DDevice9* device = nullptr;
        if (FAILED((*gameDevice)->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&device)))) return;
        if (device != resetDevice)
        {
            resetHook.reset();
            auto vtable = *reinterpret_cast<void***>(device);
            resetHook = safetyhook::create_inline(vtable[16], Reset);
            if (!resetHook) { device->Release(); return; }
            resetDevice = device;
        }
        overlay.hotkey = FixSettings().Int("MAIN.OverlayHotkey");
        static const char* languages[] = { "en", "en-GB", "fr", "de", "es", "it", "ru" };
        int language = gameLanguage ? gameLanguage() : 0;
        overlay.fallbackLanguage = language >= 0 && language < int(std::size(languages)) ? languages[language] : "en";
        overlay.Present(device, hostModule);
        device->Release();
    }

    void Init()
    {
        if (!asi_settings::ClaimHostRole()) return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &hostModule);
        auto languagePattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C3");
        if (languagePattern.size() >= 2) gameLanguage = reinterpret_cast<decltype(gameLanguage)>(languagePattern.get(1).get<void>());
        auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 8D 4C 24 ? 51 50");
        if (pattern.empty()) return;
        gameDevice = *pattern.get_first<IUnknown**>(1);
        // Capture locations before installing any hooks (the limiter patches the earlier bytes).
        auto present = hook::pattern("8B 76 ? 8B 16 53 53 53 53 56 FF 52 3C");
        auto input = hook::pattern("80 3D ? ? ? ? 00 75 21 80 3D ? ? ? ? 00 75 18 8B 0D");
        if (present.empty() || input.empty()) return;
        mappedPad = *input.get_first<uint8_t**>(20);
        inputHook = safetyhook::create_inline(input.get_first(), Input);
        // Immediately before IDirect3DDevice8::Present, after EndScene and the final UI copies.
        // +10 avoids overlapping Framelimit's mid-hook at the start of this sequence.
        static auto presentHook = safetyhook::create_mid(present.get_first(10), [](SafetyHookContext&) { Draw(); });
    }
}
