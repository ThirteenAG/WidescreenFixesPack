module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"


export module Cursor;


export namespace SniperCursor
{
    inline decltype(&UpdateWindow) OriginalUpdateWindow = UpdateWindow;

    inline BOOL WINAPI UpdateWindowHook(HWND window)
    {
        WindowCursor::Attach(window);
        return OriginalUpdateWindow(window);
    }

    inline void Initialize()
    {
        auto pattern = hook::pattern("50 FF 15 ? ? ? ? B0 01 5E 83 C4 28 C3 CC CC CC CC");
        const auto site = pattern.get_first<uint8_t>(1);
        OriginalUpdateWindow = reinterpret_cast<decltype(OriginalUpdateWindow)>(injector::GetBranchDestination(site).as_int());
        WindowCursor::Initialize(FixSettings());
        injector::MakeCALL(site, UpdateWindowHook, true);
        injector::MakeNOP(site + 5, 1, true);
    }
}
