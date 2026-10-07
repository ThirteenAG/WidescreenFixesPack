module;

#include "stdafx.h"

export module CameraProfiles;

import Build;
import Settings;

namespace
{
    std::vector<SafetyHookMid> profileHooks;
    int* carZoomCount = nullptr;
    int PedZoom(uintptr_t camera)
    {
        return *reinterpret_cast<const int*>(camera + 308);
    }
    void SetEqual(SafetyHookContext& c, bool equal)
    {
        c.rflags = (c.rflags & ~uintptr_t{0x40}) | (equal ? 0x40 : 0);
    }
}

class CameraProfilesModule
{
public:
    CameraProfilesModule()
    {
        WFP::onInitEvent() += []()
        {
            if (!Settings.restoreOriginalCamera)
                return;
            const std::array sites{
                hook::pattern("0F 2E 9B 48 01 00 00 F3 0F 11 9B 38 01 00 00 74 ? 8B 05 ? ? ? ? 0F").get_first(),
                hook::pattern("E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 B9 0A 00 00 00 E8 ? ? ? ? 0F 28 7C 24 20 84 C0").get_first(),
                hook::pattern("E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 EB ? E8 ? ? ? ? 84 C0 74 ? 8B 83 34 01 00 00").get_first(),
                hook::pattern("E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 7C 24 58 48 85 C9 74 ? 48 8B").get_first(),
                hook::pattern("0F 84 ? ? ? ? 80 7B 76 00 0F 85 ? ? ? ? 80 7B 67 00 48 89 74 24").get_first(),
                hook::pattern("B8 01 00 00 00 0F 44 D0 8B 89 4C 01 00 00 33 FF 83 FA 05 8B F7 0F 45 F2").get_first(),
                hook::pattern("0F 45 F2 3B CE 0F 84 ? ? ? ? 8B 05 ? ? ? ? 0F 57 C0 F3 48 0F 2A").get_first(),
                hook::pattern("75 ? B8 01 00 00 00 66 89 05 ? ? ? ? 0F 2E A3 60 01 00 00 74 ? 8B").get_first(),
                hook::pattern("75 ? 44 88 7E 6B 44 38 76 71 75 ? 66 83 FB 2C 77 ? 48 B9 80 09 80 30").get_first(),
                hook::pattern("F3 0F 11 A3 50 01 00 00 75 ? 85 C9 75 ? 8D 41 10 EB ? 3B 0D ? ? ?").get_first()};
            if (std::any_of(sites.begin(), sites.end(), [](auto p) { return !p; }))
                return;
            carZoomCount = reinterpret_cast<int*>((injector::ReadRelativeOffset(hook::pattern("C7 05 ? ? ? ? 05 00 00 00 0F 84 ? ? ? ? 80 7B 76 00 0F 85 ? ?").get_first<uint8_t>(2)).as_int() + 4));
            profileHooks.push_back(safetyhook::create_mid(sites[0], [](SafetyHookContext& c)
            {
                // re3: MaxDist = 2 + {0.25, 1.5, 2.9}. DE's native
                // collision placement adds 0.3 to the selected distance.
                constexpr std::array distances{1.95f, 3.2f, 4.6f};
                const auto zoom = PedZoom(c.rbx);
                if (zoom >= 0 && zoom < distances.size())
                    c.xmm3.f32[0] = distances[zoom];
            }));
            // Restore III's fourth, top-down choice through native input gates.
            // Index 3 is an unused DE placeholder; native top-down is index 4.
            profileHooks.push_back(safetyhook::create_mid(sites[1], [](SafetyHookContext& c)
            {
                const auto zoom = PedZoom(c.rcx);
                if (zoom == 4)
                    c.rdx = 2;
                else if (zoom == 0)
                    c.rdx = 4;
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[2], [](SafetyHookContext& c)
            {
                const auto zoom = PedZoom(c.rcx);
                if (zoom == 2)
                    c.rdx = 4;
                else if (zoom == 4)
                    c.rdx = 0;
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[3], [](SafetyHookContext& c)
            {
                if (PedZoom(c.rcx) == 2)
                    c.rdx = 4;
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[4], [](SafetyHookContext&)
            {
                // 0 first person, 1/2/3 follow, 4 top-down, 5 cinematic,
                // 6 wraps to first person. Both extra table entries are zero.
                *carZoomCount = 6;
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[5], [](SafetyHookContext& c)
            {
                // Preserve remote-vehicle restrictions on first person and
                // cinematic; the newly restored top-down choice is allowed.
                SetEqual(c, c.rdx == 0 || c.rdx == 5);
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[6], [](SafetyHookContext& c)
            {
                SetEqual(c, c.rdx == 6);
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[7], [](SafetyHookContext& c)
            {
                // Native mode 1 dispatch, including garage/cull-zone gates.
                SetEqual(c, c.rcx == 4);
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[8], [](SafetyHookContext& c)
            {
                SetEqual(c, *reinterpret_cast<const int*>(c.rsi + 332) == 5);
            }));
            profileHooks.push_back(safetyhook::create_mid(sites[9], [](SafetyHookContext& c)
            {
                if (c.rcx == 4)
                    c.xmm4.f32[0] = 1.0f;
                else if (c.rax >= 1 && c.rax <= 3)
                {
                    // Use the native effective index, including cull-zone
                    // restrictions. Script zoom overrides still run later.
                    constexpr std::array distances{0.05f, 1.9f, 3.9f};
                    c.xmm4.f32[0] = distances[c.rax - 1];
                }
            }));
            if (std::any_of(profileHooks.begin(), profileHooks.end(), [](const auto& hook) { return !hook; }))
                profileHooks.clear();
        };
    }
} CameraProfilesModuleInstance;
