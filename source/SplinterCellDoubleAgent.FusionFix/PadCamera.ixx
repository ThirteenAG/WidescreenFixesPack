module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <cwchar>
#include <format>
#include <string>
#include <string_view>

export module PadCamera;

namespace DAPadCamera
{
    inline bool Equal(std::wstring_view text, std::wstring_view word)
    {
        return text.size() == word.size() && std::equal(text.begin(), text.end(), word.begin(),
            [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); });
    }
    inline std::wstring Command(std::wstring_view command, float factor)
    {
        if (!std::isfinite(factor) || factor == 1.0f)
            return {};
        factor = std::clamp(factor, 0.25f, 3.0f);
        auto start = command.find_first_not_of(L" \t");
        auto end = start == command.npos ? command.npos : command.find_first_of(L" \t", start);
        if (end == command.npos || !Equal(command.substr(start, end - start), L"Axis"))
            return {};
        start = command.find_first_not_of(L" \t", end);
        if (start == command.npos)
            return {};
        end = command.find_first_of(L" \t", start);
        if (end == command.npos) end = command.size();
        const auto axis = command.substr(start, end - start);
        if (!Equal(axis, L"aTurn") && !Equal(axis, L"aLookUp"))
            return {};
        std::wstring result(command);
        const auto insertion = end;
        float speed = 1.0f;
        for (start = end; start < command.size(); start = end)
        {
            start = command.find_first_not_of(L" \t", start);
            if (start == command.npos) break;
            end = command.find_first_of(L" \t", start);
            if (end == command.npos) end = command.size();
            const auto token = command.substr(start, end - start);
            if (token.size() < 6 || !Equal(token.substr(0, 6), L"Speed=")) continue;
            std::wstring number(token.substr(6));
            wchar_t* tail = nullptr;
            speed = std::wcstof(number.c_str(), &tail);
            if (tail == number.c_str() || *tail || !std::isfinite(speed))
                return {};
            result.erase(start, end - start);
            break;
        }
        if (!std::isfinite(speed * factor))
            return {};
        result.insert(insertion, std::format(L" Speed={:.6f}", speed * factor));
        return result;
    }
}

namespace
{
    SafetyHookInline PadDirectAxisHook, PadAxisExecHook;
    thread_local bool PadLookAxis = false;
    std::atomic<float> PadCameraSpeed = 1.0f;

    void __fastcall PadDirectAxis(void* input, void* edx, int key, float value, float delta)
    {
        const auto previous = PadLookAxis;
        PadLookAxis = key == 0xE2 || key == 0xE9;
        struct Restore
        {
            bool previous;
            ~Restore() { PadLookAxis = previous; }
        } restore{ previous };
        PadDirectAxisHook.unsafe_fastcall<void>(input, edx, key, value, delta);
    }
    int __fastcall PadAxisExec(void* input, void* edx, const wchar_t* command, void* output)
    {
        const auto replacement = PadLookAxis && command ?
            DAPadCamera::Command(command, PadCameraSpeed.load(std::memory_order_relaxed)) : std::wstring{};
        return PadAxisExecHook.unsafe_fastcall<int>(input, edx, replacement.empty() ? command : replacement.c_str(), output);
    }
}

export void InitPadCamera()
{
    const auto module = GetModuleHandleW(L"Engine.dll");
    PadAxisExecHook = safetyhook::create_inline(GetProcAddress(module, "?Exec@UInput@@UAEHPBGAAVFOutputDevice@@@Z"), PadAxisExec);
    PadDirectAxisHook = safetyhook::create_inline(GetProcAddress(module, "?DirectAxis@UInput@@UAEXW4EInputKey@@MM@Z"), PadDirectAxis);
    const auto refresh = []()
    {
        PadCameraSpeed.store(std::clamp(FixSettings().Float("MAIN.GamepadCameraSpeed"), 0.25f, 3.0f), std::memory_order_relaxed);
    };
    refresh();
    FixSettings().OnChange("MAIN.GamepadCameraSpeed", refresh);
}
