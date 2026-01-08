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
    uint32_t nPostProcessFixedScale;
    uint32_t nFMVWidescreenMode;
    uint32_t nShadowMapResolution;
    uint32_t nReflectionsResolution;
    uint32_t nBloomResolution;
    float fRawInputMouse;
    bool bRawInputMouseRawData;
    bool bDeferredInput;
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