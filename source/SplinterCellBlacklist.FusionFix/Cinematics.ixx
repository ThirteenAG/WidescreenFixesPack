module;

#include <stdafx.h>
#include <stdexcept>
#include <d3d9.h>
#include <d3d11_1.h>
#include <intrin.h>
#include <mutex>
#include "FixSettings.hpp"

export module Cinematics;

import ComVars;
import CinematicFrame;

namespace
{
    struct Shot
    {
        uintptr_t viewport = 0, controller = 0;
        RECT area{};
        int width = 0, height = 0;
        bool active = false;
    };
    std::array<Shot, 8> Shots;
    std::mutex ShotMutex;
    thread_local Shot Current;
    std::atomic<bool> Ready{ false }, Covered{ true };
    uintptr_t(__cdecl* Engine)() = nullptr;
    const DWORD* TlsIndex = nullptr;
    bool Dx9 = false;
    SafetyHookInline ProjectionHook;
    SafetyHookMid HudHook;

    bool Live(const Shot& shot)
    {
        const auto engine = Engine ? Engine() : 0;
        const auto client = engine ? *reinterpret_cast<const uintptr_t*>(engine + 0x44) : 0;
        const auto count = client ? *reinterpret_cast<const int32_t*>(client + 0x30) : 0;
        const auto views = client ? *reinterpret_cast<const uintptr_t* const*>(client + 0x2C) : nullptr;
        if (!views || count < 1 || count > int(Shots.size()))
            return false;
        for (int32_t i = 0; i < count; ++i)
            if (views[i] == shot.viewport)
                return (bSplitscreen || i == 0) && *reinterpret_cast<const uintptr_t*>(shot.viewport + 0x30) == shot.controller;
        return false;
    }

    unsigned Rectangles(unsigned width, unsigned height, std::array<RECT, 16>& rectangles)
    {
        if (Covered.load())
            return 0;
        const auto borders = FixSettings().Int("MAIN.CutsceneBorders");
        if (!borders)
            return 0;
        std::array<Shot, 8> shots;
        {
            std::lock_guard lock(ShotMutex);
            shots = Shots;
        }
        unsigned count = 0;
        for (const auto& shot : shots)
        {
            if (!shot.active || !Live(shot))
                continue;
            const auto frame = CinematicFrame::Fit(float(shot.area.right - shot.area.left), float(shot.area.bottom - shot.area.top), borders);
            const auto x = [&](float value) { return std::clamp(LONG(std::lround(value * width / shot.width)), 0L, LONG(width)); };
            const auto y = [&](float value) { return std::clamp(LONG(std::lround(value * height / shot.height)), 0L, LONG(height)); };
            if (frame.left > 0)
            {
                rectangles[count++] = { x(float(shot.area.left)), y(float(shot.area.top)), x(shot.area.left + frame.left), y(float(shot.area.bottom)) };
                rectangles[count++] = { x(shot.area.left + frame.right), y(float(shot.area.top)), x(float(shot.area.right)), y(float(shot.area.bottom)) };
            }
            else if (frame.top > 0)
            {
                rectangles[count++] = { x(float(shot.area.left)), y(float(shot.area.top)), x(float(shot.area.right)), y(shot.area.top + frame.top) };
                rectangles[count++] = { x(float(shot.area.left)), y(shot.area.top + frame.bottom), x(float(shot.area.right)), y(float(shot.area.bottom)) };
            }
        }
        return count;
    }

    void BeforeHud(uintptr_t device)
    {
        if (!Ready.load() || Covered.load())
            return;
        std::array<RECT, 16> rectangles{};
        if (Dx9)
        {
            auto native = *reinterpret_cast<IDirect3DDevice9**>(device + 0x34);
            if (!native)
                return;
            IDirect3DSurface9* target = nullptr;
            if (FAILED(native->GetRenderTarget(0, &target)))
                return;
            D3DSURFACE_DESC description{};
            const bool valid = SUCCEEDED(target->GetDesc(&description));
            target->Release();
            if (!valid)
                return;
            const auto count = Rectangles(description.Width, description.Height, rectangles);
            D3DVIEWPORT9 previous{};
            if (!count || FAILED(native->GetViewport(&previous)))
                return;
            const D3DVIEWPORT9 full{ 0, 0, description.Width, description.Height, 0.0f, 1.0f };
            if (SUCCEEDED(native->SetViewport(&full)))
            {
                native->Clear(count, reinterpret_cast<const D3DRECT*>(rectangles.data()), D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
                native->SetViewport(&previous);
            }
            return;
        }
        // Use the context and target of this native HUD pass, including deferred contexts.
        const auto tls = reinterpret_cast<uintptr_t*>(__readfsdword(0x2C));
        const auto block = TlsIndex && tls ? tls[*TlsIndex] : 0;
        const auto context = block ? *reinterpret_cast<ID3D11DeviceContext**>(block + 340) : nullptr;
        const auto surface = *reinterpret_cast<const uintptr_t*>(device + 124);
        const auto resource = surface ? *reinterpret_cast<const uintptr_t*>(surface + 8) : 0;
        const auto view = resource ? *reinterpret_cast<ID3D11RenderTargetView**>(resource + 24) : nullptr;
        if (!context || !view)
            return;
        ID3D11Resource* texture = nullptr;
        view->GetResource(&texture);
        if (!texture)
            return;
        ID3D11Texture2D* image = nullptr;
        const auto result = texture->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&image));
        texture->Release();
        if (FAILED(result))
            return;
        D3D11_TEXTURE2D_DESC description{};
        image->GetDesc(&description);
        image->Release();
        const auto count = Rectangles(description.Width, description.Height, rectangles);
        if (!count)
            return;
        ID3D11DeviceContext1* clear = nullptr;
        if (SUCCEEDED(context->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&clear))))
        {
            constexpr float black[4]{ 0, 0, 0, 1 };
            clear->ClearView(view, black, rectangles.data(), count);
            clear->Release();
        }
        else
        {
            static std::once_flag warning;
            std::call_once(warning, []() { OutputDebugStringA("SplinterCellBlacklist: cinematic rectangle clearing unavailable.\n"); });
        }
    }

    char __fastcall Projection(float* input, void* edx, char flags)
    {
        if (!Ready.load() || !Current.active || !std::isfinite(input[0]) || input[0] <= 0 || input[0] >= 3.141593f)
            return ProjectionHook.fastcall<char>(input, edx, flags);
        std::array<float, 29> local{};
        std::copy_n(input, 13, local.data());
        local[3] = float(Current.area.right - Current.area.left);
        local[4] = float(Current.area.bottom - Current.area.top);
        // Native clipping is replaced only in this scene's temporary projection inputs.
        ProjectionHook.fastcall<char>(local.data(), edx, char(flags & ~5));
        std::copy_n(local.data() + 13, 16, input + 13);
        return flags;
    }
}

export namespace Cinematics
{
    class SceneScope
    {
        Shot previous;
    public:
        SceneScope(uintptr_t viewport, bool cinematic) : previous(Current)
        {
            Current = {};
            if (!Ready.load() || !viewport)
                return;
            Shot shot;
            shot.viewport = viewport;
            shot.controller = *reinterpret_cast<const uintptr_t*>(viewport + 0x30);
            if (cinematic)
            {
                if (!Live(shot))
                    return;
                const auto table = *reinterpret_cast<const uintptr_t* const*>(viewport);
                const auto window = reinterpret_cast<HWND(__thiscall*)(uintptr_t)>(table[0x100 / 4])(viewport);
                RECT client{};
                if (!window || window != WindowHandle || !GetClientRect(window, &client))
                    return;
                const int width = *reinterpret_cast<const int*>(viewport + 0x4A0);
                const int height = *reinterpret_cast<const int*>(viewport + 0x4A4);
                const int x = *reinterpret_cast<const int*>(viewport + 0x4A8);
                const int y = *reinterpret_cast<const int*>(viewport + 0x4AC);
                shot.width = client.right - client.left;
                shot.height = client.bottom - client.top;
                if (width > 0 && height > 0 && x >= 0 && y >= 0 && shot.width > 0 && shot.height > 0 &&
                    shot.width <= 32768 && shot.height <= 32768 && x <= shot.width - width && y <= shot.height - height)
                {
                    shot.area = { x, y, x + width, y + height };
                    shot.active = true;
                }
            }
            Current = shot;
            std::lock_guard lock(ShotMutex);
            auto selected = std::find_if(Shots.begin(), Shots.end(), [&](const Shot& value) { return value.viewport == viewport; });
            if (selected == Shots.end())
                selected = std::find_if(Shots.begin(), Shots.end(), [](const Shot& value) { return !value.active || !Live(value); });
            if (selected != Shots.end())
                *selected = shot;
        }
        ~SceneScope() { Current = previous; }
        SceneScope(const SceneScope&) = delete;
        SceneScope& operator=(const SceneScope&) = delete;
    };

    bool Active() { return Ready.load() && Current.active; }
    float Angle(float angle, float aspect, bool nativeCorrection)
    {
        return CinematicFrame::SceneInput(angle, aspect, nativeCorrection, FixSettings().Int("MAIN.CutsceneBorders"));
    }
    void SetEngine(uintptr_t(__cdecl* engine)()) { Engine = engine; }
    void SetCovered(bool covered) { Covered.store(covered); }
    void Video()
    {
        std::lock_guard lock(ShotMutex);
        for (auto& shot : Shots)
            shot.active = false;
    }

    void Init()
    {
        auto projection = hook::pattern("55 8B EC 81 EC 90 00 00 00 A1 ? ? ? ? 33 C5 89 45 FC 56 8B F1 D9 06 57 DC 0D ? ? ? ? D9 5D B8 D9 45 B8 E8 ? ? ? ? D9 5D B8 D9 45 B8 F3 0F 10 46 0C D9 E8 F3 0F 10 5E 10");
        auto hud11 = hook::pattern("8B 56 40 80 BA A8 05 00 00 00 74 40 F3 0F 10 05 ? ? ? ? 83 EC 08");
        auto hud9 = hook::pattern("8B 4E 40 80 B9 A4 05 00 00 00 74 40 F3 0F 10 05 ? ? ? ? 83 EC 08");
        Dx9 = !hud9.empty();
        if (!Dx9)
        {
            const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
            const auto tls = reinterpret_cast<const IMAGE_TLS_DIRECTORY*>(base + directory.VirtualAddress);
            TlsIndex = reinterpret_cast<const DWORD*>(tls->AddressOfIndex);
        }
        ProjectionHook = safetyhook::create_inline(projection.get_first(), Projection);
        HudHook = safetyhook::create_mid(Dx9 ? hud9.get_first() : hud11.get_first(), [](SafetyHookContext& regs) { BeforeHud(regs.edi); });
        Ready.store(true);
    }
}
