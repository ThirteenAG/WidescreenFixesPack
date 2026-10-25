#pragma once

namespace DisplayModes
{
    std::vector<int32_t> Modes;
    std::vector<int32_t**> References;

    SafetyHookMid ValidationHook, ModeCountHook, ScreenListHook;

    void Init(HMODULE module = GetModuleHandle(nullptr))
    {
        auto validation = hook::module_pattern(module, "A1 ? ? ? ? 69 C0 E8 03 00 00 99 F7 3D ? ? ? ? 05 1E FB FF FF 3D 26 02 00 00 0F 96 C0 C3");
        if (validation.size() == 1)
        {
            static auto width = *validation.get_first<int*>(1);
            static auto height = *validation.get_first<int*>(14);
            static auto resume = reinterpret_cast<uintptr_t>(validation.get_first(31));
            ValidationHook = safetyhook::create_mid(validation.get_first(), [](SafetyHookContext& regs)
            {
                regs.eax = *width >= 640 && *height >= 480 && *width <= 16384 && *height <= 16384;
                regs.eip = resume;
            });
            if (!ValidationHook)
                OutputDebugStringA("Bully: saved resolution validation hook unavailable.\n");
        }
        else
            OutputDebugStringA("Bully: saved resolution validation pattern unavailable.\n");
        auto count = hook::module_pattern(module, "8B 50 18 51 56 FF D2 8B D8 3B DD 75 ? 8B 0D");
        auto table = hook::module_pattern(module, "B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 81 EC 8C 05 00 00 53 55 33 ED 56 57");
        if (count.size() != 1 || table.size() != 1)
        {
            OutputDebugStringA("Bully: display mode table patterns unavailable.\n");
            return;
        }
        auto original = *table.get_first<int32_t*>(1);
        constexpr std::pair<const char*, int> sites[] =
        {
            { "B8", 1 }, { "89 04 D5", 3 }, { "39 3C C5", 3 }, { "89 3C CD", 3 }, { "39 34 C5", 3 },
            { "89 0C C5", 3 }, { "39 14 C5", 3 }, { "89 14 C5", 3 }, { "8B 3C C5", 3 }
        };
        for (size_t i = 0; i < std::size(sites); ++i)
        {
            auto signature = std::string(sites[i].first) + " " + pattern_str(to_bytes(original + (i >= 5 ? 1 : 0)));
            auto reference = hook::module_pattern(module, signature);
            if (reference.size() != 1)
            {
                References.clear();
                OutputDebugStringA("Bully: display mode table reference unavailable.\n");
                return;
            }
            References.push_back(reference.get_first<int32_t*>(sites[i].second));
        }
        // The Video screen copies the list into its own 128 entries and reads the count right after the native
        // table (+0x400). Give it a copy in the native layout: the 640x480 entry and the largest 127 modes.
        auto screen = hook::module_pattern(module, "E8 ? ? ? ? 8B 88 00 04 00 00 89 8E 18 04 00 00 33 C9 39 88 00 04 00 00");
        if (screen.size() == 1)
        {
            static auto nativeCount = reinterpret_cast<int32_t*>(original + 256);
            ScreenListHook = safetyhook::create_mid(screen.get_first(5), [](SafetyHookContext& regs)
            {
                static int32_t list[257] = {};
                int32_t count = std::clamp(*nativeCount, 0, int32_t(Modes.size() / 2));
                int32_t shown = std::min(count, 128);
                for (int32_t i = 0; i < shown; ++i)
                {
                    int32_t from = i == 0 ? 0 : count - shown + i;
                    list[i * 2] = Modes[from * 2];
                    list[i * 2 + 1] = Modes[from * 2 + 1];
                }
                list[256] = shown;
                regs.eax = reinterpret_cast<uintptr_t>(list);
            });
        }
        else
            OutputDebugStringA("Bully: video screen mode list pattern unavailable.\n");
        ModeCountHook = safetyhook::create_mid(count.get_first(7), [](SafetyHookContext& regs)
        {
            // The native list includes one 640x480 entry in addition to the adapter's modes.
            Modes.resize((static_cast<size_t>(regs.eax) + 1) * 2);
            for (size_t i = 0; i < References.size(); ++i)
                ProtectedGameRef<int32_t*>{ References[i] } = Modes.data() + (i >= 5 ? 1 : 0);
        });
    }
}
