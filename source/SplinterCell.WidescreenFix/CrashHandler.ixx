module;

#include <stdafx.h>

export module CrashHandler;

static constexpr uintptr_t kDialogOffset1      = 0x3340;
static constexpr uintptr_t kDialogOffset2      = 0x348e;
static constexpr uintptr_t kContinuationOffset = 0x34e0;

static SafetyHookInline shCrashDialog1{};
static SafetyHookInline shCrashDialog2{};

// Bypass the game's crash dialogs so crashes generate proper crash dumps
static void __fastcall HookCrashDialog1(int /*param_1*/) {}

static void* HookCrashDialog2()
{
    auto exeBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    return reinterpret_cast<void*>(exeBase + kContinuationOffset);
}

export void InitCrashHandler()
{
    auto exeBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));

    shCrashDialog1 = safetyhook::create_inline(
        reinterpret_cast<void*>(exeBase + kDialogOffset1),
        reinterpret_cast<void*>(&HookCrashDialog1));

    shCrashDialog2 = safetyhook::create_inline(
        reinterpret_cast<void*>(exeBase + kDialogOffset2),
        reinterpret_cast<void*>(&HookCrashDialog2));
}
