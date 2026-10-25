#pragma once
#include "../Shared/WindowCursor.hpp"

namespace SonicCursor
{
    inline decltype(&UpdateWindow) OriginalUpdateWindow = UpdateWindow;

    inline BOOL WINAPI UpdateWindowHook(HWND window)
    {
        WindowCursor::Attach(window);
        return OriginalUpdateWindow(window);
    }

    inline BOOL WINAPI ReleaseCursor(const RECT*)
    {
        CursorConfinement::Release();
        return TRUE;
    }

    inline void Initialize()
    {
        auto update = hook::pattern("FF 15 ? ? ? ? DB 05 ? ? ? ? 8D 74 24 10 D8 0D ? ? ? ? D9 5C 24 10");
        auto release = hook::pattern("6A 00 FF 15 ? ? ? ? 6A 00 FF 15 ? ? ? ? 5F 5E 33 C0 5B 81 C4 60 01 00 00 C2 10 00");
        if (update.size() != 1 || release.size() != 1)
        {
            OutputDebugStringA("Sonic Heroes: cursor window service unavailable.\n");
            return;
        }
        const auto updateSite = update.get_first<uint8_t>();
        const auto releaseSite = release.get_first<uint8_t>(2);
        const auto original = reinterpret_cast<decltype(OriginalUpdateWindow)>(injector::GetBranchDestination(updateSite).as_int());
        if (!original)
        {
            OutputDebugStringA("Sonic Heroes: cursor window callback unavailable.\n");
            return;
        }
        OriginalUpdateWindow = original;
        WindowCursor::Initialize(FixSettings());
        injector::MakeCALL(updateSite, UpdateWindowHook, true);
        injector::MakeNOP(updateSite + 5, 1, true);
        injector::MakeCALL(releaseSite, ReleaseCursor, true);
        injector::MakeNOP(releaseSite + 5, 1, true);
    }
}
