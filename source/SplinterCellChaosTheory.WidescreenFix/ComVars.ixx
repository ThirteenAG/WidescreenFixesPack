module;

#include "stdafx.h"
#include <unordered_set>

export module ComVars;

export struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHUDScaleX;
    float fHudOffset;
    const float fHUDScaleXOriginal = 0.003125f;
    const float fHudOffsetOriginal = 1.0f;
    float fHUDScaleXDyn;
    float fHudOffsetDyn;
    float fTextScaleX;
    int32_t nHudOffsetReal;
    int32_t nScopeScale;
    float fFMVoffsetStartX;
    float fFMVoffsetEndX;
    float fFMVoffsetStartY;
    float fFMVoffsetEndY;
    bool bRawInputMouseForMenu;
} Screen;

export union FColor
{
    uint32_t RGBA;
    struct
    {
        uint8_t B, G, R, A;
    };
};

export struct FLTColor
{
    float R, G, B, A = 1.0f;
    inline FLTColor()
    {
    }
    inline FLTColor(uint32_t color)
    {
        R = ((color >> 16) & 0xFF) / 255.0f;
        G = ((color >> 8) & 0xFF) / 255.0f;
        B = ((color) & 0xFF) / 255.0f;
        A = 1.0f;
    }
    bool empty()
    {
        return (R == 0.0f && G == 0.0f && B == 0.0f);
    }
    FLTColor& operator=(uint32_t color)
    {
        *this = FLTColor(color);
        return *this;
    }
};

export struct WidescreenHudOffset
{
    int32_t _int;
    float _float;
} WidescreenHudOffset;

export bool bHudWidescreenMode;
export int32_t nWidescreenHudOffset;
export float fWidescreenHudOffset;
export bool bDisableAltTabFix;
export int32_t nShadowMapResolution;
export bool bEnableShadowFiltering;
export bool bPlayingVideo = false;
export bool bHackingGameplay = false;

export uint32_t* dword_1120B6BC = nullptr;
export uint32_t* dword_1120B6B0 = nullptr;
export uint32_t* dword_11223A7C = nullptr;
export HWND hGameWindow = NULL;
export bool bIsWindowed = false;
export float* aMouseXPtr = nullptr;
export float* aMouseYPtr = nullptr;

export namespace UObject
{
    std::unordered_map<std::wstring, std::wstring> objectStates;
    std::wstring_view GetState(const std::wstring& type)
    {
        auto it = objectStates.find(type);
        return (it != objectStates.end()) ? std::wstring_view(it->second) : std::wstring_view(L"");
    }

    wchar_t* (__fastcall* GetFullName)(void*, void*, wchar_t*) = nullptr;
}

export std::unordered_map<uint32_t, bool> IsMenuDisplayedCache;

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
