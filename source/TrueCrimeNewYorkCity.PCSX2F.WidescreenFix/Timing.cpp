#include "Game.hpp"
namespace tcny {
namespace {
safetymips::GameInline<uint64_t(void*)> processHook;
SafetyMipsMid frameHook;
// The vsync wait is a JAL; mid hooks cannot relocate it, so replace the call.
injector::hook_back<void(int32_t)> waitVsync;
pcsx2::GameCallback<void(int32_t)> waitVsyncCallback;
// Copy the case/mission ids while the game object is known to be live
// (as HEAD did); no pointer to it outlives this call.
int caseId = -1, missionId = -1;
uint64_t Process(void* object) {
    const auto result = processHook.call(object);
    if (object) {
        auto fields = static_cast<const uint8_t*>(object);
        caseId = *reinterpret_cast<const int*>(fields + 0x1078);
        missionId = *reinterpret_cast<const int*>(fields + 0x107C);
    }
    return result;
}
// Case BC03, mission M6 requires the original frame rate.
bool Needs30FPS() { return caseId == 4 && missionId == 10; }
void Frame(SafetyMipsContext& regs) {
    display.refresh();
    FrameLimitUnthrottle = unthrottle && loading && *loading != 0;
    if (enable60) {
        const auto interval = Needs30FPS() ? 2 : 1;
        regs.s1 = regs.s0 = interval * 2;
        regs.s4 = interval;
    }
}
void WaitVsync(int32_t count) { waitVsync.fun(Needs30FPS() ? 2 : 1); }
}
void InstallTiming(uintptr_t process, uintptr_t frame, uintptr_t vsyncCall) {
    processHook = safetymips::create_inline_game(process, Process);
    frameHook = safetymips::create_mid<&Frame>(frame);
    if (enable60 && waitVsyncCallback.bind(WaitVsync) == PCSX2_HOOK_OK)
        waitVsync.fun = injector::MakeCALL(vsyncCall, waitVsyncCallback.address()).get();
}
}
