module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstdint>
#include <atomic>
#include <array>

export module Camera;


export namespace FarCryCameraFrame
{
    struct Source
    {
        const void* camera{};
        uint64_t epoch{};
        float authored{}, applied{};
        bool valid = false;

        float Read(const void* current, uint64_t generation, float encoded)
        {
            if (!valid || camera != current || epoch != generation || encoded != applied)
            {
                camera = current;
                epoch = generation;
                authored = encoded;
                valid = true;
            }
            return authored;
        }
    };

    inline float Horizontal(float encoded, float aspect)
    {
        return 2.0f * std::atan(std::tan(encoded / aspect * 0.5f) * aspect);
    }

    inline float Bound(float encoded, float aspect)
    {
        if (!std::isfinite(encoded) || !std::isfinite(aspect) || encoded <= 0 || aspect <= 0)
            return encoded;
        constexpr float limit = 150.0f * std::numbers::pi_v<float> / 180.0f;
        // The native frustum uses tan(encoded / aspect / 2).
        const auto maximum = 2.0f * std::atan(std::tan(limit * 0.5f) / aspect) * aspect;
        return std::min(encoded, maximum);
    }
}

export namespace FarCryCinematicFrame
{
    constexpr float Reference = 4.0f / 3.0f;

    struct Borders
    {
        float side{}, top{};
    };

    inline Borders Layout(float aspect, int mode, float nativeBar, float progress)
    {
        if (!std::isfinite(aspect) || aspect <= 0)
            return {};
        progress = std::clamp(progress, 0.0f, 1.0f);
        const auto side = (mode & 2) && aspect > Reference ? (1.0f - Reference / aspect) * 0.5f : 0.0f;
        const auto outer = (mode & 1) && aspect < Reference ? (1.0f - aspect / Reference) * 0.5f : 0.0f;
        const auto top = (mode & 1) ? outer + (1.0f - 2.0f * outer) * std::clamp(nativeBar, 0.0f, 0.49f) : 0.0f;
        return { side * progress, top * progress };
    }

    inline float Camera(float encoded, float aspect, int mode, float progress)
    {
        if (!std::isfinite(encoded) || !std::isfinite(aspect) || encoded <= 0 || aspect <= 0)
            return encoded;
        const auto vertical = encoded / Reference;
        const auto outer = (mode & 1) && aspect < Reference ?
            (1.0f - aspect / Reference) * std::clamp(progress, 0.0f, 1.0f) : 0.0f;
        return 2.0f * std::atan(std::tan(vertical * 0.5f) / (1.0f - outer)) * aspect;
    }
}

export namespace FarCryCinematics
{
    inline SafetyHookInline BeginHook, EndHook;
    inline std::atomic<int> Depth{ 0 };
    inline std::atomic<float> NativeBar{ 0.0f };
    inline std::atomic<ULONGLONG> Changed{ 0 };
    inline bool Ready = false;
    inline bool DeviceReady = false, DrawReady = false;

    inline bool Active() { return Ready && DeviceReady && DrawReady && Depth.load() > 0; }

    inline float Progress()
    {
        if (!FixSettings().Bool("MAIN.AnimatedCutsceneBorders"))
            return Active() ? 1.0f : 0.0f;
        const auto elapsed = float(GetTickCount64() - Changed.load()) / 250.0f;
        return Active() ? std::clamp(elapsed, 0.0f, 1.0f) : std::clamp(1.0f - elapsed, 0.0f, 1.0f);
    }

    inline void Begun(void* object, unsigned flags)
    {
#ifdef _WIN64
        constexpr size_t offset = 0x18;
#else
        constexpr size_t offset = 0x0C;
#endif
        const auto depth = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(object) + offset);
        if (Depth.exchange(std::max(0, depth)) == 0 && depth > 0)
            Changed.store(GetTickCount64());
        NativeBar.store((flags & 0x100) ? 1.0f / 6.0f : 0.0f);
    }

    inline void Ended(void* object)
    {
#ifdef _WIN64
        constexpr size_t offset = 0x18;
#else
        constexpr size_t offset = 0x0C;
#endif
        const auto depth = std::max(0, *reinterpret_cast<const int*>(static_cast<const uint8_t*>(object) + offset));
        if (Depth.exchange(depth) > 0 && depth == 0)
            Changed.store(GetTickCount64());
    }

#ifdef _WIN64
    inline void Begin(void* object, unsigned flags, bool reset)
    {
        BeginHook.call<void>(object, flags, reset);
        Begun(object, flags);
    }
    inline void End(void* object)
    {
        EndHook.call<void>(object);
        Ended(object);
    }
#else
    inline void __fastcall Begin(void* object, void*, unsigned flags, bool reset)
    {
        BeginHook.unsafe_fastcall<void>(object, nullptr, flags, reset);
        Begun(object, flags);
    }
    inline void __fastcall End(void* object, void*)
    {
        EndHook.unsafe_fastcall<void>(object, nullptr);
        Ended(object);
    }
#endif

    inline bool StockBar(float x, float y, float width, float height)
    {
        if (!Active() || x != 0.0f || width != 800.0f || height <= 0.0f || height >= 300.0f ||
            (y != 0.0f && std::abs(y + height - 600.0f) > 0.01f))
            return false;
        NativeBar.store(height / 600.0f);
        return true;
    }

    inline float Camera(float encoded, float aspect)
    {
        return FarCryCinematicFrame::Camera(encoded, aspect, FixSettings().Int("MAIN.CutsceneBorders"), Progress());
    }

    inline void Draw(IDirect3DDevice9* device)
    {
        if (!Ready || !DeviceReady || !DrawReady || !device || Progress() <= 0.0f || !FixSettings().Int("MAIN.CutsceneBorders"))
            return;
        IDirect3DSurface9* backbuffer = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer)))
            return;
        D3DSURFACE_DESC desc{};
        backbuffer->GetDesc(&desc);
        const auto borders = FarCryCinematicFrame::Layout(float(desc.Width) / desc.Height,
            FixSettings().Int("MAIN.CutsceneBorders"), NativeBar.load(), Progress());
        const auto side = LONG(std::lround(desc.Width * borders.side));
        const auto top = LONG(std::lround(desc.Height * borders.top));
        if (!side && !top)
        {
            backbuffer->Release();
            return;
        }
        D3DVIEWPORT9 viewport{};
        D3DCAPS9 caps{};
        if (FAILED(device->GetViewport(&viewport)) || FAILED(device->GetDeviceCaps(&caps)))
        {
            backbuffer->Release();
            return;
        }
        std::array<IDirect3DSurface9*, 4> targets{};
        const auto count = std::min<UINT>(4, caps.NumSimultaneousRTs);
        if (!count || FAILED(device->GetRenderTarget(0, &targets[0])))
        {
            backbuffer->Release();
            return;
        }
        for (UINT i = 1; i < count; ++i)
            device->GetRenderTarget(i, &targets[i]);
        IDirect3DSurface9* depth = nullptr;
        device->GetDepthStencilSurface(&depth);
        device->SetDepthStencilSurface(nullptr);
        for (UINT i = 1; i < count; ++i)
            device->SetRenderTarget(i, nullptr);
        if (SUCCEEDED(device->SetRenderTarget(0, backbuffer)))
        {
            const D3DVIEWPORT9 whole{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
            device->SetViewport(&whole);
            std::array<D3DRECT, 4> rectangles{};
            DWORD size = 0;
            if (side)
            {
                rectangles[size++] = { 0, 0, side, LONG(desc.Height) };
                rectangles[size++] = { LONG(desc.Width) - side, 0, LONG(desc.Width), LONG(desc.Height) };
            }
            if (top)
            {
                rectangles[size++] = { 0, 0, LONG(desc.Width), top };
                rectangles[size++] = { 0, LONG(desc.Height) - top, LONG(desc.Width), LONG(desc.Height) };
            }
            device->Clear(size, rectangles.data(), D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 1.0f, 0);
        }
        for (UINT i = 0; i < count; ++i)
        {
            device->SetRenderTarget(i, targets[i]);
            if (targets[i])
                targets[i]->Release();
        }
        device->SetDepthStencilSurface(depth);
        if (depth)
            depth->Release();
        device->SetViewport(&viewport);
        backbuffer->Release();
    }

    inline bool Init(HMODULE module)
    {
#ifdef _WIN64
        auto begin = hook::module_pattern(module, "48 83 EC 38 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 40 48 89 6C 24 48 48 89 74 24 50 48 89 7C 24 58 4C 89 64 24 30 45 0F B6 E0 8B EA 48 8B F9 83 79 18 00");
        auto end = hook::module_pattern(module, "48 83 EC 38 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 48 48 89 74 24 50 48 89 7C 24 58 48 8B F9 FF 49 18 83 79 18 00");
#else
        auto begin = hook::module_pattern(module, "B8 ? ? ? ? E8 ? ? ? ? 51 56 8B F1 83 7E 0C 00 7E 05 E8 ? ? ? ? 8B 46 08 FF 46 0C");
        auto end = hook::module_pattern(module, "B8 ? ? ? ? E8 ? ? ? ? 51 53 56 8B F1 FF 4E 0C 8B 46 0C 33 DB 3B C3 0F 8F");
#endif
        BeginHook = safetyhook::create_inline(begin.get_first(), Begin);
        EndHook = safetyhook::create_inline(end.get_first(), End);
        Ready = bool(BeginHook) && bool(EndHook);
        if (!Ready)
        {
            BeginHook.reset();
            EndHook.reset();
        }
        return Ready;
    }
}

export namespace FarCryCamera
{
    inline SafetyHookInline ViewHook;
    inline SafetyHookMid ClientHook;
    inline uintptr_t ClientReturn = 0;
    inline std::atomic<bool> Gameplay{ false };
    inline thread_local const void* ExpectedView = nullptr;
    inline thread_local bool ExpectedGameplay = false;
    inline thread_local uint64_t Epoch = 0;
    inline thread_local FarCryCameraFrame::Source Source;

    inline bool PlayerCamera(const void* client, const void* player)
    {
        if (!client || !player)
            return false;
        const auto input = static_cast<const uint8_t*>(client);
        const auto actor = static_cast<const uint8_t*>(player);
#ifdef _WIN64
        const auto override = *reinterpret_cast<const uint8_t* const*>(input + 736);
        return override && !*reinterpret_cast<const uint16_t*>(override + 8) &&
            *reinterpret_cast<const int*>(actor + 168) > 0 && !actor[214];
#else
        const auto override = *reinterpret_cast<const uint8_t* const*>(input + 552);
        return override && !*reinterpret_cast<const uint16_t*>(override + 4) &&
            *reinterpret_cast<const int*>(actor + 160) > 0 && !actor[206];
#endif
    }

    inline void Client(SafetyHookContext& regs)
    {
#ifdef _WIN64
        ExpectedView = reinterpret_cast<const void*>(regs.rsp + 0x500);
        ExpectedGameplay = PlayerCamera(reinterpret_cast<const void*>(regs.rbp), *reinterpret_cast<const void* const*>(regs.rsp + 0x38));
#else
        ExpectedView = reinterpret_cast<const void*>(regs.ebp - 0x240);
        ExpectedGameplay = PlayerCamera(reinterpret_cast<const void*>(regs.esi), *reinterpret_cast<const void* const*>(regs.ebp - 0x30));
#endif
    }

    inline void Submitted(const void* camera, uintptr_t caller)
    {
        const bool submitted = camera && camera == ExpectedView && caller == ClientReturn;
        if (submitted)
            ++Epoch;
        Gameplay.store(submitted && ExpectedGameplay);
        ExpectedView = nullptr;
        ExpectedGameplay = false;
    }

#ifndef _WIN64
    inline uintptr_t __fastcall SetView(void* system, void*, const void* camera)
    {
        Submitted(camera, reinterpret_cast<uintptr_t>(_ReturnAddress()));
        return ViewHook.unsafe_fastcall<uintptr_t>(system, nullptr, camera);
    }
#else
    inline uintptr_t __fastcall SetView(void* system, const void* camera)
    {
        Submitted(camera, reinterpret_cast<uintptr_t>(_ReturnAddress()));
        return ViewHook.unsafe_fastcall<uintptr_t>(system, camera);
    }
#endif

    inline float Factor(float aspectFactor, float userFactor)
    {
        return aspectFactor * (Gameplay.load() ? userFactor : 1.0f);
    }

    inline float Target(float encoded, float aspect, float factor)
    {
        if (FarCryCinematics::Active())
            return FarCryCinematics::Camera(encoded, aspect);
        const bool gameplay = Gameplay.load();
        const auto target = encoded * (aspect / (4.0f / 3.0f)) * (gameplay ? factor : 1.0f);
        return gameplay ? FarCryCameraFrame::Bound(target, aspect) : target;
    }

    inline void Apply(float* camera, float aspect, float factor)
    {
        const auto authored = Source.Read(camera, Epoch, camera[12]);
        Source.applied = Target(authored, aspect, factor);
        injector::WriteMemory<float>(camera + 12, Source.applied, true);
    }

    inline void Reset()
    {
        ClientHook.reset();
        ViewHook.reset();
        ClientReturn = 0;
        Gameplay.store(false);
        ExpectedView = nullptr;
        ExpectedGameplay = false;
        Source = {};
    }

    inline bool Install(HMODULE game, HMODULE system)
    {
        if (ClientHook && ViewHook)
            return true;
        if (!game || !system)
            return false;
#ifdef _WIN64
        auto client = hook::module_pattern(game, "48 8B 45 78 48 8B 48 40 48 8B 01 48 8D 94 24 00 05 00 00 FF 90 A0 01 00 00 80 BD 80 00 00 00 00");
        auto view = hook::module_pattern(system, "48 81 C1 B8 24 00 00 E9 ? ? ? ? CC CC CC CC 48 8D 81 B8 24 00 00 C3");
        auto player = hook::module_pattern(game, "48 8B 00 4C 8D 44 24 38 33 D2 48 8B CF FF 50 50 84 C0 74 ? 48 8B 4C 24 38 44 88 B1 22 01 00 00");
        constexpr ptrdiff_t callOffset = 19, hookOffset = 11;
#else
        auto client = hook::module_pattern(game, "8B 46 44 8B 48 24 8B 01 8D 95 C0 FD FF FF 52 FF 90 D0 00 00 00 80 7E 48 00");
        auto view = hook::module_pattern(system, "81 C1 A8 24 00 00 E9 ? ? ? ? 55 8B EC 8B C1 8B 4D 08 53 56 57");
        auto player = hook::module_pattern(game, "8B 07 8D 4D D0 51 53 8B CF FF 50 28 84 C0 74 ? 8B 45 D0 C6 80 1A 01 00 00 00");
        constexpr ptrdiff_t callOffset = 15, hookOffset = 8;
#endif
        player.get_first();
        const auto call = client.get_first<uint8_t>(callOffset);
        ClientReturn = reinterpret_cast<uintptr_t>(call + 6);
        ViewHook = safetyhook::create_inline(view.get_first(), SetView);
        if (ViewHook)
            ClientHook = safetyhook::create_mid(client.get_first(hookOffset), Client);
        if (!ClientHook || !ViewHook)
        {
            Reset();
            OutputDebugStringA("Far Cry: camera ownership hooks unavailable; gameplay FOV multiplier disabled.\n");
            return false;
        }
        return true;
    }
}
