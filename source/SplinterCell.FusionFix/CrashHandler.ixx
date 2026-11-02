module;

#include <stdafx.h>

export module CrashHandler;

static SafetyHookInline shCrashDialog1{};
static SafetyHookMid shCrashDialog2{};
static void* crashDialogContinuation = nullptr;

// Bypass the game's crash dialogs so crashes generate proper crash dumps
static void __fastcall HookCrashDialog1(int /*param_1*/) {}

class CrashHandler
{
public:
    CrashHandler()
    {
        WFP::onInitEvent() += []()
        {
            auto dialog1 = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 81 EC 70 02 00 00 53 56 57 89 65 F0 89 8D 8C").get_first();
            auto dialog2 = hook::pattern("8B 0D ? ? ? ? 83 39 00 74 1D 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 0C 89 85 80 FD FF FF EB 0A").get_first();
            crashDialogContinuation = hook::pattern("C7 45 FC 00 00 00 00 EB 06 B8 ? ? ? ? C3 C7 45 FC FF FF FF FF 8B 4D F4 64 89 0D 00 00 00 00 5F 5E 5B").get_first();

            shCrashDialog1 = safetyhook::create_inline(
                dialog1,
                reinterpret_cast<void*>(&HookCrashDialog1));

            shCrashDialog2 = safetyhook::create_mid(dialog2, [](SafetyHookContext& context)
            {
                context.eip = reinterpret_cast<uintptr_t>(crashDialogContinuation);
            });
        };
    }
} CrashHandler;
