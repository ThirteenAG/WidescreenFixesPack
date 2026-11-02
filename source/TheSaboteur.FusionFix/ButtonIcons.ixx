module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module ButtonIcons;


export namespace SaboteurButtons
{
    inline SafetyHookInline ImageHook;
    inline std::filesystem::path Directory;
    inline int Family = 0;

    inline void* __fastcall Load(void* self, void* edx, const char* name, void* format, void* usage)
    {
        if (Family && name)
        {
            const auto stem = std::filesystem::path(name).stem().string();
            static constexpr std::string_view names[] = {
                "icn_button_up", "icn_button_down", "icn_button_left", "icn_button_right",
                "icn_button_accept", "icn_button_cancel", "icn_LB", "icn_LT", "icn_L3",
                "icn_RB", "icn_RT", "icn_R3", "icn_start", "icn_select_back",
                "icn_joystick_L", "icn_joystick_R", "icn_dpad", "icn_dpad_up", "icn_dpad_down",
                "icn_dpad_updown", "icn_dpad_left", "icn_dpad_right", "icn_dpad_leftright"
            };
            for (const auto candidate : names)
            {
                if (!_stricmp(stem.c_str(), candidate.data()))
                {
                    const auto file = Directory / std::to_string(Family) / ("FF_Saboteur_" + std::to_string(Family) + "_" + std::string(candidate) + ".dds");
                    if (std::filesystem::is_regular_file(file))
                    {
                        if (auto image = ImageHook.unsafe_fastcall<void*>(self, edx, file.string().c_str(), reinterpret_cast<void*>(14), usage))
                            return image;
                    }
                    break;
                }
            }
        }
        return ImageHook.unsafe_fastcall<void*>(self, edx, name, format, usage);
    }

    inline void Install()
    {
        Family = FixSettings().Int("MAIN.GamepadIcons");
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Install), &module);
        wchar_t path[32768]{};
        GetModuleFileNameW(module, path, static_cast<DWORD>(std::size(path)));
        Directory = std::filesystem::path(path).replace_extension() / "buttons";
        auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 44 53 55 56 57 8B 7C 24 64 6A 5C 57 89 4C 24 18");
        ImageHook = safetyhook::create_inline(pattern.get_first(), Load);
    }
}
