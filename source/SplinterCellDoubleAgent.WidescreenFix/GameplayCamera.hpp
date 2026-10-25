#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include "../Shared/SplinterCell/GameplayFOV.hpp"

namespace DAGameplayCamera
{
    inline wchar_t*(__fastcall* fullName)(void*, void*, wchar_t*) = nullptr;
    inline decltype(fullName)* fullNameSource = nullptr;
    inline std::array<uintptr_t, 4> callers{};

    inline bool FinalizeCallers()
    {
        if (std::any_of(callers.begin(), callers.end(), [](uintptr_t p) { return p == 0; }))
        {
            callers.fill(0);
            return false;
        }
        return true;
    }

    template <class T> T Read(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }
    inline std::wstring Name(void* object)
    {
        auto resolve = fullNameSource ? *fullNameSource : fullName;
        if (!object || !resolve)
            return {};
        wchar_t buffer[256]{};
        auto name = resolve(object, nullptr, buffer);
        return name ? name : L"";
    }
    inline bool Type(void* object, std::wstring_view type)
    {
        auto name = Name(object);
        return name.size() > type.size() && name.compare(0, type.size(), type) == 0 && name[type.size()] == L' ';
    }
    inline std::wstring Leaf(void* object)
    {
        auto name = Name(object);
        auto pos = name.find_last_of(L". ");
        return pos == std::wstring::npos ? name : name.substr(pos + 1);
    }
    // Double Agent's Core layout is checked independently of earlier games.
    inline void* Property(void* cls, std::wstring_view name, std::wstring_view type)
    {
        auto total = Read<int>(cls, 68);
        if (total <= 0 || total > 0x100000)
            return nullptr;
        for (unsigned depth = 0; cls && depth < 32; ++depth, cls = Read<void*>(cls, 44))
        {
            unsigned count = 0;
            for (auto field = Read<void*>(cls, 64); field && ++count <= 8192;)
            {
                if (Leaf(field) == name && Type(field, type))
                {
                    if (Read<uint16_t>(field, 56) != 1 || Read<uint16_t>(field, 58) != 4 ||
                        size_t(Read<uint16_t>(field, 72)) + 4 > size_t(total))
                        return nullptr;
                    if (type == L"BoolProperty")
                    {
                        auto mask = Read<uint32_t>(field, 88);
                        if (!mask || (mask & (mask - 1)))
                            return nullptr;
                    }
                    return field;
                }
                auto next = Read<void*>(field, 48);
                if (next == field)
                    break;
                field = next;
            }
        }
        return nullptr;
    }
    template <class T> T Value(void* object, void* prop)
    {
        return Read<T>(object, Read<uint16_t>(prop, 72));
    }
    inline bool Flag(void* object, void* prop)
    {
        return (Value<uint32_t>(object, prop) & Read<uint32_t>(prop, 88)) != 0;
    }
    inline std::wstring State(void* object)
    {
        auto frame = Read<void*>(object, 12);
        auto state = frame ? Read<void*>(frame, 28) : nullptr;
        return Type(state, L"State") ? Leaf(state) : L"";
    }
    struct Contract
    {
        void* controllerClass = nullptr;
        void* camera = nullptr;
        void* pawn = nullptr;
        void* viewTarget = nullptr;
        void* cinematic = nullptr;
        void* cameraClass = nullptr;
        void* owner = nullptr;
        void* sam = nullptr;
        void* freeCamera = nullptr;
        void* fixedCamera = nullptr;
    };
    inline Contract contract;
    inline const int* advancedCameraMode = nullptr;
    inline void Reset()
    {
        contract = {};
    }
    inline bool Controller(void* controller)
    {
        auto cls = Read<void*>(controller, 36);
        if (!cls)
            return false;
        if (contract.controllerClass == cls)
            return true;
        if (!Type(controller, L"EPlayerController"))
            return false;
        auto camera = Property(cls, L"m_camera", L"ObjectProperty");
        auto pawn = Property(cls, L"EPawn", L"ObjectProperty");
        auto target = Property(cls, L"ViewTarget", L"ObjectProperty");
        auto cinematic = Property(cls, L"bInCinematic", L"BoolProperty");
        if (!camera || !pawn || !target || !cinematic)
            return false;
        contract = {};
        contract.controllerClass = cls;
        contract.camera = camera;
        contract.pawn = pawn;
        contract.viewTarget = target;
        contract.cinematic = cinematic;
        return true;
    }
    inline bool Camera(void* camera)
    {
        auto cls = Read<void*>(camera, 36);
        if (!cls)
            return false;
        if (contract.cameraClass == cls)
            return true;
        if (!Type(camera, L"EPlayerCam"))
            return false;
        auto owner = Property(cls, L"m_EPC", L"ObjectProperty");
        auto sam = Property(cls, L"m_sam", L"ObjectProperty");
        auto freeCamera = Property(cls, L"m_freeCam", L"BoolProperty");
        auto fixedCamera = Property(cls, L"m_fixeCam", L"BoolProperty");
        if (!owner || !sam || !freeCamera || !fixedCamera)
            return false;
        contract.cameraClass = cls;
        contract.owner = owner;
        contract.sam = sam;
        contract.freeCamera = freeCamera;
        contract.fixedCamera = fixedCamera;
        return true;
    }
    inline float Factor(void* scene, uintptr_t caller, float factor)
    {
        if (!scene || factor == 1.0f || !std::isfinite(factor) || !caller ||
            std::find(callers.begin(), callers.end(), caller) == callers.end())
            return 1.0f;
        if (!advancedCameraMode || *advancedCameraMode)
            return 1.0f;
        auto viewport = Read<void*>(scene, 4);
        if (!viewport || Read<void*>(scene, 8) || Read<int>(viewport, 464))
            return 1.0f;
        auto controller = Read<void*>(viewport, 52);
        if (!controller || !Controller(controller) || Flag(controller, contract.cinematic))
            return 1.0f;
        auto pawn = Value<void*>(controller, contract.pawn);
        auto camera = Value<void*>(controller, contract.camera);
        if (!pawn || Read<void*>(scene, 560) != pawn || Value<void*>(controller, contract.viewTarget) != pawn || !camera || !Camera(camera))
            return 1.0f;
        if (Value<void*>(camera, contract.owner) != controller || Value<void*>(camera, contract.sam) != pawn ||
            Flag(camera, contract.freeCamera) || Flag(camera, contract.fixedCamera) || State(camera) != L"s_Following")
            return 1.0f;
        return SCGameplayFOV::Factor(State(controller), factor);
    }
} // namespace DAGameplayCamera
