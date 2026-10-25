module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;
import Input;

namespace IntroFMVScreen
{
    SafetyHookInline shNotificationMessage = {};
    void __fastcall NotificationMessage(void* FEPackage, void* edx, int a2, void* FEObject, int a4, int a5)
    {
        // pop the package once for the intro skip
        static bool bOnceFlag = false;
        if (!bOnceFlag)
        {
            bOnceFlag = true;
            cFEng::PopPackage(*(char**)((uintptr_t)FEPackage + 0xC));
            return;
        }

        return shNotificationMessage.unsafe_fastcall(FEPackage, edx, a2, FEObject, a4, a5);
    }
}

// Frame rate. PrepareRealTimestep waits until the target frame time has passed and then clamps the
// frame's time step to at least 1/60 s, so the game is capped at 60 FPS and would run too fast above
// it. The game also counts time in 1/60 s ticks and adds at least one tick per frame.
namespace FrameRate
{
    float fTargetFrameTime = 1.0f / 60.0f;  // the limiter's target
    float fMinFrameTime = 1.0f / 60.0f;     // smallest time step of a frame

    // EAXSound::Update was only ever called at up to 60 FPS. Unlike NFSU2's, it has no 15 ms gate of its
    // own that drops time, so nothing is lost at high frame rates, but above 60 FPS the frames are batched
    // so that every update covers at least 1/60 s, as in the original game. The skipped frames' time is
    // passed on with the next update. The game's time steps are multiples of 1/3600 s, so a small margin
    // keeps rounding from skipping an update at exactly 60 FPS.
    float fAudioTime = 0.0f;
    void* EAXSoundUpdate = nullptr;

    bool __cdecl ShouldUpdateAudio(float* dt)
    {
        if (*dt <= 0.0f)
            return true;

        fAudioTime += *dt;
        if (fAudioTime < 1.0f / 60.0f - 1.0f / 7200.0f)
            return false;

        *dt = fAudioTime;
        fAudioTime = 0.0f;
        return true;
    }

    // EAXSound::Update: this in eax, time step on the stack, callee cleans up
    void __declspec(naked) EAXSoundUpdateHook()
    {
        _asm
        {
            push eax
            lea ecx, [esp + 8]
            push ecx
            call ShouldUpdateAudio
            add esp, 4
            test al, al
            pop eax
            jz skip
            jmp dword ptr [EAXSoundUpdate]
        skip:
            ret 4
        }
    }
}

// The game's memory allocator, bMalloc and bFree. The memory pools lock their free lists, but bMalloc adds the new
// block to the pool's list of allocations and bFree takes it out without any lock, and both update the allocation
// counter and create or destroy the main pool unlocked. The game allocates and frees on other threads too (the
// network address lookup thread frees its request, the sound and file threads run game callbacks), so two threads
// at once corrupt the list and the game later reads a freshly allocated block (filled with 0xAA) as a pointer:
// the random "0xAAAAAAAA" crash on multi-core CPUs that is usually worked around by running the game on one
// core. Both functions are made to hold one lock for the whole call instead.
namespace Heap
{
    CRITICAL_SECTION Lock;

    void __cdecl LockHeap() { EnterCriticalSection(&Lock); }
    void __cdecl UnlockHeap() { LeaveCriticalSection(&Lock); }

    SafetyHookInline shMalloc = {};
    void* __cdecl Malloc(int size, const char* debugName, int debugLine, int flags)
    {
        LockHeap();
        auto result = shMalloc.unsafe_ccall<void*>(size, debugName, debugLine, flags);
        UnlockHeap();
        return result;
    }

    // bFree: the block in eax, no stack arguments. All registers are kept for the callers.
    void* FreeOriginal = nullptr;
    void __declspec(naked) Free()
    {
        _asm
        {
            pushad
            call LockHeap
            popad
            push eax
            call dword ptr [FreeOriginal]
            pushad
            call UnlockHeap
            popad
            pop eax
            ret
        }
    }
    SafetyHookInline shFree = {};
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            // bMalloc (called by the texture loader) and bFree
            {
                auto malloc = hook::pattern("57 57 50 E8 ? ? ? ? 8B 4C 24 ? 68 00 20 00 00");
                auto free = hook::pattern("85 C0 74 3D 8B 0D ? ? ? ? 85 C9 56 0F B6 70 F0 57 8D 78 E8");
                if (!malloc.empty() && !free.empty())
                {
                    InitializeCriticalSectionAndSpinCount(&Heap::Lock, 4000);
                    Heap::shMalloc = safetyhook::create_inline(injector::GetBranchDestination(malloc.get_first(3)).get_raw<void>(), Heap::Malloc);
                    Heap::shFree = safetyhook::create_inline(free.get_first(), Heap::Free);
                    Heap::FreeOriginal = Heap::shFree.original<void*>();
                }
            }

            bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
            bool bShowLangSelect = FixSettings().Bool("MAIN.ShowLangSelect");
            int nFPSLimit = FixSettings().Int("MAIN.FPSLimit");

            WFP::onGameInitEvent() += []()
            {
                SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
                SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)));
            };

            if (bSkipIntro)
            {
                // LS_IntroFMV.fng needs to stay in the boot flow, otherwise the splash screen music doesn't play.
                // The intro video still technically plays, so e3_title.mad still needs to be in the movies folder.
                static const char* BootFlowScreens[] = { "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };
                static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };

                const char** outScreens = bShowLangSelect ? BootFlowScreensLangSelect : BootFlowScreens;
                const char** outLastScreen = bShowLangSelect ? &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1] : &BootFlowScreens[_countof(BootFlowScreens) - 1];

                uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("74 ? 85 C0 0F 95 C1").get_first(0));
                uintptr_t loc_4DE273 = reinterpret_cast<uintptr_t>(hook::pattern("BF ? ? ? ? 6A ? E8").get_first(0));
                uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("81 FF ? ? ? ? 89 46 ? 89 48 ? ? ? 7C ? ? ? 85 F6").get_first(0));

                // force US Retail bootflow
                injector::MakeJMP(loc_4DE21A, loc_4DE273);
                injector::WriteMemory<const char**>(loc_4DE273 + 1, outScreens, true);
                injector::WriteMemory<const char**>(loc_4DE2A0 + 2, outLastScreen, true);

                // skip the intro video the first time it plays
                auto pattern = hook::pattern("8B 44 24 ? 3D ? ? ? ? 56 74");
                IntroFMVScreen::shNotificationMessage = safetyhook::create_inline(pattern.get_first(0), IntroFMVScreen::NotificationMessage);

                // Continue past the title screen straight to the main menu
                onMenuScreenBaseNotify() += [](MenuScreen* menu)
                {
                    static bool bDone = false;
                    const char* package = menu->PackageFilename;
                    if (bDone || !package)
                        return;

                    // Overlays (loading, the song's name, the menu bar) and the intro movie don't change anything
                    for (auto overlay : { "PC_Loading.fng", "LS_IntroFMV.fng", "Chyron.fng", "PC_MenuBar.fng" })
                    {
                        if (_stricmp(package, overlay) == 0)
                            return;
                    }

                    // The title screen asks for Enter twice; the profile screen gets one press, which loads the last
                    // profile. The title screen only takes Enter once its animation has finished; leaving it earlier, by a
                    // key press or by sending it the accept message, stops the game from reading the keyboard.
                    static std::string currentScreen;
                    bool profileScreen = _stricmp(package, "MU_UG_NEwOrLoad_PC2.fng") == 0;
                    bool titleScreen = _stricmp(package, "LS_Splash_PC.fng") == 0 || profileScreen;
                    if (currentScreen != package)
                    {
                        currentScreen = package;
                        Input::SetAutoAccept(false);
                    }
                    Input::SetAutoAccept(titleScreen, profileScreen ? 1 : -1);
                    if (!titleScreen)
                        bDone = true;
                };
            }
            else if (bShowLangSelect)
            {
                static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_EAlogo.fng", "LS_PSAMovie.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };

                uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("89 36 89 76 04 A1 ? ? ? ? 83 F8 05").get_first(0)) + 0x15;
                uintptr_t loc_4DE273 = loc_4DE21A + 0x59;
                uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36").get_first(0)) + 8;

                injector::MakeJMP(loc_4DE21A, loc_4DE273);
                injector::WriteMemory<const char**>(loc_4DE273 + 1, BootFlowScreensLangSelect, true);
                injector::WriteMemory<const char**>(loc_4DE2A0 + 2, &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1], true);
            }

            // 44.1 kHz audio instead of 22 kHz
            {
                auto pattern = hook::pattern("C7 05 ? ? ? ? 22 56 00 00 A1 ? ? ? ? 85 C0");
                *(uint32_t*)*pattern.get_first<uint32_t*>(11) = 44100;
            }

            // __mbclen to strlen, the controls menu showed "---" for multi-character names
            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
            if (!pattern.empty())
                injector::MakeCALL(pattern.get_first(0), strlen, true);

            // The steering update snaps the steering angle to zero while it's below 1 degree. A frame's change
            // of the angle is smaller than that at high frame rates, so the wheels stick in the centre and a
            // quick change of direction is lost; at 60 FPS it already happens with fast input. Skip the snap.
            pattern = hook::pattern("D9 54 24 14 D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05 7A 08");
            injector::WriteMemory<uint8_t>(pattern.get_first(0x11), 0xEB, true);

            // The best lap counter at the end of a race isn't visible in 16:9 and gets in the way on ultrawide
            auto loc_49A222 = hook::pattern("8A 44 24 24 8B 6C 24 68 8B 7D 0C 83 CE FF 84 C0 8D 4C 24 24 74 ? 6B F6 21");
            pattern = hook::pattern("0F 84 B8 00 00 00 8D 4C 24 44");
            injector::MakeNOP(pattern.get_first(0), 6, true);
            injector::MakeJMP(pattern.get_first(0), loc_49A222.get_first(0), true);

            // Frame rate limit: 0 = unlimited, -1 = monitor refresh rate, -2 = twice the refresh rate
            if (nFPSLimit == -1)
                nFPSLimit = GetDesktopRefreshRate();
            else if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else if (nFPSLimit < 0)
                nFPSLimit = 60;

            FrameRate::fTargetFrameTime = nFPSLimit > 0 ? 1.0f / static_cast<float>(nFPSLimit) : 0.0f;
            FrameRate::fMinFrameTime = std::min(1.0f / 60.0f, nFPSLimit > 0 ? FrameRate::fTargetFrameTime : 1.0f / 1000.0f);

            pattern = hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00");
            static ProtectedGameRef<float> TargetFrameTime;
            TargetFrameTime.SetAddress(*pattern.get_first<float*>(3));
            TargetFrameTime = FrameRate::fTargetFrameTime;

            // Busy wait only for the last part of the frame
            static ProtectedGameRef<float> WaitThreshold;
            WaitThreshold.SetAddress(*pattern.get_first<float*>(63));
            WaitThreshold = std::max(FrameRate::fTargetFrameTime * 2.0f, 1.0f / 30.0f);

            // Minimum time step of a frame
            pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 0C D8 15 ? ? ? ? DF E0 F6 C4 41 74 08 DD D8 D9 05");
            injector::WriteMemory(pattern.get_first(12), &FrameRate::fMinFrameTime, true);
            injector::WriteMemory(pattern.get_first(27), &FrameRate::fMinFrameTime, true);

            // Don't add a tick when less than 1/60 s has passed; the remainder carries over to the next frame
            pattern = hook::pattern("D9 1D ? ? ? ? DD D8 75 05 B8 01 00 00 00");
            injector::WriteMemory<uint8_t>(pattern.get_first(8), 0xEB, true);

            // EAXSound::Update(RealTimeElapsed) in MainLoop
            pattern = hook::pattern("52 E8 ? ? ? ? 8B 2D ? ? ? ? BF");
            FrameRate::EAXSoundUpdate = injector::GetBranchDestination(pattern.get_first(1)).get();
            injector::MakeCALL(pattern.get_first(1), FrameRate::EAXSoundUpdateHook, true);

            // Particle effects are updated once per frame with a fixed 1/60 s step; use the frame's real time step
            pattern = hook::pattern("68 89 88 88 3C 68 ? ? ? ? E8");
            static auto ParticleTimeStepHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
            {
                *reinterpret_cast<float*>(regs.esp + 4) = std::clamp(RealTimeElapsed.get(), 0.0f, 0.05f);
            });
        };
    }
} Misc;
