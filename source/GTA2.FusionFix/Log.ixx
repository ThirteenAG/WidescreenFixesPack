module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Log;

// Diagnostic messages are available to the debugger.
namespace
{
    LPTOP_LEVEL_EXCEPTION_FILTER previousCrashFilter = nullptr;
}

export namespace Log
{
    bool Enabled = false;
    void Write(const char* message)
    {
        if (!Enabled)
            return;
        OutputDebugStringA(message ? message : "");
        OutputDebugStringA("\n");
    }
    // The game's error boxes, with the calls that led to them.
    int WINAPI ErrorMessage(HWND window, LPCSTR message, LPCSTR title, UINT flags)
    {
        Write(title);
        Write(message);
        void* frames[12]{};
        auto count = CaptureStackBackTrace(0, 12, frames, nullptr);
        for (USHORT i = 0; i < count; ++i)
        {
            char address[40]{};
            sprintf_s(address, "frame %u: %p", i, frames[i]);
            Write(address);
        }
        return MessageBoxA(window, message, title, flags);
    }
    LONG WINAPI Crash(EXCEPTION_POINTERS* exception)
    {
        char diagnostic[160]{};
        sprintf_s(diagnostic, "Unhandled exception %08lX at %p", exception->ExceptionRecord->ExceptionCode,
            exception->ExceptionRecord->ExceptionAddress);
        Write(diagnostic);
#if defined(_M_IX86)
        const auto& regs = *exception->ContextRecord;
        sprintf_s(diagnostic, "eax=%08lX ecx=%08lX edx=%08lX edi=%08lX esi=%08lX esp=%08lX ebp=%08lX",
            regs.Eax, regs.Ecx, regs.Edx, regs.Edi, regs.Esi, regs.Esp, regs.Ebp);
        Write(diagnostic);
#endif
        return previousCrashFilter ? previousCrashFilter(exception) : EXCEPTION_CONTINUE_SEARCH;
    }
    void Read(SettingsReader& ini)
    {
        Enabled = ini.ReadBoolean("DIAGNOSTICS", "Enabled", false);
        if (!Enabled)
            return;
        previousCrashFilter = SetUnhandledExceptionFilter(Crash);
        IATHook::Replace(GetModuleHandleW(nullptr), "user32.dll", std::make_tuple("MessageBoxA", ErrorMessage));
    }
}
