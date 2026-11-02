module;

#include <stdafx.h>

export module CrashHandler;

static SafetyHookInline shCrashDialog1{};
static SafetyHookMid shCrashDialog2{};
static void* crashDialogContinuation = nullptr;

// Replace the crash dialog with an unhandled exception
static void __fastcall HookCrashDialog1(int /*param_1*/)
{
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

class CrashHandler
{
public:
    CrashHandler()
    {
        WFP::onInitEvent() += []()
        {
            auto dialog1 = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 81 EC 9C 02 00 00 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4");
            void* first;
            void* second;
            if (!dialog1.empty()) // digital
            {
                first = dialog1.get_first();
                second = hook::pattern("A1 ? ? ? ? 83 38 00 74 20 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 0C 89 45 D4 8B 4D D4 89 4D DC").get_first();
                crashDialogContinuation = hook::pattern("C7 45 FC 00 00 00 00 EB 06 B8 ? ? ? ? C3 C7 45 FC FF FF FF FF EB 07 C7 45 FC FF FF FF FF").get_first();
            }
            else // retail
            {
                first = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 81 EC 9C 02 00 00 53 56 57 89 65 F0 89 8D 6C FD FF FF").get_first();
                second = hook::pattern("8B 15 ? ? ? ? 83 3A 00 74 29 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 0C 89 85 58 FD FF FF 8B 85 58 FD FF FF").get_first();
                crashDialogContinuation = hook::pattern("C7 45 FC 00 00 00 00 EB 06 B8 ? ? ? ? C3 C7 45 FC FF FF FF FF 8B 4D F4 64 89 0D 00 00 00 00 5F 5E 5B 8B E5 5D C3").get_first();
            }
            shCrashDialog1 = safetyhook::create_inline(first, reinterpret_cast<void*>(&HookCrashDialog1));
            shCrashDialog2 = safetyhook::create_mid(second, [](SafetyHookContext& context)
            {
                context.eip = reinterpret_cast<uintptr_t>(crashDialogContinuation);
            });
        };
    }
} CrashHandler;
