module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;

// Frame rate. PrepareRealTimestep waits until the target frame time has passed and uses it as the
// smallest time step of a frame. The game counts time in 1/60 s ticks and adds at least one tick per
// frame, so above 60 FPS tick-based gameplay ran too fast.
namespace FrameRate
{
    float fTargetFrameTime = 1.0f / 60.0f;
}

// The in-race special cameras (camera mover types 13 to 16 of the player's ChangeCameraMover, the jump and
// slow motion shots) are TrackCarCameraMovers. Their Update (sub_44C350) puts the camera at a point on a
// unit vector from the car plus an offset that a damped spring moves (CalcSpringForce, sub_444CF0):
//     force = K * offset + C * velocity;  velocity -= force;  offset += velocity
// with K and C from the camera data (+0x60, +0x64), offset at +0x50 and velocity at +0x40, once per Update,
// that is once per frame. The game runs gameplay at up to 60 FPS, so above that the spring settled faster
// and the shots looked different. The spring now steps 60 times per second of real time, and the camera
// gets the offset interpolated between the last two steps, so it still moves smoothly every frame.
namespace SpringCamera
{
    constexpr float StepTime = 1.0f / 60.0f;
    float* FrameTimeStep = nullptr;

    struct Spring
    {
        float x[3], v[3];       // the spring's own state
        float prevX[3];         // offset after the step before
        float written[3];       // what was handed to the camera, to notice a new mover at the same address
        float time;             // time since the last step
    };
    std::unordered_map<uintptr_t, Spring> Springs;

    void Update(uintptr_t mover)
    {
        auto offset = reinterpret_cast<float*>(mover + 0x50);
        auto velocity = reinterpret_cast<float*>(mover + 0x40);
        float K = *reinterpret_cast<float*>(mover + 0x60);
        float C = *reinterpret_cast<float*>(mover + 0x64);

        auto [it, inserted] = Springs.try_emplace(mover);
        auto& s = it->second;
        if (inserted || memcmp(s.written, offset, sizeof(s.written)) != 0)
        {
            // A new camera: start from the state its constructor set, stepping at once like the game did
            std::copy_n(offset, 3, s.x);
            std::copy_n(velocity, 3, s.v);
            std::copy_n(offset, 3, s.prevX);
            s.time = StepTime;
        }
        else
            s.time += std::clamp(*FrameTimeStep, 0.0f, 0.1f);

        for (int steps = 0; s.time >= StepTime && steps < 8; ++steps)
        {
            s.time -= StepTime;
            std::copy_n(s.x, 3, s.prevX);
            for (int i = 0; i < 3; ++i)
            {
                s.v[i] -= K * s.x[i] + C * s.v[i];
                s.x[i] += s.v[i];
            }
        }
        s.time = std::min(s.time, StepTime);

        float blend = s.time / StepTime;
        for (int i = 0; i < 3; ++i)
        {
            offset[i] = s.prevX[i] + (s.x[i] - s.prevX[i]) * blend;
            velocity[i] = s.v[i];
        }
        std::copy_n(offset, 3, s.written);
    }
}

// Audio at high frame rates. EAXSound::Update (called once per frame from MainLoop with the frame's time
// step, and with 0 from the window procedure when the window loses focus) returns at once unless more
// than 15 ms of real time passed since its last update (the QPC ticker it keeps at +0xA4). When it runs
// it only gets the time step of the current frame, so above ~66 FPS the time of the skipped frames was
// lost: engine and gear change sounds, which are driven by this time, played too slowly and stretched.
// The time of frames the 15 ms check skips is kept and handed over with the next update that runs.
// When the update is skipped for another reason (sound off, or the game's loading flag) or the window
// procedure stops the sound, the kept time is dropped, as the game itself doesn't advance the sound then.
namespace AudioTime
{
    constexpr size_t LastUpdateTicker = 0xA4;

    int32_t* SoundEnabled = nullptr;    // checked first by EAXSound::Update
    int32_t* SoundSuspended = nullptr;  // set while loading, EAXSound::Update then only pauses the music
    float fPendingTime = 0.0f;

    SafetyHookInline shEAXSoundUpdate = {};
    void __fastcall EAXSoundUpdate(uintptr_t EAXSound, void* edx, float timeStep)
    {
        if (timeStep <= 0.0f)
        {
            fPendingTime = 0.0f;
            return shEAXSoundUpdate.unsafe_fastcall(EAXSound, edx, timeStep);
        }

        float total = fPendingTime + timeStep;
        auto lastUpdate = *reinterpret_cast<volatile uint32_t*>(EAXSound + LastUpdateTicker);
        shEAXSoundUpdate.unsafe_fastcall(EAXSound, edx, total);

        bool bUpdated = *reinterpret_cast<volatile uint32_t*>(EAXSound + LastUpdateTicker) != lastUpdate;
        bool bHeldBack = !bUpdated && *SoundEnabled && !*SoundSuspended;
        fPendingTime = bHeldBack ? std::min(total, 0.1f) : 0.0f;
    }
}

// The game's memory allocator is not thread safe. MemoryPool::AllocateMemory and MemoryPool::FreeMemory
// take the pool's critical section for the free list, but bWareMalloc links the new block into the
// pool's list of allocations (pool +0x0C/+0x10) after AllocateMemory released it, and bFree unlinks the
// block before FreeMemory takes it; bFree also destroys the main pool without it when the pool's last
// block is freed, and AllocateMemory grows the pool (pool +0x14/+0x18/+0x24) after leaving it. The pools
// fill new blocks with 0xAA and freed memory with 0xEE.
// Other threads do allocate from the main pool: RealCore's file system gets the game's allocator
// (bWareMalloc/bFree through the object set up in the startup code), and the completion callbacks of
// its async files, which run on the RealFile I/O thread, allocate the "ASYNCFILE BUF" read buffers
// (the movie player streams through these), and the jobs free them there. DirtySock's memory functions
// are bMalloc/bFree as well and its NetIdle and DNS threads free sockets and lookups once networking was
// started (LAN/online menus). When such a thread and the main thread both link or unlink a block at the
// same time, the allocation list keeps a pointer to a block that is freed and reused later; the next
// bFree of a neighbour then writes into that block's new owner or follows a 0xAAAAAAAA link and crashes.
// On one core the threads rarely interleave inside these few instructions, which is why pinning the
// game's threads to one core hid the crash. bWareMalloc and bFree run under one lock now.
namespace GameAllocator
{
    CRITICAL_SECTION Lock;

    SafetyHookInline shWareMalloc = {};
    void* __cdecl WareMalloc(int size, const char* name, int line, int flags)
    {
        EnterCriticalSection(&Lock);
        auto result = shWareMalloc.unsafe_ccall<void*>(size, name, line, flags);
        LeaveCriticalSection(&Lock);
        return result;
    }

    SafetyHookInline shFree = {};
    void __cdecl Free(void* block)
    {
        EnterCriticalSection(&Lock);
        shFree.unsafe_ccall(block);
        LeaveCriticalSection(&Lock);
    }
}

// RealCore's "bf:" device, the file system inside the game's archives (big_vfs.cpp), keeps a pool of
// file entries: a free list (+0x3C) and a list of open files (+0x48). Open takes an entry from the free
// list and adds it to the open files and Close moves it back, neither under a lock. Open runs on the
// thread that opens the file (the main thread or one of the game's loading threads), while Close runs
// where the file is closed, often the "bf:" device's own I/O thread finishing a request. When both change
// the lists at once an entry gets lost or ends up in both lists; lost entries add up until the pool is
// empty, and then the next open either fails, which leaves a loading screen waiting for its file forever,
// or writes through the NULL it got (#2330). It happens on one core too, only less often, as the threads
// can still switch inside these functions. Open and Close run under one lock now; both only search and
// relink entries in memory. Mount and Unmount (the archive list) stay as they are: they read and close
// the archive through other I/O threads and wait for them, and they only run on the main thread.
namespace ArchiveFiles
{
    CRITICAL_SECTION Lock;

    SafetyHookInline shOpen = {};
    int __fastcall Open(void* device, void* edx, const char* path, int flags, int* archive)
    {
        EnterCriticalSection(&Lock);
        auto result = shOpen.unsafe_thiscall<int>(device, path, flags, archive);
        LeaveCriticalSection(&Lock);
        return result;
    }

    SafetyHookInline shClose = {};
    void* __fastcall Close(void* device, void* edx, int file)
    {
        EnterCriticalSection(&Lock);
        auto result = shClose.unsafe_thiscall<void*>(device, file);
        LeaveCriticalSection(&Lock);
        return result;
    }
}

// The title screen ("Press Enter Key") continues by itself, straight to the main menu
namespace SplashScreen
{
    constexpr uint32_t MessageTick = 0xC98356BA;
    constexpr uint32_t MessageAccept = 0x406415E3;

    uint8_t* bAllowContinue = nullptr;
    SafetyHookInline shNotificationMessage = {};
    void __fastcall NotificationMessage(uintptr_t screen, void* edx, uint32_t message, void* object, uint32_t param1, uint32_t param2)
    {
        shNotificationMessage.unsafe_fastcall(screen, edx, message, object, param1, param2);

        // Once the screen has started (the time it started is at +0x4C), accept it
        static bool bDone = false;
        uint32_t startTime = *reinterpret_cast<uint32_t*>(screen + 0x4C);
        if (!bDone && message == MessageTick && startTime != 0 && startTime != 0x7FFFFFFF)
        {
            bDone = true;
            *bAllowContinue = 1;
            shNotificationMessage.unsafe_fastcall(screen, edx, MessageAccept, object, 0, 0);
        }
    }
}

// Movie subtitles. The game shows them by its build region (dword_864F24), not by its language: for Dutch,
// Swedish and Danish in the region of those releases, Korean, Chinese (Taiwan) and Thai. The PC release has the
// US region, so no language got them, although every language file has the subtitles' texts (SUBTITLES\<movie>
// lists the text hashes). Changing the region isn't an option, it also picks the language files, the regional
// cars and the online region. The check (sub_50D350, and inline in InGamePlayMovie's constructor) now uses
// the "Subtitles" setting.
namespace Subtitles
{
    const int* Setting = nullptr;

    bool Show()
    {
        return *Setting != 0;
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
            int nFPSLimit = FixSettings().Int("MAIN.FPSLimit");

            if (bSkipIntro)
            {
                static constexpr auto SkipThis = "SkipThis";
                auto pattern = hook::pattern("8B F1 50 89 74 24 08 E8 ? ? ? ? 8B 46 04 68 ? ? ? ? 68 ? ? ? ? 50");
                injector::WriteMemory(pattern.count(3).get(0).get<uint32_t>(16), &SkipThis, true); // FMV Opening
                injector::WriteMemory(pattern.count(3).get(1).get<uint32_t>(16), &SkipThis, true); // EA Bumper

                // THX
                injector::WriteMemory(hook::pattern("56 57 8B F1 50 89 74 24 0C E8 ? ? ? ? 68 ? ? ? ? 33 C0").count(1).get(0).get<uint32_t>(15), &SkipThis, true);

                // PSA
                injector::WriteMemory(hook::pattern("C7 06 ? ? ? ? E8 ? ? ? ? 8B 46 04 68 ? ? ? ? 68 ? ? ? ? 50").count(1).get(0).get<uint32_t>(15), &SkipThis, true);

                pattern = hook::pattern("8B 44 24 04 3D 61 24 AF B5 56 8B F1 77 5F");
                if (!pattern.empty())
                {
                    SplashScreen::bAllowContinue = *pattern.get_first<uint8_t*>(0x23);
                    SplashScreen::shNotificationMessage = safetyhook::create_inline(pattern.get_first(), SplashScreen::NotificationMessage);
                }
            }

            // Frame rate limit: 0 = unlimited, -1 = monitor refresh rate, -2 = twice the refresh rate
            if (nFPSLimit == -1)
                nFPSLimit = GetDesktopRefreshRate();
            else if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else if (nFPSLimit < 0)
                nFPSLimit = 60;

            FrameRate::fTargetFrameTime = nFPSLimit > 0 ? 1.0f / static_cast<float>(nFPSLimit) : 1.0f / 1000.0f;

            // Default frame time target, restored by the game after cutscenes
            auto pattern = hook::pattern("D9 05 ? ? ? ? D8 74 ? ? D9 1D ? ? ? ? C3");
            static ProtectedGameRef<float> DefaultFrameTime;
            DefaultFrameTime.SetAddress(*pattern.get_first<float*>(33));
            DefaultFrameTime = FrameRate::fTargetFrameTime;

            // Current frame time target
            pattern = hook::pattern("D9 05 ? ? ? ? B9 64 00 00 00 D8 64");
            static ProtectedGameRef<float> CurrentFrameTime;
            CurrentFrameTime.SetAddress(*pattern.get_first<float*>(2));
            CurrentFrameTime = FrameRate::fTargetFrameTime;

            // In-engine cutscenes (NIS: the race start with the flag girl, the race end, shop and career scenes) switch
            // to 30 FPS when they start (sub_435F40 -> sub_576CB0(30)), so their animated cars and cameras were only
            // ever run at 30 FPS. Above 60 FPS the race start scene goes wrong: the camera path wobbles and the
            // shot ends low and close over the player's car. Allow up to 60 FPS, a multiple of 30, as before.
            pattern = hook::pattern("7C ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F 5E");
            static ProtectedGameRef<float> CutsceneFPS;
            CutsceneFPS.SetAddress(pattern.get_first<float>(3));
            {
                int fps = nFPSLimit > 0 ? std::min(nFPSLimit, 60) : 60;
                CutsceneFPS = static_cast<float>(std::max(30, fps - fps % 30));
            }

            // The world of a LAN/online race (the one with the "ExtrapolatedStateSlotPool" for the other players'
            // cars) also switches to 30 FPS. That rate is part of the network play, so it's raised to 60 at most,
            // as in the earlier versions of the fix, rather than to the full limit.
            pattern = hook::pattern("8B 15 ? ? ? ? 52 A3 ? ? ? ? E8 ? ? ? ? 83 C4 14");
            static ProtectedGameRef<float> NetworkRaceFPS;
            NetworkRaceFPS.SetAddress(*pattern.get_first<float*>(2));
            {
                int fps = nFPSLimit > 0 ? std::min(nFPSLimit, 60) : 60;
                NetworkRaceFPS = static_cast<float>(std::max(30, fps - fps % 30));
            }

            // Don't add a tick when less than 1/60 s has passed; the remainder carries over to the next frame
            pattern = hook::pattern("D9 1D ? ? ? ? DD D8 75 05 B8 01 00 00 00");
            injector::WriteMemory<uint8_t>(pattern.get_first(8), 0xEB, true);

            // Steering ignores the opposite direction until the wheels are centred again. The original game has
            // this bug too; higher frame rates and faster input make it more noticeable. Skip the auto-centring check.
            pattern = hook::pattern("DD D8 D9 44 24 18 D9 54 24 1C D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05");
            injector::WriteMemory<uint8_t>(pattern.get_first(0x17), 0xEB, true);

            // EAXSound::Update
            pattern = hook::pattern("51 A1 ? ? ? ? 85 C0 53 8B D9 0F 84 ? ? ? ? A1 ? ? ? ? 85 C0 56 74");
            AudioTime::SoundEnabled = *pattern.get_first<int32_t*>(2);
            AudioTime::SoundSuspended = *pattern.get_first<int32_t*>(0x12);
            AudioTime::shEAXSoundUpdate = safetyhook::create_inline(pattern.get_first(), AudioTime::EAXSoundUpdate);

            // Particle effects are updated once per frame with a fixed 1/60 s step; use the frame's real time step
            static float* RealTimeElapsed = *hook::get_pattern<float*>("D9 05 ? ? ? ? D8 05 ? ? ? ? D9 05 ? ? ? ? D8 C9 E8 ? ? ? ? 85 C0", 2);

            // TrackCarCameraMover::Update, the spring step: lea edx,[esi+40h] ... up to the code after it, which
            // expects ebx = esi+0x50 and ecx = esp+0xB0 (the lea at +0x36)
            SpringCamera::FrameTimeStep = RealTimeElapsed;
            pattern = hook::pattern("8D 56 40 52 8D 5E 50 53 8D 4C 24 38 51 8B CE E8 ? ? ? ? D9 02 D8 64 24 30");
            if (!pattern.empty() && memcmp(pattern.get_first(0x36), "\x8D\x8C\x24\xB0\x00\x00\x00", 7) == 0)
            {
                static uintptr_t SpringStepEnd = reinterpret_cast<uintptr_t>(pattern.get_first(0x51));
                static auto SpringStepHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    SpringCamera::Update(regs.esi);
                    regs.ebx = regs.esi + 0x50;
                    regs.ecx = regs.esp + 0xB0;
                    regs.eip = SpringStepEnd;
                });
            }
            pattern = hook::pattern("68 89 88 88 3C B9 ? ? ? ? E8");
            static auto ParticleTimeStepHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
            {
                *reinterpret_cast<float*>(regs.esp) = std::clamp(*RealTimeElapsed, 0.0f, 0.05f);
            });

            // The allocator, see GameAllocator. This runs before the game creates its threads.
            InitializeCriticalSectionAndSpinCount(&GameAllocator::Lock, 4000);
            pattern = hook::pattern("51 53 55 57 8B 7C 24 18 85 FF 74 ? 68 ? ? ? ? 57 E8"); // bWareMalloc
            GameAllocator::shWareMalloc = safetyhook::create_inline(pattern.get_first(), GameAllocator::WareMalloc);
            pattern = hook::pattern("57 8B 7C 24 08 85 FF 0F 84 ? ? ? ? BA ? ? ? ? 56 8B 02 85 C0"); // bFree
            GameAllocator::shFree = safetyhook::create_inline(pattern.get_first(), GameAllocator::Free);

            // The archives' file system, see ArchiveFiles
            InitializeCriticalSectionAndSpinCount(&ArchiveFiles::Lock, 4000);
            auto bfOpen = hook::pattern("55 8B EC 81 EC 18 01 00 00 53 56 8B 75 08 57 6A 03 33 DB 68");
            auto bfClose = hook::pattern("55 8B EC 56 8B 75 08 57 8B F9 C1 E6 05 03 77 1C 6A 00 56 8D 4F 48 E8");
            if (!bfOpen.empty() && !bfClose.empty())
            {
                ArchiveFiles::shOpen = safetyhook::create_inline(bfOpen.get_first(), ArchiveFiles::Open);
                ArchiveFiles::shClose = safetyhook::create_inline(bfClose.get_first(), ArchiveFiles::Close);
            }

            // Movie subtitles, see Subtitles
            Subtitles::Setting = &FixSettings().Int("MAIN.Subtitles");
            pattern = hook::pattern("E8 ? ? ? ? 83 F8 05 75 0C A1 ? ? ? ? 85 C0 74 03 B0 01 C3");
            auto inlineCheck = hook::pattern("E8 ? ? ? ? 83 F8 05 75 08 39 1D ? ? ? ? 75 26 E8 ? ? ? ? 83 F8 02 75 08 39 1D ? ? ? ? 75 14 E8 ? ? ? ? 83 F8 04 74 0A E8 ? ? ? ? 83 F8 0C 75 0E");
            if (!pattern.empty() && !inlineCheck.empty())
            {
                static auto shShowSubtitles = safetyhook::create_inline(pattern.get_first(), static_cast<bool(*)()>(Subtitles::Show));

                static auto showExit = reinterpret_cast<uintptr_t>(inlineCheck.get_first(0x38));
                static auto skipExit = reinterpret_cast<uintptr_t>(inlineCheck.get_first(0x46));
                static auto shInlineCheck = safetyhook::create_mid(inlineCheck.get_first(), [](SafetyHookContext& regs)
                {
                    regs.eip = Subtitles::Show() ? showExit : skipExit;
                });
            }

            // Run without the disc in an optical drive
            pattern = hook::pattern("56 8B 35 ? ? ? ? A1 ? ? ? ? 85 C0 ? ? E8");
            injector::WriteMemory<uint8_t>(pattern.get_first(15), 0x78, true);
            injector::WriteMemory<uint8_t>(pattern.get_first(36), 0x00, true);
        };
    }
} Misc;
