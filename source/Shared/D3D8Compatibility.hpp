#pragma once
#include <d3d9.h>

namespace D3D8Compatibility
{
    inline SafetyHookInline Create9Hook, CreateDeviceHook, PresentHook, CreateTextureHook;
    inline bool HybridAdapter = false;
    inline bool GenerateMipmaps = false;
    inline int (*FrameLimit)() = nullptr;

    inline void SelectHybridAdapter(HMODULE module)
    {
        // This private entry point has no stable ABI; require its known x86 prologue.
        auto select = GetProcAddress(module, MAKEINTRESOURCEA(16));
        constexpr unsigned char prologue[] = { 0x8B, 0xFF, 0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x08, 0xA3 };
        if (select && memcmp(select, prologue, sizeof(prologue)) == 0)
            reinterpret_cast<void(WINAPI*)(UINT)>(select)(1);
    }

    inline HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination,
        HWND window, const RGNDATA* dirty)
    {
        const int limit = FrameLimit ? FrameLimit() : 0;
        static LARGE_INTEGER frequency = []()
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return value;
        }();
        static thread_local LONGLONG next = 0;
        static thread_local int previous = 0;
        if (limit > 0 && frequency.QuadPart > 0)
        {
            LARGE_INTEGER now{};
            QueryPerformanceCounter(&now);
            const auto period = frequency.QuadPart / limit;
            if (previous != limit || next == 0 || now.QuadPart > next + period)
                next = now.QuadPart + period;
            while (now.QuadPart < next)
            {
                if (next - now.QuadPart > frequency.QuadPart / 500)
                    Sleep(1);
                else
                    YieldProcessor();
                QueryPerformanceCounter(&now);
            }
            next += period;
        }
        else
            next = 0;
        previous = limit;
        return PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
    }

    inline HRESULT WINAPI CreateTexture(IDirect3DDevice9* device, UINT width, UINT height, UINT levels,
        DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** texture, HANDLE* shared)
    {
        if (texture && levels == 1 && (pool == D3DPOOL_DEFAULT || pool == D3DPOOL_MANAGED) &&
            !(usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_AUTOGENMIPMAP)))
        {
            const auto result = CreateTextureHook.stdcall<HRESULT>(device, width, height, 0,
                usage | D3DUSAGE_AUTOGENMIPMAP, format, pool, texture, shared);
            if (SUCCEEDED(result))
                return result;
        }
        return CreateTextureHook.stdcall<HRESULT>(device, width, height, levels, usage, format, pool, texture, shared);
    }

    inline HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window,
        DWORD behavior, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        const auto result = CreateDeviceHook.stdcall<HRESULT>(d3d, adapter, type, window, behavior, parameters, device);
        if (SUCCEEDED(result) && device && *device)
        {
            auto table = *reinterpret_cast<void***>(*device);
            if (FrameLimit && !PresentHook)
                PresentHook = safetyhook::create_inline(table[17], Present);
            if (GenerateMipmaps && !CreateTextureHook)
                CreateTextureHook = safetyhook::create_inline(table[23], CreateTexture);
        }
        return result;
    }

    inline IDirect3D9* WINAPI Create9(UINT version)
    {
        auto result = Create9Hook.stdcall<IDirect3D9*>(version);
        if (result && !CreateDeviceHook)
        {
            auto table = *reinterpret_cast<void***>(result);
            CreateDeviceHook = safetyhook::create_inline(table[16], CreateDevice);
        }
        return result;
    }

    inline void Initialize(bool hybrid, bool mipmaps = false, int (*limit)() = nullptr)
    {
        HybridAdapter = hybrid;
        GenerateMipmaps = mipmaps;
        FrameLimit = limit;
        CallbackHandler::RegisterCallback(L"d3d9.dll", []()
        {
            auto module = GetModuleHandleW(L"d3d9.dll");
            if (HybridAdapter)
                SelectHybridAdapter(module);
            if (GenerateMipmaps || FrameLimit)
            {
                if (auto create = GetProcAddress(module, "Direct3DCreate9"))
                    Create9Hook = safetyhook::create_inline(create, Create9);
            }
        });
    }
}
