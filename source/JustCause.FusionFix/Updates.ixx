module;
#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;
import Imports;


// Update notice: one line at the top of the game's menu screens
export namespace JCUpdates
{
    struct Font : IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetDescA(void*) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetDescW(void*) = 0;
        virtual BOOL STDMETHODCALLTYPE GetTextMetricsA(void*) = 0;
        virtual BOOL STDMETHODCALLTYPE GetTextMetricsW(void*) = 0;
        virtual HDC STDMETHODCALLTYPE GetDC() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetGlyphData(UINT, void**, RECT*, POINT*) = 0;
        virtual HRESULT STDMETHODCALLTYPE PreloadCharacters(UINT, UINT) = 0;
        virtual HRESULT STDMETHODCALLTYPE PreloadGlyphs(UINT, UINT) = 0;
        virtual HRESULT STDMETHODCALLTYPE PreloadTextA(LPCSTR, INT) = 0;
        virtual HRESULT STDMETHODCALLTYPE PreloadTextW(LPCWSTR, INT) = 0;
        virtual INT STDMETHODCALLTYPE DrawTextA(void*, LPCSTR, INT, RECT*, DWORD, D3DCOLOR) = 0;
        virtual INT STDMETHODCALLTYPE DrawTextW(void*, LPCWSTR, INT, RECT*, DWORD, D3DCOLOR) = 0;
        virtual HRESULT STDMETHODCALLTYPE OnLostDevice() = 0;
        virtual HRESULT STDMETHODCALLTYPE OnResetDevice() = 0;
    };
    using CreateFont_t = HRESULT(WINAPI*)(IDirect3DDevice9*, INT, UINT, UINT, UINT, BOOL, DWORD, DWORD, DWORD, DWORD, LPCWSTR, Font**);

    inline std::wstring Line;
    inline std::mutex mutex;
    inline Font* Text = nullptr;
    inline IDirect3DDevice9* TextDevice = nullptr;
    inline int TextHeight = 0;
    inline CreateFont_t Create = nullptr;

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void BeforeReset()
    {
        if (Text)
            Text->OnLostDevice();
    }

    inline void AfterReset()
    {
        if (Text)
            Text->OnResetDevice();
    }

    // From Present, while one of the game's menus is on screen
    inline void Draw(IDirect3DDevice9* device, int width, int height, bool menu)
    {
        std::wstring line;
        {
            const std::lock_guard lock(mutex);
            line = Line;
        }
        if (line.empty() || !menu || !device || !Create || width <= 0 || height <= 0)
            return;
        const auto size = std::max(12, height / 32);
        if (Text && (TextDevice != device || TextHeight != size))
        {
            Text->Release();
            Text = nullptr;
        }
        if (!Text && FAILED(Create(device, size, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Tahoma", &Text)))
        {
            Text = nullptr;
            return;
        }
        TextDevice = device;
        TextHeight = size;
        if (FAILED(device->BeginScene()))
            return;
        RECT area{ 0, height / 40, width, height / 40 + size * 2 };
        RECT shadow = area;
        OffsetRect(&shadow, 2, 2);
        Text->DrawTextW(nullptr, line.c_str(), -1, &shadow, DT_CENTER | DT_TOP | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
        Text->DrawTextW(nullptr, line.c_str(), -1, &area, DT_CENTER | DT_TOP | DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));
        device->EndScene();
    }

    inline void Tick()
    {
        RWGTA::UpdateCheck::Tick();
    }

    inline void Init()
    {
        if (const auto d3dx = GetModuleHandleW(L"d3dx9_26.dll"))
            Create = reinterpret_cast<CreateFont_t>(GetProcAddress(d3dx, "D3DXCreateFontW"));
        const bool available = Create != nullptr;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;
        JCImports::Replace("KERNEL32.dll", "ExitProcess", &Exit);
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "JustCause.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return true; };
        config.show = [](const std::wstring& line)
        {
            const std::lock_guard lock(mutex);
            Line = line;
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

