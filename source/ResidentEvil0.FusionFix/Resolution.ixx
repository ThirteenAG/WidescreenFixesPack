module;

#include <stdafx.h>
#include <d3d9.h>
#include <map>

export module Resolution;

import EngineText;

export namespace RE0Resolution
{
    inline SafetyHookMid countHook, enumHook;
    inline thread_local std::vector<UINT> indices;
    inline void* catalog = nullptr;

    // The game accepts a saved size only if it appears in its enumerated catalog. Windowed D3D9 can also use
    // dimensions that are not display modes, so add the saved size to the native catalog before it is selected.
    inline void AddWindowedResolution(const char* saved)
    {
        unsigned width = 0, height = 0; char extra = 0;
        if (!catalog || sscanf_s(saved, "%ux%u%c", &width, &height, &extra, 1u) != 2 ||
            width < 640 || height < 480 || width > 8192 || height > 8192)
            return;
        auto base = static_cast<uint8_t*>(catalog);
        auto& count = *reinterpret_cast<uint32_t*>(base + 0x443444);
        auto& unique = *reinterpret_cast<uint32_t*>(base + 0x447E10);
        auto list = reinterpret_cast<uintptr_t*>(base + 0x443448);
        for (uint32_t i = 0; i < unique; ++i)
            if (std::strcmp(reinterpret_cast<const char*>(list[i * 2]), saved) == 0) return;
        if (!count || count >= 256 || unique >= 256) return;
        auto entry = base + 0x43D844 + count * 92;
        auto first = base + 0x43D844;
        sprintf_s(reinterpret_cast<char*>(entry), 64, "%ux%u", width, height);
        std::memcpy(entry + 64, first + 64, 16);
        *reinterpret_cast<uint32_t*>(entry + 80) = width;
        *reinterpret_cast<uint32_t*>(entry + 84) = height;
        *reinterpret_cast<float*>(entry + 88) = *reinterpret_cast<float*>(first + 88);
        list[unique * 2] = reinterpret_cast<uintptr_t>(entry);
        list[unique * 2 + 1] = unique;
        ++unique;
        list[unique * 2] = 0;
        ++count;
    }

    inline void Collect(IDirect3D9* core, UINT adapter, UINT count)
    {
        indices.clear();
        if (!core || count > 65536) return;
        std::map<std::pair<UINT, UINT>, std::pair<UINT, UINT>> modes;
        for (UINT i = 0; i < count; ++i)
        {
            D3DDISPLAYMODE mode{};
            if (FAILED(core->EnumAdapterModes(adapter, D3DFMT_X8R8G8B8, i, &mode)) ||
                mode.Width < 640 || mode.Height < 480 || mode.Width > 8192 || mode.Height > 8192) continue;
            const auto key = std::pair{ mode.Width, mode.Height };
            const auto found = modes.find(key);
            if (found == modes.end() || mode.RefreshRate > found->second.first)
                modes[key] = { mode.RefreshRate, i };
        }
        // The native catalog has room for 256 sizes, including their separate refresh labels.
        for (const auto& [size, value] : modes)
        {
            if (indices.size() == 256)
            {
                OutputDebugStringA("Resident Evil 0: display modes exceed the native catalog capacity.\n");
                break;
            }
            indices.push_back(value.second);
        }
    }

    inline void Install()
    {
        auto count = hook::pattern("FF 52 18 8B D8 33 FF 85 DB 0F 84");
        auto enumeration = hook::pattern("51 57 6A 16 55 52 FF 50 1C 50 8B CE");
        auto limits = hook::pattern("F6 C2 07 0F 85 ? ? ? ? 81 FA 80 02 00 00 72 ? 8B 44 24 74 3D E0 01 00 00 72 ? 81 FA 00 10 00 00 77 ? 3D 70 08 00 00 77 ?");
        countHook = safetyhook::create_mid(count.get_first(3), [](SafetyHookContext& context)
        {
            const auto value = reinterpret_cast<void*>(context.esi);
            catalog = value;
            Collect(RE0Text::Read<IDirect3D9*>(value, 160), RE0Text::Read<UINT>(value, 0x447DEC), context.eax);
            context.eax = static_cast<uint32_t>(indices.size());
        });
        enumHook = safetyhook::create_mid(enumeration.get_first(6), [](SafetyHookContext& context)
        {
            const auto index = RE0Text::Read<UINT>(reinterpret_cast<void*>(context.esp), 12);
            if (index < indices.size())
                RE0Text::Write(reinterpret_cast<void*>(context.esp), 12, indices[index]);
        });
        injector::MakeNOP(limits.get_first(3), 6, true);
        injector::WriteMemory<uint32_t>(limits.get_first<uint32_t>(30), 8192, true);
        injector::WriteMemory<uint32_t>(limits.get_first<uint32_t>(37), 8192, true);
    }
}
