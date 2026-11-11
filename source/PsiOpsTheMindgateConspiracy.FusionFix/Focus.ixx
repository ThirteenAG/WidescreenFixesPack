module;
#include <stdafx.h>
#include "FixSettings.hpp"

export module Focus;
import Window;
import FloatingPoint;


export namespace PsiOpsFocus
{
    inline SafetyHookInline updateHook;
    inline void(__thiscall* requestPause)(void*, int) = nullptr;
    inline std::atomic<bool> enabled{ true };

    inline uintptr_t __fastcall Update(void* self, void*)
    {
        bool pause = false;
        {
            PsiOps::FloatingPointScope floatingPoint;
            pause = PsiOpsWindow::blocksInput && PsiOpsWindow::blocksInput();
            if (enabled && PsiOpsWindow::window)
                pause = pause || IsIconic(PsiOpsWindow::window) || GetForegroundWindow() != PsiOpsWindow::window;
        }
        if (pause)
            requestPause(self, 1);
        return updateHook.unsafe_thiscall<uintptr_t>(self);
    }

    inline void Init()
    {
        auto update = hook::pattern("55 8B EC 83 EC 0C 89 4D F4 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 45 F4 0F B6 48 24 85 C9 75 41");
        auto request = hook::pattern("55 8B EC 51 89 4D FC 8B 45 FC 8B 4D 08 3B 48 6C 7E 09 8B 55 FC 8B 45 08 89 42 6C 8B E5 5D C2 04 00");
        requestPause = reinterpret_cast<decltype(requestPause)>(request.get_first());
        updateHook = safetyhook::create_inline(update.get_first(), Update);
        const auto refresh = []() { enabled = FixSettings().Bool("MAIN.PauseOnFocusLoss"); };
        refresh();
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", refresh);
    }
}
