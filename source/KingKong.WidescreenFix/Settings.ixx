module;

#include <stdafx.h>

export module Settings;

import ComVars;

export void InitSettings()
{
    auto pattern = hook::pattern("75 66 8D 4C 24 04 51");
    injector::MakeNOP(pattern.get_first(0), 2, true); //0x40BD3F

    pattern = hook::pattern("75 39 83 7C 24 08 01 75 32 8B"); //0x40BD6C
    struct RegHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esp + 0x118);
            regs.edx = (regs.esp + 0x0C);

            GetModuleFileNameA(NULL, (char*)regs.edx, MAX_PATH);
            *(strrchr((char*)regs.edx, '\\') + 1) = '\0';
        }
    }; injector::MakeInline<RegHook>(pattern.get_first(0), pattern.get_first(20));

    // Button names
    pattern = hook::pattern("C7 44 24 ? ? ? ? ? E8 ? ? ? ? 8B 10 8B C8 FF 52 ? 83 C0 10 89 44 24 ? C7 84 24 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 10 8B C8 FF 52 ? 83 C0 10 89 44 24 ? 68 45 01 00 00 C6 84 24 ? ? ? ? ? E8 ? ? ? ? 85 C0 74 ? 68 45 01 00 00 50 8D 4C 24 ? E8 ? ? ? ? 8B 44 24 ? 46 56 50 8D 4C 24 ? 68 ? ? ? ? 51 89 74 24 ? E8 ? ? ? ? 8B 54 24");
    static auto unk_44C810 = *pattern.get_first<void*>(4);

    static auto ButtonsHook = [](SafetyHookContext& regs)
    {
        static const char* labelStrs[] = {
            "(A)",
            "(B)",
            "(X)",
            "(Y)",
            "(LB)",
            "(RB)",
            "(LT)",
            "(RT)",
            "(Back)",
            "(Start)",
            "(LS)",
            "(RS)",
            "(D-pad Up)",
            "(D-pad Down)",
            "(D-pad Left)",
            "(D-pad Right)",
            //"(Left Stick X)",
            //"(Left Stick Y)",
            //"(Right Stick X)",
            //"(Right Stick Y)",
        };

        auto sv = std::string_view((char*)regs.eax);

        for (auto [index, label] : std::views::enumerate(labelStrs))
        {
            if (sv == ("Joy button ") + std::to_string(index + 1))
            {
                strcpy((char*)regs.eax, label);
                break;
            }
        }
    };

    pattern = hook::pattern("8B 54 24 ? 8B 44 24 ? 8B 32");
    static auto ButtonNamesHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        ButtonsHook(regs);
    });

    pattern = hook::pattern("8B 7C 24 ? 8B 44 24 ? 8B 37");
    static auto ButtonNamesHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        ButtonsHook(regs);
    });
}
