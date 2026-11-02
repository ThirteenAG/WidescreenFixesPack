module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Cinematics;

import ComVars;
import CinematicFrame;

namespace
{
    std::atomic<float> ShotAspect = 0, FrameAspect = 0;
    void* ShotClass = nullptr;
    uint16_t DefinitionOffset = 0, ControlOffset = 0;
    uint32_t ControlMask = 0;

    template<class T> T Read(void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }
    std::wstring Name(void* object)
    {
        if (!object || !UObject::GetFullName)
            return {};
        wchar_t buffer[256]{};
        const auto name = UObject::GetFullName(object, nullptr, buffer);
        return name ? name : L"";
    }
    void* Field(void* type, std::wstring_view name, std::wstring_view kind)
    {
        const int total = type ? Read<int>(type, 68) : 0;
        if (total <= 0 || total > 0x100000)
            return nullptr;
        for (unsigned depth = 0; type && depth < 32; ++depth, type = Read<void*>(type, 44))
        {
            unsigned count = 0;
            for (auto field = Read<void*>(type, 64); field && ++count <= 8192;)
            {
                const auto full = Name(field);
                const auto leaf = full.find_last_of(L". ");
                if (full.starts_with(std::wstring(kind) + L" ") && leaf != std::wstring::npos && full.substr(leaf + 1) == name)
                {
                    const auto size = Read<uint16_t>(field, 58), offset = Read<uint16_t>(field, 72);
                    return Read<uint16_t>(field, 56) == 1 && size >= 4 && size_t(offset) + size <= size_t(total) ? field : nullptr;
                }
                const auto next = Read<void*>(field, 48);
                if (next == field)
                    break;
                field = next;
            }
        }
        return nullptr;
    }
    bool Bind(void* shot)
    {
        const auto cls = Read<void*>(shot, 36);
        if (cls == ShotClass && ControlMask)
            return true;
        if (!Name(shot).starts_with(L"ECameraShot "))
            return false;
        const auto definition = Field(cls, L"CameraShot", L"StructProperty");
        const auto structure = definition ? Read<void*>(definition, 88) : nullptr;
        const auto control = Field(structure, L"AllowPlayerControlSam", L"BoolProperty");
        const auto mask = control ? Read<uint32_t>(control, 88) : 0;
        if (!mask || (mask & (mask - 1)))
            return false;
        ShotClass = cls; DefinitionOffset = Read<uint16_t>(definition, 72);
        ControlOffset = Read<uint16_t>(control, 72); ControlMask = mask;
        return true;
    }
}

export namespace Cinematics
{
    void Shot(void* shot, float reference)
    {
        const bool authored = shot && Bind(shot) &&
            !(Read<uint32_t>(shot, size_t(DefinitionOffset) + ControlOffset) & ControlMask);
        ShotAspect.store(authored ? reference : 0.0f, std::memory_order_relaxed);
    }
    float MainView()
    {
        const float aspect = !NativeMenuBlocksCinematics.load(std::memory_order_relaxed) ? ShotAspect.load(std::memory_order_relaxed) : 0.0f;
        FrameAspect.store(aspect, std::memory_order_relaxed);
        return aspect;
    }
    float ProjectionAspect(float reference)
    {
        return DACinematicFrame::Fit(Screen.fWidth, Screen.fHeight, reference, FixSettings().Int("MAIN.CutsceneBorders")).projectionAspect;
    }
    void Present(IDirect3DDevice9* device)
    {
        const auto reference = FrameAspect.exchange(0.0f, std::memory_order_relaxed);
        if (!device || reference <= 0 || NativeMenuBlocksCinematics.load(std::memory_order_relaxed))
            return;
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            return;
        D3DSURFACE_DESC desc{};
        if (SUCCEEDED(surface->GetDesc(&desc)))
        {
            const auto frame = DACinematicFrame::Fit(float(desc.Width), float(desc.Height), reference, FixSettings().Int("MAIN.CutsceneBorders"));
            const LONG width = LONG(desc.Width), height = LONG(desc.Height);
            const LONG x = std::clamp(LONG(std::lround(frame.left)), 0L, width / 2);
            const LONG y = std::clamp(LONG(std::lround(frame.top)), 0L, height / 2);
            const RECT bars[] = { { 0, 0, x, height }, { width - x, 0, width, height },
                { 0, 0, width, y }, { 0, height - y, width, height } };
            for (const auto& bar : bars)
                if (bar.right > bar.left && bar.bottom > bar.top)
                    device->ColorFill(surface, &bar, D3DCOLOR_XRGB(0, 0, 0));
        }
        surface->Release();
    }
}
