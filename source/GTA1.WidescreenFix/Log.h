#pragma once

// The log file next to the game executable (GTA1.WidescreenFix.log), and what goes into it:
// messages of the fix, error boxes of the game and, with [DIAGNOSTICS] Enabled,
// unhandled exceptions.
namespace Log
{
    inline bool Enabled = false;                   // [DIAGNOSTICS] Enabled
    inline void Write(const char* message)
    {
        wchar_t path[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) return;
        auto file = std::filesystem::path(path).parent_path() / L"GTA1.WidescreenFix.log";
        FILE* stream = nullptr;
        if (_wfopen_s(&stream, file.c_str(), L"a") == 0 && stream)
        {
            fprintf(stream, "%lu: %s\n", GetTickCount(), message ? message : "");
            fclose(stream);
        }
    }
    // The game's error boxes, with the calls that led to them.
    inline int WINAPI ErrorMessage(HWND window, LPCSTR message, LPCSTR title, UINT flags)
    {
        Write(title); Write(message);
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
    inline LONG WINAPI Crash(EXCEPTION_POINTERS* exception)
    {
        char diagnostic[160]{};
        sprintf_s(diagnostic, "Unhandled exception %08lX at %p", exception->ExceptionRecord->ExceptionCode,
            exception->ExceptionRecord->ExceptionAddress);
        Write(diagnostic);
#if defined(_M_IX86)
        sprintf_s(diagnostic, "eax=%08lX ecx=%08lX edx=%08lX edi=%08lX esi=%08lX esp=%08lX", exception->ContextRecord->Eax, exception->ContextRecord->Ecx, exception->ContextRecord->Edx, exception->ContextRecord->Edi, exception->ContextRecord->Esi, exception->ContextRecord->Esp);
        Write(diagnostic);
        __try
        {
            auto frame = reinterpret_cast<uint32_t*>(exception->ContextRecord->Ebp);
            sprintf_s(diagnostic, "native frame return=%08X args=%08X,%08X,%08X,%08X,%08X", frame[1], frame[2], frame[3], frame[4], frame[5], frame[6]);
            Write(diagnostic);
            sprintf_s(diagnostic, "native caller=%08X stack=%08X,%08X,%08X,%08X,%08X,%08X", frame[7], frame[8], frame[9], frame[10], frame[11], frame[12], frame[13]);
            Write(diagnostic);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
        return EXCEPTION_CONTINUE_SEARCH;
    }
    inline void Read(CIniReader& ini)
    {
        Enabled = ini.ReadBoolean("DIAGNOSTICS", "Enabled", false);
        if (Enabled) SetUnhandledExceptionFilter(Crash);
    }
    inline void Install()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "user32.dll", std::make_tuple("MessageBoxA", ErrorMessage));
    }
}
