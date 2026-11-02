module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

export module Camera;

import ComVars;

namespace CameraFOV
{
    inline bool IsGameplayType(std::string_view name)
    {
        return name == "uCameraQFPS" || name == "uCameraVeh";
    }

    inline bool IsNormalState(int state, bool ready, bool external, uint32_t lookFlags)
    {
        return (state == 0 || state == 1) && !ready && !external && !(lookFlags & 0xD);
    }

    inline float Scale(float fov, float aspect, float factor)
    {
        if (!std::isfinite(fov) || fov <= 0.0f || fov >= 3.14159265358979323846f || !std::isfinite(aspect) || aspect <= 0.0f ||
            std::abs(aspect - 1.0f) <= 0.01f || !std::isfinite(factor) || factor <= 0.0f || factor == 1.0f)
            return fov;
        constexpr float pi = 3.14159265358979323846f;
        constexpr float horizontalLimit = 150.0f * pi / 180.0f;
        const float nativeTangent = std::tan(fov * 0.5f);
        const float limitTangent = std::tan(horizontalLimit * 0.5f) / aspect;
        const float tangent = (std::min)(nativeTangent * std::clamp(factor, 0.5f, 2.0f), limitTangent);
        return 2.0f * std::atan(tangent);
    }
}

// The game's projection setup (near, far, vertical FOV, aspect) is used by every camera: gameplay, cutscenes,
// shadows and light shafts. The game is Hor+ by itself.
// - Near clip: at wider aspects the near plane of the screen's cameras is moved closer, so the wider view doesn't cut
//   into walls next to the camera. Square projections (shadow maps of spot lights) are left alone.
// - MAIN.FOVFactor scales the normal and vehicle gameplay cameras; scripted, weapon and unknown camera types keep
//   their native projection.

namespace
{
    const float* pFOVFactor = nullptr;
    bool gameplayLayoutReady = false;

    bool BindGameplayLayout()
    {
        struct Field { const char* name; uint32_t offset; size_t matches = 0; };
        Field fields[] = { { "cameraState", 732 }, { "agentReady", 740 }, { "mbUseExt", 829 }, { "mLookCamera", 6656 } };
        auto properties = hook::pattern("8D 93 ? ? ? ? 8B CF 89 47 04 C7 44 24 10 ? ? ? ? C7 44 24 14");
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto end = base + pe->OptionalHeader.SizeOfImage;
        for (size_t i = 0; i < properties.size(); ++i)
        {
            const auto code = properties.get(i).get<uint8_t>();
            const auto name = *reinterpret_cast<const char**>(code + 15);
            const auto address = reinterpret_cast<uintptr_t>(name);
            if (address < base || address >= end || !std::memchr(name, 0, (std::min)(size_t{ 64 }, static_cast<size_t>(end - address)))) continue;
            for (auto& field : fields)
                if (std::strcmp(name, field.name) == 0 && *reinterpret_cast<const uint32_t*>(code + 2) == field.offset)
                    ++field.matches;
        }
        return std::all_of(std::begin(fields), std::end(fields), [](const auto& field) { return field.matches == 1; });
    }

    SafetyHookInline shSetPerspective = {};
    void __fastcall SetPerspective(float* matrix, void* edx, float zNear, float zFar, float fov, float aspect)
    {
        if (std::abs(aspect - 1.0f) > 0.01f)
        {
            const float diff = IsSplitScreenActive() ? SplitScreen::GetDiff() : GetDiff();
            if (std::isfinite(diff) && diff > 1.0f)
                zNear /= diff;
        }
        shSetPerspective.unsafe_fastcall(matrix, edx, zNear, zFar, fov, aspect);
    }

    // The class name of an MT Framework object: vtable slot 4 returns its type info, which holds the name at +4
    const char* GetClassName(void* object)
    {
        if (!object)
            return nullptr;
        auto vtable = *(uintptr_t**)object;
        if (!vtable || !vtable[4])
            return nullptr;
        auto getDTI = (void* (__fastcall*)(void*))vtable[4];
        auto dti = (uintptr_t)getDTI(object);
        return dti ? *(const char**)(dti + 4) : nullptr;
    }

    bool IsGameplayCamera(void* camera)
    {
        static std::unordered_map<uintptr_t, std::string_view> known;
        if (!camera)
            return false;
        auto vtable = *(uintptr_t*)camera;
        auto it = known.find(vtable);
        if (it == known.end())
        {
            const auto name = GetClassName(camera);
            if (!name) return false;
            it = known.emplace(vtable, name).first;
        }
        if (!CameraFOV::IsGameplayType(it->second)) return false;
        if (it->second == "uCameraVeh") return true;
        if (!gameplayLayoutReady) return false;
        const auto bytes = static_cast<const uint8_t*>(camera);
        return CameraFOV::IsNormalState(*reinterpret_cast<const int*>(bytes + 732), bytes[740] != 0,
            bytes[829] != 0 || bytes[830] != 0,
            *reinterpret_cast<const uint32_t*>(bytes + 6748) | *reinterpret_cast<const uint32_t*>(bytes + 6752));
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            pFOVFactor = &FixSettings().Float("MAIN.FOVFactor");
            gameplayLayoutReady = BindGameplayLayout();
            if (!gameplayLayoutReady)
                OutputDebugStringA("ResidentEvil6.FusionFix: gameplay camera state layout unavailable or ambiguous.\n");

            auto pattern = hook::pattern("E8 ? ? ? ? 80 7D 10 00 74 23");
            shSetPerspective = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetPerspective);

            // uCameraBase's projection: ecx still holds the camera before the native mov ecx, esi; FOV is at esp+8
            pattern = hook::pattern("8B 74 24 18 8B CE F3 0F 11 04 24");
            static auto CameraFOVHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const float factor = *pFOVFactor;
                if (factor == 1.0f || !std::isfinite(factor) || factor <= 0.0f || IsCutsceneViewActive())
                    return;
                if (!IsGameplayCamera((void*)regs.ecx))
                    return;
                float& fov = *reinterpret_cast<float*>(regs.esp + 8);
                fov = CameraFOV::Scale(fov, *reinterpret_cast<float*>(regs.esp + 12), factor);
            });
        };
    }
} Camera;
