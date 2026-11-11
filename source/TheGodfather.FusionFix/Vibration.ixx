module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <dinput.h>
#include <mutex>
#include <unordered_map>
#include <stdexcept>

export module Vibration;

import Window;

import ComVars;
import Input;

export namespace GodfatherVibration
{
    using Sender = bool(__cdecl*)(short, unsigned short, unsigned short);
    SafetyHookInline deviceHook, outputHook, destroyHook;
    std::atomic<bool> enabled{ true };
    std::mutex guard;
    std::unordered_map<const void*, GUID> devices;
    Sender sender{};
    short target = -1;

    short Controller(const GUID& guid)
    {
        constexpr BYTE signature[]{ 'X', 'I', 'D', 'I', 'V', 'C', 0, 0 };
        const auto index = guid.Data1 >> 16;
        if ((guid.Data1 & 0xFFFF) != 0xFFFF || guid.Data2 || guid.Data3 || index < 1 || index > 4
            || memcmp(guid.Data4, signature, sizeof(signature)))
            return -1;
        return static_cast<short>(index - 1);
    }

    short Selected()
    {
        auto input = GodfatherInputSlot.is_initialized() ? GodfatherInputSlot.get() : nullptr;
        if (!input)
            return -1;
        const auto bytes = static_cast<uint8_t*>(input);
        const auto count = *reinterpret_cast<const unsigned*>(bytes + 2140);
        const auto index = *reinterpret_cast<const int*>(bytes + 2144);
        if (!bytes[716] || count > 128 || index < 0 || unsigned(index) >= count)
            return -1;
        const auto device = *reinterpret_cast<void* const*>(bytes + 1628 + 4 * index);
        const auto found = devices.find(device);
        return found != devices.end() ? Controller(found->second) : -1;
    }

    bool Allowed()
    {
        return enabled.load(std::memory_order_relaxed) && GodfatherWindow::window
            && GetForegroundWindow() == GodfatherWindow::window && !GodfatherInput::Blocked()
            && GodfatherWantedMode.is_initialized() && GodfatherPauseReasons.is_initialized()
            && GodfatherWantedMode.get() == 2
            && !GodfatherPauseReasons.get();
    }

    void StopLocked()
    {
        if (target >= 0 && sender)
            sender(target, 0, 0);
        target = -1;
    }

    void Stop()
    {
        std::scoped_lock lock(guard);
        StopLocked();
    }

    void Service()
    {
        std::scoped_lock lock(guard);
        if (!Allowed() || target != Selected())
            StopLocked();
    }

    void __stdcall Output(unsigned slot, unsigned low, unsigned high)
    {
        // Other logical channels are updated after the primary player each tick.
        if (slot != 0)
            return;
        std::scoped_lock lock(guard);
        const auto selected = Allowed() ? Selected() : short(-1);
        if (target != selected)
            StopLocked();
        if (selected < 0)
            return;
        if (!sender)
        {
            if (const auto module = GetModuleHandleW(L"Xidi.32.dll"))
                sender = reinterpret_cast<Sender>(GetProcAddress(module, "XidiSendVibration"));
        }
        if (sender)
        {
            target = selected;
            sender(target, static_cast<unsigned short>((low & 0xFF) * 257),
                static_cast<unsigned short>((high & 0xFF) * 257));
        }
    }

    int __fastcall Device(void* self, void*, const DIDEVICEINSTANCEA* instance, const DIDATAFORMAT* format)
    {
        const auto result = deviceHook.unsafe_thiscall<int>(self, instance, format);
        std::scoped_lock lock(guard);
        devices.erase(self);
        if (result && instance)
            devices.emplace(self, instance->guidInstance);
        return result;
    }

    int __fastcall Destroy(void* self, void*)
    {
        Stop();
        return destroyHook.unsafe_thiscall<int>(self);
    }

    void Init()
    {
        auto device = hook::pattern("81 EC 48 02 00 00 A1 ? ? ? ? 8B 94 24 4C 02 00 00 56 57 8B F1 6A 00 "
            "89 84 24 50 02 00 00 8B 86 64 04 00 00 8B 80 B4 00 00 00");
        auto output = hook::pattern("8B 54 24 15 84 C0 8B 44 24 16 50 52 56 74 0F E8 ? ? ? ? 5F 5E 5D 5B 83 C4 0C C2 04 00");
        auto destroy = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 08 56 8B F1 "
            "57 89 74 24 0C C7 06 ? ? ? ? C7 46 08 ? ? ? ? 68 ? ? ? ? C7 44 24 1C 01 00 00 00 "
            "E8 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B 7E 0C 85 FF 74 11");
        auto thunk = injector::GetBranchDestination(output.get_first<uint8_t>(15)).get<uint8_t>();
        deviceHook = safetyhook::create_inline(device.get_first(), Device, safetyhook::InlineHook::StartDisabled);
        outputHook = safetyhook::create_inline(thunk, Output, safetyhook::InlineHook::StartDisabled);
        destroyHook = safetyhook::create_inline(destroy.get_first(), Destroy, safetyhook::InlineHook::StartDisabled);
        (void)deviceHook.enable();
        (void)outputHook.enable();
        (void)destroyHook.enable();
        const auto refresh = []()
        {
            enabled.store(FixSettings().Bool("MAIN.Vibration"), std::memory_order_relaxed);
            Service();
        };
        refresh();
        FixSettings().OnChange("MAIN.Vibration", refresh);
        const auto frame = GodfatherWindow::frame;
        GodfatherWindow::frame = [frame](IDirect3DDevice9* device)
        {
            if (frame)
                frame(device);
            Service();
        };
        const auto message = GodfatherWindow::message;
        GodfatherWindow::message = [message](HWND target, UINT id, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            if (target == GodfatherWindow::window && ((id == WM_ACTIVATEAPP && !wParam) || id == WM_NCDESTROY))
                Stop();
            return message && message(target, id, wParam, lParam, result);
        };
    }
}
