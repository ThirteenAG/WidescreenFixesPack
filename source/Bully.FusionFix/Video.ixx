module;

#include <stdafx.h>
#include <d3d9.h>
#include <dshow.h>
#include <vmr9.h>

export module Video;

import ComVars;

export namespace BullyVideo
{
    SafetyHookInline UpdateHook;
    SafetyHookInline CleanupHook;
    void* Owner = nullptr;
    IVMRWindowlessControl9* Control = nullptr;
    LONG Width = 0, Height = 0, AspectWidth = 0, AspectHeight = 0;
    LONG ClientWidth = 0, ClientHeight = 0;
    bool Applied = false;
    bool ReportedFailure = false;

    void Reset()
    {
        Owner = nullptr;
        Control = nullptr;
        Width = Height = AspectWidth = AspectHeight = 0;
        ClientWidth = ClientHeight = 0;
        Applied = false;
        ReportedFailure = false;
    }

    void Apply(void* player)
    {
        auto bytes = static_cast<uint8_t*>(player);
        if (!hGameWindow.is_initialized() || !player || *reinterpret_cast<int*>(bytes + 4) != 1 ||
            !*reinterpret_cast<void**>(bytes + 16))
            return;
        auto control = *reinterpret_cast<IVMRWindowlessControl9**>(bytes + 24);
        if (!control)
            return;
        if (Owner != player || Control != control)
        {
            Reset();
            Owner = player;
            Control = control;
        }
        RECT client{};
        if (!GetClientRect(hGameWindow, &client) || client.right <= 0 || client.bottom <= 0)
            return;
        if (Applied && ClientWidth == client.right && ClientHeight == client.bottom)
            return;
        if (Width <= 0 || Height <= 0)
        {
            if (FAILED(control->GetNativeVideoSize(&Width, &Height, &AspectWidth, &AspectHeight)) ||
                Width <= 0 || Height <= 0 || Width > 16384 || Height > 16384)
            {
                Width = Height = 0;
                return;
            }
        }
        // The whole movie fitted to the window at its own aspect (the movies are 16:9)
        RECT source{ 0, 0, Width, Height };
        const HRESULT border = control->SetBorderColor(RGB(0, 0, 0));
        const HRESULT aspect = control->SetAspectRatioMode(VMR9ARMode_LetterBox);
        const HRESULT position = control->SetVideoPosition(&source, &client);
        if (FAILED(border) || FAILED(aspect) || FAILED(position))
        {
            if (!ReportedFailure)
            {
                OutputDebugStringA("Bully: movie layout unavailable.\n");
                ReportedFailure = true;
            }
            return;
        }
        Applied = true;
        ClientWidth = client.right;
        ClientHeight = client.bottom;
    }

    char __fastcall Update(void* player, void*)
    {
        Apply(player);
        return UpdateHook.thiscall<char>(player);
    }

    char __fastcall Cleanup(void* player, void*)
    {
        if (Owner == player)
            Reset();
        return CleanupHook.thiscall<char>(player);
    }

    void Install(HMODULE module = GetModuleHandle(nullptr))
    {
        auto update = hook::module_pattern(module, "51 56 8B F1 83 7E 10 00 74 26 83 7E 04 01 75 20");
        auto cleanup = hook::module_pattern(module, "53 56 8B F1 8B 46 1C 83 F8 FF 74 0E 50 FF 15 ? ? ? ? C7 46 1C FF FF FF FF");
        auto window = hook::module_pattern(module, "8B 15 ? ? ? ? 8B 08 52 50 8B 41 28 FF D0");
        hGameWindow.SetAddress(*window.get_first<HWND*>(2));
        CleanupHook = safetyhook::create_inline(cleanup.get_first(), Cleanup);
        UpdateHook = safetyhook::create_inline(update.get_first(), Update);
    }
}
