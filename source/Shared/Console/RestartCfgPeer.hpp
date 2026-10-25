#pragma once
#include "PSP.hpp"
#include "RestartCfgFile.hpp"

namespace console::portable {
// Known module + known restart CFG, not the PC loaded-settings interface. No
// foreign function, global pointer or GP is retained or called. The owned
// semaphore and hooks are process-lifetime; dynamic unload is unsupported.
class RestartCfgPeer {
    const char* moduleName;
    RestartCfgFile<> file;
    SceUID semaphore = -1;
    static bool same(const SceKernelModuleInfo& a, const SceKernelModuleInfo& b) {
        return a.text_addr == b.text_addr && a.text_size == b.text_size && a.gp_value == b.gp_value &&
            a.nsegment == b.nsegment && !std::strcmp(a.name, b.name) &&
            !std::memcmp(a.segmentaddr, b.segmentaddr, sizeof(a.segmentaddr)) &&
            !std::memcmp(a.segmentsize, b.segmentsize, sizeof(a.segmentsize));
    }
    bool present(SceKernelModuleInfo& info) const {
        return moduleName && FindModule(moduleName, info) && info.text_addr && info.text_size;
    }
public:
    RestartCfgPeer(const char* module, const char* path, const char* section, const char* key)
        : moduleName(module), file(path, section, key) {}
    // Call once before installing native callbacks; failed initialization leaves
    // only this sibling unavailable and does not prevent the host's own menu.
    bool Initialize() {
        if (semaphore >= 0) return true;
        semaphore = sceKernelCreateSema("Restart CFG peer", 0, 1, 1, nullptr);
        return semaphore >= 0;
    }
    bool Read(bool& requested) {
        if (semaphore < 0 || sceKernelPollSema(semaphore, 1) < 0) return false;
        SceKernelModuleInfo before{}, after{};
        bool value = false;
        const bool valid = present(before) && file.Read(value) && present(after) && same(before, after);
        sceKernelSignalSema(semaphore, 1);
        if (valid) requested = value;
        return valid;
    }
    bool Write(bool requested) {
        if (semaphore < 0 || sceKernelPollSema(semaphore, 1) < 0) return false;
        SceKernelModuleInfo before{}, after{};
        const bool valid = present(before) && file.Write(requested, [&] { return present(after) && same(before, after); });
        sceKernelSignalSema(semaphore, 1);
        return valid;
    }
};
// Fixed-token choice variant; the existing boolean peer stays unchanged.
template<class Codec> class RestartCfgChoicePeer {
    using Value = typename Codec::Value;
    const char* moduleName;
    RestartCfgFile<8192, Codec> file;
    SceUID semaphore = -1;
    static bool same(const SceKernelModuleInfo& a, const SceKernelModuleInfo& b) {
        return a.text_addr == b.text_addr && a.text_size == b.text_size && a.gp_value == b.gp_value &&
            a.nsegment == b.nsegment && !std::strcmp(a.name, b.name) &&
            !std::memcmp(a.segmentaddr, b.segmentaddr, sizeof(a.segmentaddr)) &&
            !std::memcmp(a.segmentsize, b.segmentsize, sizeof(a.segmentsize));
    }
    bool present(SceKernelModuleInfo& info) const {
        return moduleName && FindModule(moduleName, info) && info.text_addr && info.text_size;
    }
public:
    RestartCfgChoicePeer(const char* module, const char* path, const char* section, const char* key, Codec codec)
        : moduleName(module), file(path, section, key, codec) {}
    // Call once before installing native callbacks; failed initialization leaves
    // only this sibling unavailable and does not prevent the host's own menu.
    bool Initialize() {
        if (semaphore >= 0) return true;
        semaphore = sceKernelCreateSema("Restart CFG peer", 0, 1, 1, nullptr);
        return semaphore >= 0;
    }
    bool Read(Value& requested) {
        if (semaphore < 0 || sceKernelPollSema(semaphore, 1) < 0) return false;
        SceKernelModuleInfo before{}, after{};
        Value value{};
        const bool valid = present(before) && file.Read(value) && present(after) && same(before, after);
        sceKernelSignalSema(semaphore, 1);
        if (valid) requested = value;
        return valid;
    }
    bool Write(Value requested) {
        if (semaphore < 0 || sceKernelPollSema(semaphore, 1) < 0) return false;
        SceKernelModuleInfo before{}, after{};
        const bool valid = present(before) && file.Write(requested, [&] { return present(after) && same(before, after); });
        sceKernelSignalSema(semaphore, 1);
        return valid;
    }
};
}
