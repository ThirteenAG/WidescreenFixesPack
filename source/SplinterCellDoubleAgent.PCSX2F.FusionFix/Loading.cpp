#include "Game.hpp"
#include "LoadingGeometry.hpp"

namespace scda {
namespace {
SafetyMipsMid loadingDraw;
safetymips::GameInline<uint64_t(void*)> loadingEnd;
uint64_t FinishLoading(void* context) {
    // The native cleanup stops the loading thread. Clear the flag afterwards
    // so its last draw cannot switch unthrottling back on.
    const auto result = loadingEnd.call(context);
    FrameLimitUnthrottle = 0;
    return result;
}
}
void InstallLoading() {
    loadingDraw = safetymips::create_mid(game.loadingDraw, [](SafetyMipsContext& regs) {
        FrameLimitUnthrottle = 1;
        UpdateSettings(true);
        // The loading thread has finished its GIF packet on the stack. The
        // first two XYZ2 values clear the full frame; later XYZ2 values draw
        // the loading artwork and its progress bar over that black clear.
        const auto start = uintptr_t(regs.sp) + 0x140;
        const auto end = uintptr_t(regs.s0);
        if (end <= start + 34 * sizeof(uint64_t) || end > start + 0x1C0) return;
        auto* packet = reinterpret_cast<uint64_t*>(start);
        const auto entries = (end - start) / sizeof(uint64_t);
        const float aspect = Aspect();
        for (size_t i = 34; i + 1 < entries; i += 2)
            if (packet[i + 1] == 4) packet[i] = FitLoadingVertex(packet[i], aspect);
    });
    loadingEnd.create(game.loadingEnd, FinishLoading);
}
}
