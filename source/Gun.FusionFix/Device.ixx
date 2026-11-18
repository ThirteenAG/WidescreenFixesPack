module;

#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "Shared/TonyHawk/PadIconArt.hpp"
#include <atomic>

export module Device;

import Display;
import Cutscene;
import Movie;

#pragma comment(lib, "version.lib")

export namespace GunDevice
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline SafetyHookInline CreateDeviceHook, PresentHook, ResetHook;
    inline SafetyHookInline InputCreateHook, InputDeviceHook;
    inline std::atomic<bool> UsingPad{ false };
    inline IDirect3D9*(WINAPI* NativeCreate9)(UINT) = nullptr;
    inline std::function<void()> Frame;
    inline LONG HotkeyDown = 0, HotkeyRequested = 0;

    inline HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    inline std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    inline bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
                InterlockedExchange(&HotkeyDown, 0);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    inline void Keyboard(HWND window)
    {
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down)
        {
            if (!InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
        }
        else
            InterlockedExchange(&HotkeyDown, 0);
        if (InterlockedExchange(&HotkeyRequested, 0) && window && GetForegroundWindow() == window)
            Overlay.SetOpen(!Overlay.IsOpen());
    }

    inline HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                IDirect3DSurface9* surface = nullptr;
                if (CurrentDevice && SUCCEEDED(CurrentDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    surface->GetDesc(&desc);
                    surface->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            });
            return true;
        }();
        CurrentDevice = device;
        Keyboard(GunDisplay::window);
        GunCutscene::Draw(device);
        Overlay.Present(device, ThisModule());
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && !BlocksInput());
        if (Frame)
            Frame();
        return PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        Overlay.BeforeReset();
        return ResetHook.stdcall<HRESULT>(device, parameters);
    }

    inline HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        const auto result = CreateDeviceHook.stdcall<HRESULT>(d3d, adapter, type, window, behavior, parameters, device);
        if (SUCCEEDED(result) && device && *device && !PresentHook)
        {
            auto table = *reinterpret_cast<void***>(*device);
            ResetHook = safetyhook::create_inline(table[16], Reset);
            PresentHook = safetyhook::create_inline(table[17], Present);
            GunMovie::InstallDraw(table[83]);
        }
        return result;
    }

    inline IDirect3D9* WINAPI Create9(UINT version)
    {
        auto result = NativeCreate9(version);
        if (result && !CreateDeviceHook)
            CreateDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(result))[16], CreateDevice);
        return result;
    }

    // The game reads keyboard, mouse and pad through DirectInput; nothing reaches it while the overlay has input.
    // The states also tell which device was used last, for the controller icons.
    inline void Track(DWORD size, const uint8_t* data)
    {
        static uint8_t keys[256]{}, buttons[8]{}, pad[272]{};
        if (!data || GetForegroundWindow() != GunDisplay::window)
            return;
        if (size == 256)
        {
            for (int i = 0; i < 256; ++i)
            {
                if ((data[i] & 0x80) && !(keys[i] & 0x80))
                    UsingPad.store(false, std::memory_order_relaxed);
            }
            std::memcpy(keys, data, 256);
        }
        else if (size == 16 || size == 20)
        {
            const auto* axes = reinterpret_cast<const LONG*>(data);
            bool used = axes[0] || axes[1];
            for (DWORD i = 12; i < size; ++i)
                used |= (data[i] & 0x80) && !(buttons[i - 12] & 0x80);
            std::memcpy(buttons, data + 12, size - 12);
            if (used)
                UsingPad.store(false, std::memory_order_relaxed);
        }
        else if (size == 80 || size == 272)
        {
            // Axes run from -1000 to 1000
            const auto* axes = reinterpret_cast<const LONG*>(data);
            const auto* old = reinterpret_cast<const LONG*>(pad);
            bool used = false;
            for (int i = 0; i < 6 && !used; ++i)
                used = std::abs(axes[i]) > 500 && std::abs(axes[i] - old[i]) > 200;
            const auto* pov = reinterpret_cast<const DWORD*>(data + 32);
            const auto* oldPov = reinterpret_cast<const DWORD*>(pad + 32);
            for (int i = 0; i < 4 && !used; ++i)
                used = LOWORD(pov[i]) != 0xFFFF && pov[i] != oldPov[i];
            for (int i = 48; i < 80 && !used; ++i)
                used = (data[i] & 0x80) && !(pad[i] & 0x80);
            std::memcpy(pad, data, size);
            if (used)
                UsingPad.store(true, std::memory_order_relaxed);
        }
    }

    // Keyboard, mouse and the Xidi pad can be different objects with their own functions
    inline SafetyHookInline StateHooks[4], DataHooks[4];
    inline void* StateTargets[4]{}, *DataTargets[4]{};

    template <int N>
    HRESULT WINAPI DeviceState(void* device, DWORD size, void* data)
    {
        const auto result = StateHooks[N].stdcall<HRESULT>(device, size, data);
        if (SUCCEEDED(result) && data)
        {
            if (BlocksInput())
                std::memset(data, 0, size);
            else
                Track(size, static_cast<const uint8_t*>(data));
        }
        return result;
    }

    template <int N>
    HRESULT WINAPI DeviceData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        const auto result = DataHooks[N].stdcall<HRESULT>(device, size, data, count, flags);
        if (SUCCEEDED(result) && count && BlocksInput())
            *count = 0;
        return result;
    }

    template <int N>
    void HookDevice(void* state, void* data)
    {
        if constexpr (N < 4)
        {
            if (StateTargets[N] == state)
                return;
            if (StateTargets[N])
                return HookDevice<N + 1>(state, data);
            StateTargets[N] = state;
            DataTargets[N] = data;
            StateHooks[N] = safetyhook::create_inline(state, DeviceState<N>);
            DataHooks[N] = safetyhook::create_inline(data, DeviceData<N>);
        }
    }

    inline HRESULT WINAPI InputDevice(void* input, const GUID* guid, void** device, void* outer)
    {
        const auto result = InputDeviceHook.stdcall<HRESULT>(input, guid, device, outer);
        if (SUCCEEDED(result) && device && *device)
        {
            auto table = *reinterpret_cast<void***>(*device);
            HookDevice<0>(table[9], table[10]);
        }
        return result;
    }

    inline HRESULT WINAPI InputCreate(HINSTANCE instance, DWORD version, const IID* iid, void** output, void* outer)
    {
        const auto result = InputCreateHook.stdcall<HRESULT>(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !InputDeviceHook)
            InputDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(*output))[3], InputDevice);
        return result;
    }

    // Controller icons. The game writes the name of the bound key into a prompt when its text is set; while the pad is
    // in use the pad binding is written instead as a button-font code that carries the icon, and the glyph drawn for it
    // gets the icon picture.
    namespace Prompts
    {
        constexpr uint8_t Marker = 0x80;
        inline char* (__thiscall* NativeName)(void*, int, int, unsigned, int) = nullptr;
        inline int (__cdecl* Number)(const char*) = nullptr;
        inline uint8_t** ButtonFont = nullptr;
        inline thread_local int Pending = -1, Active = -1;
        inline IDirect3DDevice9* AtlasDevice = nullptr;
        inline IDirect3DTexture9* Atlas[3][TonyHawkPadIcons::Count]{};
        inline char Codes[TonyHawkPadIcons::Count][4]{};
        inline SafetyHookMid ButtonHook, MetaHook, TextureHook, BuildHook;
        inline const uint8_t* XboxLayout = nullptr;
        inline const uint8_t* OtherLayout = nullptr;
        inline uint8_t* Controls = nullptr;

        // Binding codes of the pad (Xbox 360 layout): 1-10 axis directions, 17-20 D-pad, 21-30 buttons
        inline int Icon(uint32_t code)
        {
            constexpr int axes[] = { 19, 18, 17, 16, 6, 7, 23, 22, 21, 20 };
            constexpr int pov[] = { 15, 14, 13, 12 };
            constexpr int buttons[] = { 0, 1, 2, 3, 4, 5, 8, 9, 10, 11 };
            if (code >= 1 && code <= 10)
                return axes[code - 1];
            if (code >= 17 && code <= 20)
                return pov[code - 17];
            if (code >= 21 && code <= 30)
                return buttons[code - 21];
            return -1;
        }

        inline char* __fastcall Name(void* input, void*, int device, int action, unsigned slot, int config)
        {
            if (slot == 0 && config && UsingPad.load(std::memory_order_relaxed))
            {
                const auto* binding = reinterpret_cast<const uint32_t*>(config) + 18 * action;
                for (unsigned i = 0; i < 6; ++i)
                {
                    const uint32_t type = binding[3 * i + 1], code = binding[3 * i + 2];
                    const int icon = type >= 3 && type <= 12 ? Icon(code) : -1;
                    if (icon >= 0)
                        return Codes[icon];
                }
            }
            return NativeName(input, device, action, slot, config);
        }

        inline int Resolve(const char* text, bool draw)
        {
            const auto value = static_cast<uint8_t>(*text);
            const int icon = value >= Marker && value < Marker + TonyHawkPadIcons::Count ? value - Marker : -1;
            // The icon takes the place of an arrow glyph, which every button font has
            const int glyph = icon >= 0 ? 4 : Number(text);
            if (draw)
            {
                // The arrow glyphs (down, right, left, up) are the D-pad on the console
                constexpr int dpad[] = { 13, 15, 14, 12 };
                Pending = icon >= 0 ? icon : glyph >= 4 && glyph <= 7 && UsingPad.load(std::memory_order_relaxed) ? dpad[glyph - 4] : -1;
            }
            return glyph;
        }

        inline int __cdecl Width(const char* text)
        {
            return Resolve(text, false);
        }

        inline int __cdecl Draw(const char* text)
        {
            return Resolve(text, true);
        }

        inline IDirect3DTexture9* GetAtlas(uint8_t* font, int icon)
        {
            auto* wrapper = *reinterpret_cast<uint8_t**>(font + 0x178);
            auto* native = wrapper ? *reinterpret_cast<IDirect3DTexture9**>(wrapper + 0x18) : nullptr;
            if (!native)
                return nullptr;
            IDirect3DDevice9* device = nullptr;
            if (FAILED(native->GetDevice(&device)))
                return nullptr;
            if (device != AtlasDevice)
            {
                for (auto& family : Atlas)
                {
                    for (auto*& texture : family)
                    {
                        if (texture) { texture->Release(); texture = nullptr; }
                    }
                }
                if (AtlasDevice) AtlasDevice->Release();
                AtlasDevice = device;
            }
            else
                device->Release();
            static const auto& setting = FixSettings().Int("MAIN.GamepadIcons");
            const int family = std::clamp(setting, 0, 3);
            const int index = family == 0 ? 0 : family - 1;
            auto*& result = Atlas[index][icon];
            if (result)
                return result;
            D3DSURFACE_DESC desc{};
            if (FAILED(native->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height)
                return nullptr;
            IDirect3DTexture9* texture = nullptr;
            if (FAILED(AtlasDevice->CreateTexture(desc.Width, desc.Height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr)))
                return nullptr;
            D3DLOCKED_RECT lock{};
            if (FAILED(texture->LockRect(0, &lock, nullptr, 0)))
            {
                texture->Release();
                return nullptr;
            }
            for (UINT y = 0; y < desc.Height; ++y)
                std::memset(static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch, 0, desc.Width * 4);
            // Every glyph rectangle of the button font gets the icon: the batch draws only the one glyph
            const auto* glyphs = *reinterpret_cast<const uint8_t* const*>(font + 8);
            const auto* map = font + 0x10C;
            for (int i = 0; i < 33; ++i)
            {
                const auto* uv = reinterpret_cast<const float*>(glyphs + 24 * map[i]);
                TonyHawkPadIcons::Rect r{ static_cast<int>(uv[0] * desc.Width), static_cast<int>(uv[1] * desc.Height),
                    static_cast<int>((uv[2] - uv[0]) * desc.Width + 0.5f), static_cast<int>((uv[3] - uv[1]) * desc.Height + 0.5f) };
                if (r.x >= 0 && r.y >= 0 && r.x + r.w <= static_cast<int>(desc.Width) && r.y + r.h <= static_cast<int>(desc.Height))
                    TonyHawkPadIcons::Paint(static_cast<uint32_t*>(lock.pBits), lock.Pitch / 4, r, icon, index + 1);
            }
            texture->UnlockRect(0);
            return result = texture;
        }

        inline void Install()
        {
            for (int i = 0; i < TonyHawkPadIcons::Count; ++i)
            {
                Codes[i][0] = '\\';
                Codes[i][1] = 'b';
                Codes[i][2] = static_cast<char>(Marker + i);
            }
            auto name = hook::pattern("50 57 53 55 E8 ? ? ? ? 8B 0D ? ? ? ? 8B E8");
            auto width = hook::pattern("47 57 E8 ? ? ? ? 8B 35 ? ? ? ? 0F B6 84 30 0C 01 00 00");
            auto draw = hook::pattern("43 53 E8 ? ? ? ? 8B 0D ? ? ? ? 0F B6 84 08 0C 01 00 00");
            auto button = hook::pattern("03 F0 89 0D ? ? ? ? E9");
            auto meta = hook::pattern("03 F0 8B 44 24 44 A3 ? ? ? ? C6 44 24 11 01");
            auto texture = hook::pattern("8B 86 78 01 00 00 8B 50 1C 8B 40 18 52 50 6A 00 E8");
            NativeName = reinterpret_cast<decltype(NativeName)>(injector::GetBranchDestination(name.get_first(4)).as_int());
            Number = reinterpret_cast<decltype(Number)>(injector::GetBranchDestination(draw.get_first(2)).as_int());
            ButtonFont = *draw.get_first<uint8_t**>(9);
            injector::MakeCALL(name.get_first(4), Name, true);
            injector::MakeCALL(width.get_first(2), Width, true);
            injector::MakeCALL(draw.get_first(2), Draw, true);
            // The button font is selected after the previous batch was drawn
            ButtonHook = safetyhook::create_mid(button.get_first(2), [](SafetyHookContext&) { Active = Pending; });
            MetaHook = safetyhook::create_mid(meta.get_first(6), [](SafetyHookContext&) { Active = -1; });
            TextureHook = safetyhook::create_mid(texture.get_first(16), [](SafetyHookContext& regs)
            {
                if (Active < 0 || regs.esi != reinterpret_cast<uintptr_t>(*ButtonFont))
                    return;
                if (auto* atlas = GetAtlas(*ButtonFont, Active))
                    reinterpret_cast<uintptr_t*>(regs.esp)[1] = reinterpret_cast<uintptr_t>(atlas);
            });
            // The game has its own default controls for an Xbox 360 controller, chosen by the controller's name; the
            // bundled Xidi and Windows' own driver for Xbox controllers have that layout. Controller bindings that are
            // still the other default get that layout when the game builds its controls.
            auto layout = hook::pattern("8D B5 ? ? ? ? 74 06 8D B5 ? ? ? ? 83 C5 50 81 FD B0 09 00 00").count(2).get(0);
            auto build = hook::pattern("8B 0D ? ? ? ? 56 68 ? ? ? ? 6A 00 E8 ? ? ? ? 8B 0D ? ? ? ? 6A 00 E8");
            auto table = hook::pattern("BE ? ? ? ? 0F BF 46 FC 0F B7 4E FE");
            XboxLayout = *layout.get<const uint8_t*>(2);
            OtherLayout = *layout.get<const uint8_t*>(10);
            Controls = *table.get_first<uint8_t*>(1) - 56;
            BuildHook = safetyhook::create_mid(build.get_first(), [](SafetyHookContext&)
            {
                // Entries of 80 bytes; the controller binding of an action is at +56
                for (int i = 0; i < 31; ++i)
                {
                    auto& binding = *reinterpret_cast<uint32_t*>(Controls + 80 * i + 56);
                    const auto other = *reinterpret_cast<const uint32_t*>(OtherLayout + 80 * i + 56);
                    if (binding == other)
                        binding = *reinterpret_cast<const uint32_t*>(XboxLayout + 80 * i + 56);
                }
            });
        }
    }

    inline void Init()
    {
        GunMovie::Init();
        Prompts::Install();

        auto create = hook::pattern("6A 20 E8 ? ? ? ? 3B C7 A3 ? ? ? ? 75").get_first<uint8_t>(2);
        NativeCreate9 = reinterpret_cast<decltype(NativeCreate9)>(injector::GetBranchDestination(create, true).as_int());
        injector::MakeCALL(create, &Create9, true);
        GunDisplay::messageHook = Message;

        // The game loads DirectInput from the system folder at run time; the game folder comes first, for the bundled
        // Xidi
        static const char dinput[] = "dinput8.dll";
        auto library = hook::pattern("68 ? ? ? ? 52 88 9C 24 1F 01 00 00 E8");
        injector::WriteMemory(library.get_first(1), dinput, true);

        // The game loads DirectInput at run time.
        CallbackHandler::RegisterCallback(L"dinput8.dll", []()
        {
            if (auto create = GetProcAddress(GetModuleHandleW(L"dinput8.dll"), "DirectInput8Create"))
                InputCreateHook = safetyhook::create_inline(create, InputCreate);
        });
    }
}

