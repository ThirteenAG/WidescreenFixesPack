module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <atomic>
#include <memory>

export module Diagnostics;


export namespace FarCryDiagnostics
{
    inline std::atomic<bool> Warned{ false };

    struct Notice
    {
        std::wstring title, text;
    };

    inline DWORD WINAPI Show(void* argument)
    {
        std::unique_ptr<Notice> notice(static_cast<Notice*>(argument));
        HWND window = nullptr;
        for (unsigned attempt = 0; attempt < 300 && !window; ++attempt)
        {
            EnumWindows([](HWND candidate, LPARAM result) -> BOOL
            {
                DWORD process = 0;
                GetWindowThreadProcessId(candidate, &process);
                if (process != GetCurrentProcessId() || !IsWindowVisible(candidate) || GetWindow(candidate, GW_OWNER))
                    return TRUE;
                *reinterpret_cast<HWND*>(result) = candidate;
                return FALSE;
            }, reinterpret_cast<LPARAM>(&window));
            if (!window)
                Sleep(100);
        }
        if (window)
            MessageBoxW(window, notice->text.c_str(), notice->title.c_str(), MB_OK | MB_ICONWARNING);
        return 0;
    }

    inline void Unsupported(const char* reason)
    {
        OutputDebugStringA(reason);
        if (Warned.exchange(true))
            return;
        const auto& texts = FixSettings().GetTexts();
        auto notice = std::make_unique<Notice>(asi_settings::text::Widen(texts.Resolve("@plugin.name")),
            asi_settings::text::Widen(texts.Resolve("@diagnostic.unsupported")));
        const auto thread = CreateThread(nullptr, 0, Show, notice.get(), 0, nullptr);
        if (thread)
        {
            notice.release();
            CloseHandle(thread);
        }
    }
}
