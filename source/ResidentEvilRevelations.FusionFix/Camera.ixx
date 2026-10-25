module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "CameraFOV.hpp"

export module Camera;

import ComVars;

namespace
{
    const float* pFOVFactor{};
    uint8_t** pGameCamera{};
    SafetyHookInline shSetPerspective;
    SafetyHookMid shGameplayFOV;

    template<typename T>
    const T& Field(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    const uint8_t* TypeInfo(const void* object)
    {
        if (!object)
            return nullptr;
        auto vtable = *static_cast<const uintptr_t* const*>(object);
        if (!vtable || !vtable[4])
            return nullptr;
        return reinterpret_cast<const uint8_t* (__thiscall*)(const void*)>(vtable[4])(object);
    }

    bool HasType(const void* object, std::string_view name, size_t size)
    {
        auto type = TypeInfo(object);
        if (!type || (Field<uint32_t>(type, 24) & 0x7FFFFF) != size)
            return false;
        auto typeName = Field<const char*>(type, 4);
        return typeName && name == typeName;
    }

    bool IsPlayer(const void* object)
    {
        auto type = TypeInfo(object);
        if (!type || (Field<uint32_t>(type, 24) & 0x7FFFFF) < 0x1152)
            return false;
        for (size_t depth = 0; type && depth < 16; ++depth)
        {
            auto name = Field<const char*>(type, 4);
            if (name && std::string_view(name) == "uPlayer")
                return true;
            type = Field<const uint8_t*>(type, 16);
        }
        return false;
    }

    bool IsGameplayManager(uint8_t* manager)
    {
        if (!pGameCamera || !*pGameCamera || !manager ||
            !HasType(*pGameCamera, "sGameCamera", 3344) ||
            Field<uint8_t*>(*pGameCamera, 0xCE0) != manager || !HasType(manager, "uCameraManage", 544))
            return false;
        auto aspect = Field<float>(manager, 0x38);
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            return false;
        auto critical = reinterpret_cast<CRITICAL_SECTION*>(manager + 0x200);
        EnterCriticalSection(critical);
        struct Lock
        {
            CRITICAL_SECTION* critical;
            ~Lock()
            {
                LeaveCriticalSection(critical);
            }
        } lock{ critical };
        auto index = Field<uint32_t>(manager, 0xCC);
        auto count = Field<uint32_t>(manager, 0xD8);
        auto cameras = Field<uint8_t**>(manager, 0xE4);
        if (!cameras || count > 64 || index >= count || index != Field<uint32_t>(manager, 0xE8) ||
            index != Field<uint32_t>(manager, 0xD0) || (Field<uint32_t>(manager, 0x138) & ~0x3Fu))
            return false;
        auto camera = cameras[index];
        if (!HasType(camera, "cTPSCamera", 416) || Field<uint8_t*>(camera, 0x50) != manager ||
            !Field<uintptr_t>(camera, 0xC0) || !Field<uintptr_t>(camera, 0xD4) ||
            Field<uintptr_t>(camera, 0xC4) || Field<uint8_t>(camera, 0xDC) || Field<uint32_t>(camera, 0xE4) ||
            Field<int32_t>(camera, 0xA0) < 0 || Field<int32_t>(camera, 0xA0) != Field<int32_t>(camera, 0xA4))
            return false;
        auto player = Field<uint8_t*>(manager, 0x78);
        if (!player || player != Field<uint8_t*>(manager, 0x74) || !IsPlayer(player))
            return false;
        auto state = Field<uint32_t>(player, 12) & 7;
        auto control = Field<uint32_t>(player, 0xE30);
        if ((state != 1 && state != 2) || (control != 0 && control != 3) || Field<uint8_t>(player, 0x1151))
            return false;
        count = Field<uint32_t>(manager, 0x108);
        auto effects = Field<uint8_t**>(manager, 0x114);
        if (count > 64 || (count && !effects))
            return false;
        for (size_t i = 0; i < count; ++i)
        {
            if (effects[i] && Field<uint8_t>(effects[i], 0xC0))
                return false;
        }
        return true;
    }

    void ApplyGameplayFOV(SafetyHookContext& regs)
    {
        if (!pFOVFactor || *pFOVFactor == 1.0f)
            return;
        auto manager = reinterpret_cast<uint8_t*>(regs.esi);
        if (!IsGameplayManager(manager))
            return;
        auto fov = Field<float>(manager, 0x3C);
        auto nearClip = Field<float>(manager, 0x34);
        if (RevCameraFOV::Scale(fov, nearClip, Field<float>(manager, 0x30), *pFOVFactor))
        {
            // Change the published camera output, retaining the controller's unscaled state.
            *reinterpret_cast<float*>(manager + 0x3C) = fov;
            *reinterpret_cast<float*>(manager + 0x34) = nearClip;
        }
    }

    void __fastcall SetPerspective(float* matrix, void* edx, float zNear, float zFar, float fov, float aspect)
    {
        if (std::isfinite(aspect) && std::abs(aspect - 1.0f) > 0.01f)
        {
            auto diff = GetDiff();
            if (std::isfinite(diff) && diff > 0.0f)
                zNear /= diff;
        }
        shSetPerspective.unsafe_fastcall(matrix, edx, zNear, zFar, fov, aspect);
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
            auto perspective = hook::pattern("E8 ? ? ? ? 85 DB 74 2E 8B 56 50");
            if (PatternUnique(perspective, "ResidentEvilRevelations.FusionFix: Camera"))
                shSetPerspective = safetyhook::create_inline(injector::GetBranchDestination(perspective.get_first()).as_int(), SetPerspective);
            if (!shSetPerspective)
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: near-clip projection hook unavailable.\n");

            auto output = hook::pattern("D9 5E 34 8B 03 8B 50 48 8B CB FF D2 D9 5E 30 D9 86 8C 00 00 00 0F 57 C0 D9 5E 3C 8B CB D9 86 90 00 00 00");
            auto root = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 66 89 5C 24 0C 88 5C 24 0E 8B 44 24 0C");
            auto aim = hook::pattern("56 8B F1 E8 ? ? ? ? 84 C0 74 0A 8A 44 24 08 88 86 51 11 00 00 5E C2 04 00");
            auto control = hook::pattern("8B 81 30 0E 00 00 85 C0 75 03 B0 01 C3 83 F8 03 0F 94 C0 C3");
            auto owner = hook::pattern("8D 9E 00 02 00 00 53 FF 15 ? ? ? ? 8B 7C 24 10 89 7E 74 85 FF 74 28 8B 07 8B 50 10 8B CF FF D2 8B 40 1C 25 00 00 0F F0 3D 00 00 01 80 75 10 53 89 7E 78");
            if (output.size() != 1 || root.size() != 1 || aim.size() != 1 || control.size() != 1 || owner.size() != 1 ||
                *root.get_first<uint8_t**>(2) != *root.get_first<uint8_t**>(13) ||
                injector::GetBranchDestination(aim.get_first(3)).as_int() != reinterpret_cast<uintptr_t>(control.get_first()))
            {
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: unsupported gameplay camera layout.\n");
                return;
            }
            pGameCamera = *root.get_first<uint8_t**>(2);
            // The native x87 store completes before the callback; projection consumes the published camera output.
            shGameplayFOV = safetyhook::create_mid(output.get_first(27), ApplyGameplayFOV);
            if (!shGameplayFOV)
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: gameplay camera hook unavailable.\n");
        };
    }
} Camera;
