#pragma once
#include "../../../external/injector/include/psp/runtime.hpp"
#include "../../../external/injector/include/psp/safetymips.hpp"
#include "Viewport.hpp"
#include <cstring>
extern "C" {
#include <pspctrl.h>
#include <psppower.h>
#include "../../../external/injector/include/psp/inireader.h"
#include "../../../external/injector/include/psp/patterns.h"
#include "../../../external/injector/include/psp/log.h"
}
namespace console::portable {
inline bool fastForward;
inline uintptr_t gameGP;
inline void Unthrottle(bool enable) {
    if (fastForward == enable) return;
    if (sceIoDevctl("kemulator:", 0x30, reinterpret_cast<void*>(enable ? 1 : 0), 0, nullptr, 0) >= 0)
        fastForward = enable;
}
inline void Rejected(psp_hook_status status) {
    Unthrottle(false);
#ifndef NDEBUG
    logger.WriteF("Patch validation failed: %u", unsigned(status));
#else
    (void)status;
#endif
}
inline bool Emulator() { return sceIoDevctl("kemulator:", 3, nullptr, 0, nullptr, 0) == 0; }
inline bool Attach(const SceKernelModuleInfo& info) {
    injector::SetGameBaseAddress(info.text_addr, info.text_size);
    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
    gameGP = info.gp_value;
    return true;
}
inline bool FindModule(const char* name, SceKernelModuleInfo& info) {
    SceUID modules[64]; int count = 0;
    if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) < 0) return false;
    if (count > 64) count = 64;
    for (int i = 0; i < count; ++i) {
        info = {}; info.size = sizeof(info);
        if (sceKernelQueryModuleInfo(modules[i], &info) >= 0 && std::strcmp(info.name, name) == 0) return true;
    }
    return false;
}
inline bool Start(const char* game, const char* ini, const char* log) {
    if (!Emulator() || injector::InitializeRuntime(Rejected) != PSP_HOOK_OK) return false;
    SceKernelModuleInfo own{}; own.size = sizeof(own);
    if (sceKernelQueryModuleInfo(sceKernelGetModuleIdByAddress(reinterpret_cast<const void*>(&Start)), &own) >= 0)
        injector::SetModuleBaseAddress(own.text_addr, own.text_size);
    inireader.SetIniPath(ini);
#ifndef NDEBUG
    logger.SetPath(log);
#else
    (void)log;
#endif
    SceKernelModuleInfo info{};
    return FindModule(game, info) && Attach(info);
}
inline bool Begin() { return injector::InitializeCheckedRuntime(Rejected) == PSP_HOOK_OK; }
inline int Finish() { return injector::FlushCaches() == PSP_HOOK_OK ? 0 : -1; }
inline uintptr_t Absolute(uintptr_t at, int high = 0, int low = 4) {
    if (!at) return 0;
    return (uintptr_t(injector::ReadMemory<uint16_t>(at + high)) << 16) + int16_t(injector::ReadMemory<uint16_t>(at + low));
}
inline float Aspect() {
    float value = 480.0f / 272.0f;
    if (sceIoDevctl("kemulator:", 0x31, nullptr, 0, &value, sizeof(value)) < 0) return 480.0f / 272.0f;
    return bounded(value, 0.5f, 8.0f, 480.0f / 272.0f);
}
}
