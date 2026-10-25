module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import ComVars;

namespace
{
    constexpr uint32_t MessageMovieDone = 0xC3960EB9;
}

// The title screen ("Press START to begin") continues by itself, straight to the main menu
namespace SplashScreen
{
    constexpr uint32_t MessageTick = 0xC98356BA;
    constexpr uint32_t MessageAccept = 0x406415E3;

    bool bReached = false;  // the boot movies are over

    SafetyHookInline shNotificationMessage = {};
    void __fastcall NotificationMessage(uintptr_t screen, void* edx, uint32_t message, void* object, uint32_t param1, uint32_t param2)
    {
        bReached = true;
        shNotificationMessage.unsafe_fastcall(screen, edx, message, object, param1, param2);

        // The screen accepts input once it has faded in (+0x2C)
        static bool bDone = false;
        if (!bDone && message == MessageTick && *reinterpret_cast<uint8_t*>(screen + 0x2C))
        {
            bDone = true;
            shNotificationMessage.unsafe_fastcall(screen, edx, MessageAccept, object, 0, 0);
        }
    }
}

// The boot movies (EA logo, intro) before the title screen. The movie screen starts a movie when it gets the
// "start movie" event (type 7) and the boot sequence goes on to its next screen when the movie screen gets "movie
// done". The movies are skipped by answering the event with a single "movie done" and not starting the movie.
// Setting the game's own skip flag instead (as before) sent "done" and then still looked the movie file up: a
// missing file (installs without movies) sent a second "done", the boot sequence advanced twice per movie, ended
// early, and the next "done" crashed on the sequence that was already gone.
namespace IntroMovies
{
    void*** FEngInstance = nullptr;   // cFEng::mInstance, which points to the cFEng
    void* QueueMessage = nullptr;     // thiscall (cFEng*, hash, object, package, -1, 0)

    SafetyHookInline shStartMovie = {};
    char __fastcall StartMovie(uintptr_t screen, void* edx, uintptr_t event)
    {
        if (!SplashScreen::bReached && *reinterpret_cast<int32_t*>(event + 0x18) == 7 && FEngInstance && *FEngInstance && **FEngInstance)
        {
            reinterpret_cast<void(__thiscall*)(void*, uint32_t, void*, uint32_t, int32_t, uint32_t)>(QueueMessage)
                (**FEngInstance, MessageMovieDone, nullptr, *reinterpret_cast<uint32_t*>(screen + 4), -1, 0);
            return 0;
        }
        return shStartMovie.unsafe_fastcall<char>(screen, edx, event);
    }

    // Goes to the next screen of the boot sequence. The game deletes the sequence after its last screen, and a
    // late "movie done" or a button press on the last movie called this without one.
    SafetyHookInline shNextScreen = {};
    int __fastcall NextScreen(void* sequence, void* edx, int fade)
    {
        if (!sequence)
            return 0;
        return shNextScreen.unsafe_fastcall<int>(sequence, edx, fade);
    }
}

// Frame rate. The game simulates in fixed steps and draws one frame per step: it waits until a step's
// time has passed, then runs the steps due. The step is 1/60 s, which capped the game at 60 FPS. The
// step is set to the frame rate limit, so the game runs smoothly at any refresh rate.
namespace FrameRate
{
    float fStepTime = 1.0f / 60.0f;

    LARGE_INTEGER Frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LONGLONG NextFrame = 0;     // when the next frame is due

    LONGLONG Now()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return now.QuadPart;
    }

    // Waits until the given time, with a high resolution timer and the last part with a busy wait
    void WaitUntil(LONGLONG target)
    {
        static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        double seconds = double(target - Now()) / double(Frequency.QuadPart);
        constexpr double SpinTime = 0.0015;
        if (seconds > SpinTime)
        {
            if (timer)
            {
                LARGE_INTEGER due;
                due.QuadPart = -static_cast<LONGLONG>((seconds - SpinTime) * 10000000.0);
                if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
            }
            else
                Sleep(static_cast<DWORD>((seconds - SpinTime) * 1000.0));
        }

        while (Now() < target)
            YieldProcessor();
    }

    // Frames start at a fixed rate, on the real clock. A frame that is late by less than a frame keeps the
    // rate (the next one comes sooner); after a longer stall (loading, the window in the background) the
    // count starts over.
    void WaitForFrame()
    {
        LONGLONG period = static_cast<LONGLONG>(double(Frequency.QuadPart) * fStepTime + 0.5);
        LONGLONG now = Now();
        if (NextFrame == 0 || now - NextFrame > period)
            NextFrame = now;
        else if (now < NextFrame)
            WaitUntil(NextFrame);
        NextFrame += period;
    }

    // Sim::Service: waits for the frame and runs the simulation steps due. The game slept for whole
    // milliseconds and measured with the time of the previous frames (the game's clock advances at the end
    // of a frame), so frames came early and late and got 0, 1 or 2 steps: the game judders above 60 FPS.
    // This waits on the real clock, and the steps are the game's elapsed time in steps, rounded to the
    // nearest step with the rest carried over: a frame on time always gets one step, and the simulation
    // still follows the game's clock over time.
    void* GetTime = nullptr;                                        // thiscall (int32_t* ticks), 4000 ticks per second
    void* RunSteps = nullptr;                                       // thiscall (unsigned steps), at most 6 steps

    double __fastcall SimService(uintptr_t sim, void*, uint8_t forceSteps)
    {
        auto& lastTicks = *reinterpret_cast<int32_t*>(sim + 0x14);
        auto& totalTime = *reinterpret_cast<float*>(sim + 0x18);
        auto& fraction = *reinterpret_cast<float*>(sim + 0x1C);     // steps carried over, -0.5 to 0.5
        float step = *reinterpret_cast<float*>(sim + 0x20);

        int32_t ticks = 0;
        reinterpret_cast<void(__thiscall*)(uintptr_t, int32_t*)>(GetTime)(sim, &ticks);
        float elapsed = static_cast<float>(ticks - lastTicks) * 0.00025f;
        lastTicks = ticks;
        if (elapsed < 0.0f)
            return 0.0;

        if (!forceSteps)
            WaitForFrame();

        totalTime += elapsed;
        float due = elapsed / step + fraction;
        uint32_t steps = due > 0.5f ? static_cast<uint32_t>(due + 0.5f) : 0;
        fraction = due - static_cast<float>(steps);
        if (forceSteps)
        {
            steps = 4;
            fraction = 0.0f;
        }

        reinterpret_cast<void(__thiscall*)(uintptr_t, uint32_t)>(RunSteps)(sim, steps);
        return static_cast<double>(steps) * step;
    }
}

// Front end timers. The front end (menus and HUD animations) and the memory card screens get the frame time in
// whole ticks (1/1200 s and 1/1000 s), cut off each frame: at 144 FPS that's 8 instead of 8.33 ticks, so they ran
// 4% and 14% slow. The cut off part is carried over to the next frame.
namespace FrontEndTime
{
    GameRef<float> RealFrameTime;   // the frame time of PrepareRealTimestep

    uint32_t Ticks(double& carry, double ticksPerSecond)
    {
        double ticks = static_cast<double>(RealFrameTime) * ticksPerSecond + carry;
        if (ticks < 0.0)
            ticks = 0.0;
        double whole = std::floor(ticks);
        carry = ticks - whole;
        return static_cast<uint32_t>(whole);
    }
}

// Bugs of the game
namespace GameBugs
{
    // The simulation time (Sim::GetTime) is a float that gets the step added every step, and it's only reset when
    // a world is loaded: in free roam it grows for hours. Adding a step of a few milliseconds to a big float
    // rounds the step to the float's precision, always the same way, so the time ran slow (the "timebug": race
    // times, timers and anything timed with the simulation drift after a long session). At 60 steps per second
    // it's 0.4% slow after 2.3 hours and 6% after 9 hours; at 240 steps per second 6% already after 2.3 hours.
    // The time is now counted in double precision and stored rounded, so it never drifts; differences of two
    // times are exact to the float's precision.
    float* pSimTime = nullptr;

    void OnSimStep(float step)
    {
        static double exact = 0.0;
        static float written = 0.0f;

        // The game added the step in float: anything else means the time was reset or set in between
        if (*pSimTime != written + step)
            exact = *pSimTime;
        else
        {
            exact += step;
            *pSimTime = static_cast<float>(exact);
        }
        written = *pSimTime;
    }

    // Attrib::StringToLowerCaseKey: the key of a name, 0 for an empty name. Some callers pass names they looked
    // up, which can be missing (null), and it read the null string.
    SafetyHookInline shStringToLowerCaseKey = {};
    uint32_t __cdecl StringToLowerCaseKey(const char* name)
    {
        if (!name)
            return 0;
        return shStringToLowerCaseKey.unsafe_ccall<uint32_t>(name);
    }

    // The front end's listeners are told about package changes from a vector the listeners themselves leave
    // while they're told (cop and roadblock HUD parts going away): the loop went on past the shortened vector and
    // called a null or stale entry. They're now told from a copy, each only while it's still listening.
    void*** Listeners = nullptr;
    int32_t* ListenerCount = nullptr;

    SafetyHookInline shNotifyListeners = {};
    void __cdecl NotifyListeners()
    {
        std::vector<void*> listeners(*Listeners, *Listeners + *ListenerCount);
        for (auto listener : listeners)
        {
            if (!listener || std::find(*Listeners, *Listeners + *ListenerCount, listener) == *Listeners + *ListenerCount)
                continue;
            reinterpret_cast<void(__thiscall*)(void*)>((*reinterpret_cast<void***>(listener))[3])(listener);
        }
    }
}

// The paint compositor's marker pixels, for each skin size (see the skin slots below)
namespace SkinCompositor
{
    int32_t* Built = nullptr;       // the markers were found
    int32_t* Table = nullptr;       // 4 colours x 16 pixel indices
    int32_t* Counts = nullptr;      // markers found per colour

    struct Markers
    {
        int32_t table[64];
        int32_t counts[4];
    };
    std::map<uint32_t, Markers> BySize;
    uint32_t CurrentSize = 0;       // the skin size the game's table is for

    // The name of a car's vinyl texture; mode 2 is the precomposited one ("<CAR>_PRECOM_<vinyl>"). The PC's
    // PREVINYL.BIN files are stubs with two small placeholder textures, which the precomposited copy would read
    // a whole 256x256 skin from: on the PC a precomposited vinyl doesn't exist, the skin gets the paint only.
    SafetyHookInline shVinylName = {};
    uint32_t __cdecl VinylName(void* vinyl, int carType, int mode)
    {
        if (mode == 2)
            return 0;
        return shVinylName.unsafe_ccall<uint32_t>(vinyl, carType, mode);
    }

    // RideInfo::SetCompositeNameHash(slot): the skin, wheel and spinner textures of the slot (DUMMY_SKIN<n>,
    // DUMMY_WHEEL<n>, DUMMY_SPINNER<n>). The rims of a car with a composited skin are drawn with DUMMY_WHEEL<n>,
    // its own rim texture with the rim paint. Only DUMMY_WHEEL1-4 exist, so cars in slots 5-12 had no rims at all;
    // they keep their own rim texture instead (the replacement is left out, as for cars without a skin).
    SafetyHookInline shSetSkinSlot = {};
    void __fastcall SetSkinSlot(uint8_t* rideInfo, void*, int slot)
    {
        shSetSkinSlot.unsafe_thiscall(rideInfo, slot);
        if (slot > 4)
            *reinterpret_cast<uint32_t*>(rideInfo + 64) = 0;
    }

    SafetyHookInline shPaint = {};
    int __cdecl Paint(int32_t* params)
    {
        auto texture = reinterpret_cast<uint8_t*>(params[0]);
        if (!texture)
            return shPaint.unsafe_ccall<int>(params);

        uint32_t size = (uint32_t(*reinterpret_cast<uint16_t*>(texture + 68)) << 16) | *reinterpret_cast<uint16_t*>(texture + 70);
        if (size != CurrentSize)
        {
            // Keep the table of the previous size, and take this size's if it was found before
            if (*Built && CurrentSize)
            {
                auto& saved = BySize[CurrentSize];
                std::copy_n(Table, 64, saved.table);
                std::copy_n(Counts, 4, saved.counts);
            }
            if (auto it = BySize.find(size); it != BySize.end())
            {
                std::copy_n(it->second.table, 64, Table);
                std::copy_n(it->second.counts, 4, Counts);
                *Built = 1;
            }
            else
            {
                std::fill_n(Counts, 4, 0);
                *Built = 0;
            }
            CurrentSize = size;
        }
        return shPaint.unsafe_ccall<int>(params);
    }
}

// Movie subtitles. The movie screens (the story movies, the tutorials and the PSA) load the subtitles when the
// game's language isn't English or the movie asks for them, so English never had them for the story movies,
// although English.bin has their texts. The language check now uses the "Subtitles" setting: On shows them in
// every language, Off only for the movies that ask for them.
namespace Subtitles
{
    const int* Setting = nullptr;

    int Show()
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

            // Movie subtitles, see Subtitles
            Subtitles::Setting = &FixSettings().Int("MAIN.Subtitles");
            for (auto check : { "E8 ? ? ? ? 85 C0 75 07 8A 46 1C 84 C0 74 0D", "E8 ? ? ? ? 85 C0 75 05 38 5E 1C 74 0D", "E8 ? ? ? ? 85 C0 75 05 38 5F 1C 74 0D" })
            {
                auto subtitles = hook::pattern(check);
                if (!subtitles.empty())
                    injector::MakeCALL(subtitles.get_first(), Subtitles::Show, true);
            }

            // The low quality audio setting uses 22050 Hz: always 44100 Hz
            uint32_t* dword_4C41F5 = hook::pattern("C7 05 ? ? ? ? ? ? 00 00 A1 ? ? ? ? 85 C0 ? ? A3 ? ? ? ? 8D 44 24 08").count(1).get(0).get<uint32_t>(6);
            injector::WriteMemory<int>(dword_4C41F5, 44100, true);

            // The boot sequence: going on after its last screen
            auto pattern = hook::pattern("56 8B F1 8B 46 0C 8B 40 04 85 C0 74 05 83 C0 FC EB 02 33 C0 8B 4C 24 08 6A 00 51 89 46 0C");
            if (!pattern.empty())
                IntroMovies::shNextScreen = safetyhook::create_inline(pattern.get_first(), IntroMovies::NextScreen);

            if (bSkipIntro)
            {
                // The intro movies
                pattern = hook::pattern("81 EC 58 01 00 00 55 56 8B B4 24 64 01 00 00 83 7E 18 07 8B E9 0F 85 ? ? ? ? A1 ? ? ? ? 85 C0 74 1C 8B 45 04 8B 0D");
                if (!pattern.empty())
                {
                    IntroMovies::FEngInstance = *pattern.get_first<void***>(0x29);
                    IntroMovies::QueueMessage = injector::GetBranchDestination(pattern.get_first(0x3B)).get();
                    IntroMovies::shStartMovie = safetyhook::create_inline(pattern.get_first(), IntroMovies::StartMovie);
                }

                // The title screen
                pattern = hook::pattern("51 8B 44 24 08 3D 37 75 25 98 56 8B F1 77");
                if (!pattern.empty())
                    SplashScreen::shNotificationMessage = safetyhook::create_inline(pattern.get_first(), SplashScreen::NotificationMessage);
            }

            // The profile is loaded at startup like on the consoles: only errors are shown, not "Load complete" with
            // a button to press. The boot sequence hides one load message (0x4000), but showing all messages again
            // during the boot brought it back; the load messages stay hidden until the boot sequence ends.
            {
                pattern = hook::pattern("C6 41 18 01 68 00 40 00 00 C7 05 ? ? ? ? 20 00 00 00 8B 89 C8 00 00 00 6A 01 E8");
                if (!pattern.empty())
                {
                    static SafetyHookInline shSetMessage = {};
                    static constexpr uint32_t LoadMessages = 0x1 | 0x1000 | 0x2000 | 0x4000;
                    static bool bBooting = false;
                    struct Memcard
                    {
                        // RealmcIface::MemcardInterface::SetMessage(state, messages), state 1 hides
                        static void __fastcall SetMessage(void* memcard, void*, int state, uint32_t messages)
                        {
                            if (messages == 0x4000)
                            {
                                // Start and end of the boot sequence
                                bBooting = state == 1;
                                messages = LoadMessages;
                            }
                            shSetMessage.unsafe_thiscall(memcard, state, messages);
                            if (bBooting && state == 0 && (messages & LoadMessages))
                                shSetMessage.unsafe_thiscall(memcard, 1, LoadMessages);
                        }
                    };
                    shSetMessage = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0x1B)).as_int(), Memcard::SetMessage);
                }
            }

            // Frame rate limit and simulation rate: -1 = monitor refresh rate, -2 = twice the refresh rate.
            // The game needs a fixed rate, so it can't be unlimited.
            if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else if (nFPSLimit <= 0)
                nFPSLimit = GetDesktopRefreshRate();
            SimRate = std::clamp(nFPSLimit, 30, 1000);
            FrameRate::fStepTime = 1.0f / static_cast<float>(SimRate);

            // Sim::Init(1/60): the simulation step
            static ProtectedGameRef<float> InitialStepTime;
            InitialStepTime.SetAddress(hook::pattern("E8 ? ? ? ? 68 89 88 88 3C").count(1).get(0).get<float>(6));
            InitialStepTime = FrameRate::fStepTime;

            // Not patched: GetDebugRealTime's 1/60 (it multiplies a frame count that is never written, so it's always 0)
            // and World_Service's "ugly timestep hack" value (only ever set to 0, never read).

            // Sim::Service
            pattern = hook::pattern("51 56 57 8D 44 24 08 50 8B F1 E8 ? ? ? ? 8B 7C 24 08 8B 46 14 8B CF");
            if (!pattern.empty())
            {
                auto service = pattern.get_first();
                FrameRate::GetTime = injector::GetBranchDestination(pattern.get_first(10)).get();
                auto runSteps = hook::range_pattern(reinterpret_cast<uintptr_t>(service), reinterpret_cast<uintptr_t>(service) + 0x100, "57 8B CE E8");
                if (!runSteps.empty())
                {
                    FrameRate::RunSteps = injector::GetBranchDestination(runSteps.get_first(3)).get();
                    static auto SimServiceHook = safetyhook::create_inline(service, FrameRate::SimService);
                }
            }

            // The real time counter in 1/60 s frames (animated textures, shader time, the streamer): it counts
            // the frame time in 1/60 s with the rest carried over, but at least one per frame, so above 60 FPS
            // it counted frames and ran fast. A frame shorter than 1/60 s now adds 0.
            pattern = hook::pattern("DD D8 75 05 B8 01 00 00 00 D9 05");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(2), 0xEB, true);

            // Front end and memory card ticks
            pattern = hook::pattern("D9 05 ? ? ? ? 50 D8 0D ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? 8B 0A 50 E8");
            if (!pattern.empty())
            {
                FrontEndTime::RealFrameTime.SetAddress(*pattern.get_first<float*>(2));
                static auto FrontEndTicks = safetyhook::create_mid(pattern.get_first(0x12), [](SafetyHookContext& regs)
                {
                    static double carry = 0.0;
                    regs.eax = FrontEndTime::Ticks(carry, 1200.0);
                });

                // The race leaderboard switches between the racers' names and their gaps every 90 frames: 1.5
                // seconds at 60 FPS, but much faster at high frame rates (#1218). It now counts 1/60 s ticks.
                auto leaderboard = hook::pattern("8B 95 28 02 00 00 4A 8B C2 85 C0 53 57 89 95 28 02 00 00");
                if (!leaderboard.empty())
                {
                    static auto LeaderboardHook = safetyhook::create_mid(leaderboard.get_first(), [](SafetyHookContext& regs)
                    {
                        static double carry = 0.0;
                        int ticks = static_cast<int>(FrontEndTime::Ticks(carry, 60.0));
                        auto& counter = *reinterpret_cast<int32_t*>(regs.ebp + 0x228);
                        if (counter > 0)
                            counter = std::max(1, counter - ticks + 1);     // the game takes 1 off next
                    });
                }

                pattern = hook::pattern("D9 05 ? ? ? ? D8 0D ? ? ? ? E8 ? ? ? ? 50 8B CE E8");
                if (!pattern.empty())
                {
                    static auto MemcardTicks = safetyhook::create_mid(pattern.get_first(0x11), [](SafetyHookContext& regs)
                    {
                        static double carry = 0.0;
                        regs.eax = FrontEndTime::Ticks(carry, 1000.0);
                    });
                }
            }

            // The simulation time
            pattern = hook::pattern("D9 05 ? ? ? ? 8B 0D ? ? ? ? D8 46 1C 8B D8 A1 ? ? ? ? 41 D9 1D ? ? ? ? 3B C7 89 0D");
            if (!pattern.empty() && *pattern.get_first<float*>(2) == *pattern.get_first<float*>(0x19))
            {
                GameBugs::pSimTime = *pattern.get_first<float*>(2);
                static auto SimTimeHook = safetyhook::create_mid(pattern.get_first(0x1D), [](SafetyHookContext& regs)
                {
                    GameBugs::OnSimStep(*reinterpret_cast<float*>(regs.esi + 0x1C));
                });
            }

            pattern = hook::pattern("81 EC 00 01 00 00 56 57 8B BC 24 0C 01 00 00 8A 07 84 C0");
            if (!pattern.empty())
                GameBugs::shStringToLowerCaseKey = safetyhook::create_inline(pattern.get_first(), GameBugs::StringToLowerCaseKey);

            pattern = hook::pattern("A1 ? ? ? ? 56 8B 35 ? ? ? ? 8D 0C 86 3B F1 74 1C 8B 0E 8B 11 FF 52 0C");
            if (!pattern.empty())
            {
                GameBugs::ListenerCount = *pattern.get_first<int32_t*>(1);
                GameBugs::Listeners = *pattern.get_first<void***>(8);
                GameBugs::shNotifyListeners = safetyhook::create_inline(pattern.get_first(), GameBugs::NotifyListeners);
            }

            // Roadblocks (a "pure virtual function call" crash): a roadblock request keeps a list of the cop cars
            // asked to form it, and the roadblock is built from the list later. A car destroyed in between (despawned
            // or totalled) stays in the list, and building the roadblock called it. A destroyed car has the vtable of
            // its abstract base again (pure virtual functions), so such cars are left out; the roadblock spawns cars
            // for the missing places or is given up, as when a car can't be used.
            pattern = hook::pattern("8B 2F 8D 54 24 10 52 8B CD 89 6C 24 20 E8");
            auto requestNext = hook::pattern("8B 74 24 34 8B 55 00 8B CD FF 92 80 00 00 00 8D 44 24 1C 50 8D 4C 24 48 E8 ? ? ? ? 8B 4B 0C");
            auto purecall = hook::pattern("55 8B EC 83 3D ? ? ? ? 00 74 06 FF 15 ? ? ? ? 6A 19 E8");
            if (!pattern.empty() && !requestNext.empty() && !purecall.empty())
            {
                static void* PureCall = purecall.get_first();
                static uintptr_t NextCar = reinterpret_cast<uintptr_t>(requestNext.get_first(0x1D));
                static auto RoadblockCarHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto car = *reinterpret_cast<void***>(regs.edi);
                    if (!car || !*car || (*reinterpret_cast<void***>(car))[32] == PureCall)
                        regs.eip = NextCar;
                });
            }

            // Racers without their body (more than 3 opponents, as other mods allow). A racer's paint and vinyls go
            // into a skin texture of its own, one of twelve skin slots (DUMMY_SKIN1-12 in CARS\TEXTURES.BIN). Slots 1-4
            // are 512x512 and get vinyls composited at run time; slots 5-12 are 256x256 for the consoles'
            // precomposited vinyls, whose PREVINYL.BIN files are empty stubs on the PC. The PC version only takes
            // slots 1-4 for the cars in the world, so the fifth racer and later got no skin and their body was drawn
            // without it: only the chassis, wheels and lights. Those cars now get slots 5-12: the game marks them as
            // precomposited (RideInfo::SetCompositeNameHash), finds no precomposited vinyl and paints the skin
            // with the car's colour only (no vinyl, lower resolution, but a complete car).
            pattern = hook::pattern("42 83 FA 04 7E EC EB ? BA 05 00 00 00 8D 4A FF");
            auto compositor = hook::pattern("83 EC 44 8B 44 24 48 8B 48 04 8D 50 08 56 8B 30 85 F6");
            auto tableBuilt = hook::pattern("A1 ? ? ? ? 85 C0 89 6C 24 ? 89 54 24 ? 89 5C 24 ? 0F 85 ? ? ? ? 33 C0 B9 40 00 00 00 BF");
            auto tableCounts = hook::pattern("8B 14 85 ? ? ? ? 8B 7C 24 ? 8B D8 C1 E3 04");
            auto vinylName = hook::pattern("83 EC 40 56 8B 74 24 4C 69 F6 D0 00 00 00 57 8B 3D");
            auto skinSlot = hook::pattern("83 EC 40 56 57 8B 7C 24 4C 83 FF 01 8B F1 7C 0B");
            if (!pattern.empty() && !compositor.empty() && !tableBuilt.empty() && !tableCounts.empty() && !vinylName.empty() && !skinSlot.empty())
            {
                SkinCompositor::shSetSkinSlot = safetyhook::create_inline(skinSlot.get_first(), SkinCompositor::SetSkinSlot);
                SkinCompositor::shVinylName = safetyhook::create_inline(vinylName.get_first(), SkinCompositor::VinylName);

                // The paint compositor fills the skin with the paint colour and writes the other colours to the
                // skin's marker pixels. It finds the markers once, in the first skin it paints (painting covers
                // them), and used those pixel positions for every skin: in a 256x256 skin the positions of a
                // 512x512 one are out of bounds (it crashed writing past the texture). It now keeps the marker
                // positions for each skin size.
                SkinCompositor::Built = *tableBuilt.get_first<int32_t*>(1);
                SkinCompositor::Table = *tableBuilt.get_first<int32_t*>(0x21);
                SkinCompositor::Counts = *tableCounts.get_first<int32_t*>(3);
                SkinCompositor::shPaint = safetyhook::create_inline(compositor.get_first(), SkinCompositor::Paint);

                injector::WriteMemory<uint8_t>(pattern.get_first(7), 0x00, true);    // the jmp past the search of slots 5-12
            }

            // The sound thread (a game bug, #1085): when the sound stops playing for a while it makes the DirectSound
            // buffer again, and if that fails the buffer is null and the next update crashed. The update now stops
            // while there's no buffer.
            pattern = hook::pattern("39 1D ? ? ? ? 56 57 0F 84 ? ? ? ? A1 ? ? ? ? 8B 08 8D 55 F0 52 50 FF 51 24");
            auto bufferUse = hook::pattern("88 5D FF A1 ? ? ? ? 8B 08");
            if (!pattern.empty() && !bufferUse.empty())
            {
                static void** pSoundBuffer = *pattern.get_first<void**>(0xF);
                static uintptr_t UpdateEnd = reinterpret_cast<uintptr_t>(pattern.get_first(14)) + *pattern.get_first<int32_t>(10);   // the jz to the end
                static auto CheckBuffer = [](SafetyHookContext& regs)
                {
                    if (!*pSoundBuffer)
                        regs.eip = UpdateEnd;
                };
                if (*bufferUse.get_first<void**>(4) == pSoundBuffer)
                {
                    static auto SoundBufferHook1 = safetyhook::create_mid(pattern.get_first(0xE), CheckBuffer);
                    static auto SoundBufferHook2 = safetyhook::create_mid(bufferUse.get_first(3), CheckBuffer);
                }
            }
        };
    }
} Misc;
