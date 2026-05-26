module;

#include <stdafx.h>

export module CrashHandler;

// Bypass the game's crash dialogs so crashes generate proper crash dumps
struct CrashLayout
{
    uintptr_t handleError;   // HandleError() RVA
    uintptr_t funclet;       // Fallback MessageBox funclet RVA
    uintptr_t continuation;  // Resume address RVA
};

static constexpr CrashLayout kKnownLayouts[] =
{
    { 0x3500, 0x3666, 0x36bd },  // Digital version
    { 0x43d0, 0x453a, 0x4599 },  // Retail version
};

// HandleError() prologue signature
static constexpr uint8_t kPrologue[] = { 0x55, 0x8B, 0xEC, 0x6A, 0xFF };

// Unique HandleError() body signature
// Identical in both builds and unique in the whole executable
static constexpr uint8_t kBodySig[]  = { 0x81, 0x79, 0x08, 0x18, 0x03, 0x00, 0x00 };

static SafetyHookInline shCrashDialog1{};
static SafetyHookInline shCrashDialog2{};
static uintptr_t        gContinuationAddr = 0;

static bool BytesMatch(const uint8_t* at, const uint8_t* sig, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        if (at[i] != sig[i])
            return false;
    return true;
}

// Validate a HandleError() candidate using its prologue and body signature
static bool IsHandleError(uintptr_t addr)
{
    if (!BytesMatch(reinterpret_cast<const uint8_t*>(addr), kPrologue, sizeof(kPrologue)))
        return false;

    for (uintptr_t s = addr + 0x40; s < addr + 0xC0; ++s)
        if (BytesMatch(reinterpret_cast<const uint8_t*>(s), kBodySig, sizeof(kBodySig)))
            return true;
    return false;
}

// Replace the crash dialog with an unhandled exception
static void __fastcall HookCrashDialog1(int /*param_1*/)
{
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

// Skip the fallback MessageBox handler
static void* HookCrashDialog2()
{
    return reinterpret_cast<void*>(gContinuationAddr);
}

static void ApplyHooks(uintptr_t handleError, const CrashLayout* layout, uintptr_t exeBase)
{
    // Replace the crash dialog (WCrashBoxDialog) handler
    shCrashDialog1 = safetyhook::create_inline(
        reinterpret_cast<void*>(handleError),
        reinterpret_cast<void*>(&HookCrashDialog1));

    // Hook the fallback MessageBox handler on known builds
    if (layout)
    {
        gContinuationAddr = exeBase + layout->continuation;
        shCrashDialog2 = safetyhook::create_inline(
            reinterpret_cast<void*>(exeBase + layout->funclet),
            reinterpret_cast<void*>(&HookCrashDialog2));
    }
}

export void InitCrashHandler()
{
    auto exeBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));

    // Try known build layouts first
    for (auto& layout : kKnownLayouts)
    {
        auto handleError = exeBase + layout.handleError;
        if (IsHandleError(handleError))
        {
            ApplyHooks(handleError, &layout, exeBase);
            return;
        }
    }

    // Fallback: locate HandleError() by signature scan
    auto pattern = hook::pattern("81 79 08 18 03 00 00 75");
    if (pattern.empty())
        return;

    auto p = reinterpret_cast<uint8_t*>(pattern.get_first());
    for (int i = 0; i < 0x200; ++i, --p)
    {
        if (IsHandleError(reinterpret_cast<uintptr_t>(p)))
        {
            ApplyHooks(reinterpret_cast<uintptr_t>(p), nullptr, exeBase);
            return;
        }
    }
}
