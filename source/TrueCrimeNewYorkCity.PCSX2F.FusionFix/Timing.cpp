#include "Game.hpp"

namespace tcny
{
    namespace
    {
        safetymips::GameInline<uint64_t(void*)> processHook;
        SafetyMipsMid frameHook;
        // The vsync wait is a JAL; mid hooks cannot relocate it, so replace the call.
        injector::hook_back<void(int32_t)> waitVsync;
        pcsx2::GameCallback<void(int32_t)> waitVsyncCallback;
        // Screen setup (screen, native widescreen flag, mode): projection and HUD scale.
        pcsx2::GameFunction<void(uintptr_t, int32_t, int32_t)> setupScreen;
        uintptr_t screen;
        uint32_t layout;
        // Copy the case/mission ids while the game object is known to be live;
        // no pointer to it outlives this call.
        int caseId = -1, missionId = -1;

        uint64_t Process(void* object)
        {
            const auto result = processHook.call(object);
            if (object)
            {
                auto fields = static_cast<const uint8_t*>(object);
                caseId = *reinterpret_cast<const int*>(fields + 0x1078);
                missionId = *reinterpret_cast<const int*>(fields + 0x107C);
            }
            return result;
        }
        // Case BC03, mission M6 requires the original frame rate.
        bool Needs30FPS()
        {
            return caseId == 4 && missionId == 10;
        }
        void Frame(SafetyMipsContext& regs)
        {
            ++frame;
            PollSettings();
            display.refresh();
            // A new screen aspect: let the game set its projection and HUD scale again.
            const uint32_t current = Bits(display.scale);
            if (current != layout)
            {
                if (layout && screen)
                {
                    // HUD pages already shown keep their old placement until the
                    // game builds them again (pause menu, loading).
                    setupScreen(screen, 0, 0);
                }
                layout = current;
            }
            FrameLimitUnthrottle = unthrottle && loading && *loading != 0;
            if (enable60)
            {
                const auto interval = Needs30FPS() ? 2 : 1;
                regs.s1 = regs.s0 = interval * 2;
                regs.s4 = interval;
            }
        }
        void WaitVsync(int32_t)
        {
            waitVsync.fun(Needs30FPS() ? 2 : 1);
        }
    } // namespace

    void InstallTiming(uintptr_t process, uintptr_t frameSite, uintptr_t vsyncCall, uintptr_t wide)
    {
        // The native widescreen setter (flag stored, then the screen set up).
        screen = (uintptr_t(Read<uint16_t>(wide + 0xC)) << 16) + int16_t(Read<uint16_t>(wide + 0x14));
        setupScreen.bind(injector::GetBranchDestination(wide + 0x2C).as_int());
        layout = Bits(display.scale);
        processHook = safetymips::create_inline_game(process, Process);
        frameHook = safetymips::create_mid<&Frame>(frameSite);
        if (enable60 && waitVsyncCallback.bind(WaitVsync) == PCSX2_HOOK_OK)
        {
            waitVsync.fun = injector::MakeCALL(vsyncCall, waitVsyncCallback.address()).get();
        }
    }
} // namespace tcny
