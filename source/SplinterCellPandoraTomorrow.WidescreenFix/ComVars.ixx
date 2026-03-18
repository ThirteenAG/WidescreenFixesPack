module;

#include "stdafx.h"
#include <queue>
#include <functional>
#include <unordered_set>

export module ComVars;

export struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHUDScaleX;
    double dHUDScaleX;
    double dHUDScaleXInv;
    float fHudOffset;
    float fHudOffsetRight;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
    std::optional<float> fHudAspectRatioConstraint;
    float fWidescreenHudOffset;
    int nHudWidescreenMode;
    bool bOpsatWidescreenMode;
    bool bScopeWidescreenMode;
    uint32_t nPostProcessFixedScale;
    uint32_t nFMVWidescreenMode;
    uint32_t nShadowMapResolution;
    uint32_t nReflectionsResolution;
    uint32_t nBloomResolutionMultiplier;
    float fRawInputMouse;
    bool bRawInputMouseRawData;
    bool bDeferredInput;
    int nCutsceneBorders;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
} gColor;

export bool bSkipIntro = false;
export bool bPlayingVideo = false;
export bool bDisplayingBackground = false;
export bool bPressStartToContinue = false;
export bool bSkipPressStartToContinue = false;
export bool bRestoreCutsceneFOV = false;
export HWND hGameWindow = NULL;
export bool bIsWindowed = false;

export std::vector<std::pair<const std::wstring, std::wstring>> ResList =
{
    { L"640x480",   L"" },
    { L"800x600",   L"" },
    { L"1024x768",  L"" },
    { L"1280x1024", L"" },
    { L"1600x1200", L"" },
};

export namespace UWindowsViewport
{
    std::queue<std::function<void()>> deferredCauseInputEvent;
    std::function<void(int inputID, int a3, float value)> deferredCauseInputEventForRawInput;
}

export namespace UObject
{
    std::unordered_map<std::wstring, std::wstring> objectStates;
    std::wstring_view GetState(const std::wstring& type)
    {
        auto it = objectStates.find(type);
        return (it != objectStates.end()) ? std::wstring_view(it->second) : std::wstring_view(L"");
    }

    wchar_t* (__fastcall* GetFullName)(void*, void*, wchar_t*) = nullptr;
    void* (__fastcall* FindState)(void*, void*, int) = nullptr;
}

export int curDrawTileManagerTextureIndex = -1;
export std::wstring curDrawTileManagerTextureName;

export struct FArray
{
    void* Array;
    unsigned int Count;
    unsigned int Most;
};

export struct FName
{
    int Index;
};

export template<typename GetterT>
struct UOverride
{
    using Getter = GetterT;

    static inline std::unordered_map<std::wstring, Getter> ByName;
    static inline std::unordered_map<void*, Getter>        ByPropPtr;

    // Names that are registered but not yet bound to a prop pointer.
    static inline std::unordered_set<std::wstring>         PendingNames;

    static inline void Register(const std::wstring& name, Getter getter)
    {
        ByName[name] = getter;
        PendingNames.insert(name);
    }

    static inline Getter ResolveFast(void* propPtr)
    {
        if (auto it = ByPropPtr.find(propPtr); it != ByPropPtr.end())
            return it->second;
        return nullptr;
    }

    static inline bool AllCached()
    {
        // When no registered names remain unbound, there's nothing left to resolve by name.
        return PendingNames.empty();
    }

    static inline Getter ResolveAndBind(void* propPtr, std::wstring_view fullName)
    {
        if (auto cached = ResolveFast(propPtr))
            return cached;

        // If we've already cached every registered override, skip the name work entirely.
        if (AllCached() || fullName.empty())
            return nullptr;

        if (auto it = ByName.find(std::wstring(fullName)); it != ByName.end())
        {
            PendingNames.erase(it->first); // mark this registration as bound
            return ByPropPtr.emplace(propPtr, it->second).first->second;
        }
        return nullptr;
    }

    static inline void ClearCache()
    {
        ByPropPtr.clear();

        // Rebuild pending set from all registered names since nothing is bound anymore.
        PendingNames.clear();
        for (const auto& kv : ByName)
            PendingNames.insert(kv.first);
    }

    template<class Registry, class T>
    static inline void ApplyScalarOverride(void* prop, void* edx, T* dst)
    {
        if (auto g = Registry::ResolveFast(prop))
        {
            *dst = g(); return;
        }

        // If all registered overrides are already cached, skip full-name resolution.
        if (Registry::AllCached() || Registry::ByName.empty())
            return;

        wchar_t buffer[256];
        if (auto g = Registry::ResolveAndBind(prop, UObject::GetFullName(prop, edx, buffer)))
        {
            *dst = g();
        }
    }

    template<class Registry>
    static inline void ApplyArrayOverride(void* prop, void* edx, FArray* dst)
    {
        if (!dst || !dst->Array || !dst->Count) return;

        auto g = Registry::ResolveFast(prop);
        if (!g)
        {
            // If all registered overrides are already cached, skip full-name resolution.
            if (Registry::AllCached() || Registry::ByName.empty())
                return;

            wchar_t buffer[256];
            g = Registry::ResolveAndBind(prop, UObject::GetFullName(prop, edx, buffer));
        }
        if (!g) return;

        const auto src = g();
        if (!src.Array || !src.Count) return;

        // Inner element size: *(uint16_t*)(Inner + 58)
        auto GetArrayElemSize = [](void* uArrayProperty) -> unsigned int
        {
            // Inner pointer is at (this + 21*sizeof(void*))
            const auto inner = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(uArrayProperty) + 21u * sizeof(uintptr_t));
            return *reinterpret_cast<const uint16_t*>(inner + 58);
        };

        const auto elemSize = GetArrayElemSize(prop);
        const auto n = std::min(dst->Count, src.Count);
        if (n && elemSize)
            std::memcpy(dst->Array, src.Array, static_cast<size_t>(n) * elemSize);
    }
};

// Type aliases for current override kinds.
export using UIntOverrides = UOverride<int(*)()>;
export using UFloatOverrides = UOverride<float(*)()>;
export using UByteOverrides = UOverride<uint8_t(*)()>;
export using UNameOverrides = UOverride<FName(*)()>;
export using UObjectOverrides = UOverride<void* (*)()>;
export using UArrayOverrides = UOverride<FArray(*)()>;

export class ScriptConstants
{
public:
    union OverrideValue
    {
        int32_t asInt;
        float   asFloat;
        uint32_t raw;
    };

    using OverrideFunc = float(__cdecl*)();

    static constexpr int MAX_OVERRIDES = 64;
    static constexpr uint32_t MAGIC = 0x7FC20000u;

    static inline OverrideValue StaticOverrides[MAX_OVERRIDES] = {};
    static inline OverrideFunc  DynamicOverrides[MAX_OVERRIDES] = {};

    static inline bool IsOverrideTag(uint32_t bits)
    {
        return (bits & 0xFFFF0000u) == MAGIC;
    }

    static inline uint32_t DecodeOverrideID(uint32_t bits)
    {
        return bits & 0x0000FFFFu;
    }

    static inline uint32_t EncodeOverrideID(uint32_t id)
    {
        return MAGIC | (id & 0x0000FFFFu);
    }

    static void SetStaticFloat(uint32_t id, float value)
    {
        if (id < MAX_OVERRIDES)
        {
            StaticOverrides[id].asFloat = value;
            DynamicOverrides[id] = nullptr;
        }
    }

    static void SetStaticInt(uint32_t id, int32_t value)
    {
        if (id < MAX_OVERRIDES)
        {
            StaticOverrides[id].asInt = value;
            DynamicOverrides[id] = nullptr;
        }
    }

    static void SetDynamicFloat(uint32_t id, OverrideFunc func)
    {
        if (id < MAX_OVERRIDES)
            DynamicOverrides[id] = func;
    }

    static void SetDynamicInt(uint32_t id, int32_t(__cdecl* func)())
    {
        if (id < MAX_OVERRIDES)
            DynamicOverrides[id] = reinterpret_cast<OverrideFunc>(func);
    }

    //ID  0: 00 00 C2 7F
    //ID  1: 01 00 C2 7F
    //ID  2: 02 00 C2 7F
    //ID  3: 03 00 C2 7F
    //ID  4: 04 00 C2 7F
    //ID  5: 05 00 C2 7F
    //ID  6: 06 00 C2 7F
    //ID  7: 07 00 C2 7F
    //ID  8: 08 00 C2 7F
    //ID  9: 09 00 C2 7F
    //ID 10: 0A 00 C2 7F
    //ID 11: 0B 00 C2 7F
    //ID 12: 0C 00 C2 7F
    //ID 13: 0D 00 C2 7F
    //ID 14: 0E 00 C2 7F
    //ID 15: 0F 00 C2 7F
    //ID 16: 10 00 C2 7F
    //ID 17: 11 00 C2 7F
    //ID 18: 12 00 C2 7F
    //ID 19: 13 00 C2 7F
    //ID 20: 14 00 C2 7F
    //ID 21: 15 00 C2 7F
    //ID 22: 16 00 C2 7F
    //ID 23: 17 00 C2 7F
    //ID 24: 18 00 C2 7F
    //ID 25: 19 00 C2 7F
    //ID 26: 1A 00 C2 7F
    //ID 27: 1B 00 C2 7F
    //ID 28: 1C 00 C2 7F
    //ID 29: 1D 00 C2 7F
    //ID 30: 1E 00 C2 7F
    //ID 31: 1F 00 C2 7F
    //ID 32: 20 00 C2 7F
    //ID 33: 21 00 C2 7F
    //ID 34: 22 00 C2 7F
    //ID 35: 23 00 C2 7F
    //ID 36: 24 00 C2 7F
    //ID 37: 25 00 C2 7F
    //ID 38: 26 00 C2 7F
    //ID 39: 27 00 C2 7F
    //ID 40: 28 00 C2 7F
    //ID 41: 29 00 C2 7F
    //ID 42: 2A 00 C2 7F
    //ID 43: 2B 00 C2 7F
    //ID 44: 2C 00 C2 7F
    //ID 45: 2D 00 C2 7F
    //ID 46: 2E 00 C2 7F
    //ID 47: 2F 00 C2 7F
    //ID 48: 30 00 C2 7F
    //ID 49: 31 00 C2 7F
    //ID 50: 32 00 C2 7F
    //ID 51: 33 00 C2 7F
    //ID 52: 34 00 C2 7F
    //ID 53: 35 00 C2 7F
    //ID 54: 36 00 C2 7F
    //ID 55: 37 00 C2 7F
    //ID 56: 38 00 C2 7F
    //ID 57: 39 00 C2 7F
    //ID 58: 3A 00 C2 7F
    //ID 59: 3B 00 C2 7F
    //ID 60: 3C 00 C2 7F
    //ID 61: 3D 00 C2 7F
    //ID 62: 3E 00 C2 7F
    //ID 63: 3F 00 C2 7F
};