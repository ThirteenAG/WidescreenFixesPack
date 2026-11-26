module;
#include <stdafx.h>
export module Resolution;
import ComVars;
import Hud;
import Video;

export void UpdateScreen(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    Screen.nWidth = width;
    Screen.nHeight = height;
    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    JCHud::screenAspect.store(Screen.fAspectRatio);
    JCVideo::outputSize.store((static_cast<uint32_t>(width) << 16) | static_cast<uint32_t>(height));
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fWidescreenOffsetX = (Screen.fWidth - Screen.fHeight * (16.0f / 9.0f)) / 2.0f;
    Screen.fWidescreenOffsetY = (Screen.fHeight - (Screen.fHeight * ((4.0f / 3.0f) / (16.0f / 9.0f)))) / 2.0f;
}

// The game's screen resolution setting is an index into five fixed 4:3 modes (menu, settings.txt and device). The
// index now picks one of the monitor's modes: the menu lists them, settings.txt keeps the chosen size as text.
export namespace JCResolution
{
    inline constexpr int ResolutionSetting = 14;
    // The options row has room for six sizes
    inline constexpr int MaximumModes = 6;
    inline std::vector<std::pair<int, int>> Modes;
    inline std::string Labels;
    inline int DesktopIndex = 0;
    inline int ParsedIndex = 0;
    inline bool Parsed = false;
    inline uint8_t** Settings = nullptr;
    inline SafetyHookInline ValidHook, LoadHook;
    using Assign = void(__thiscall*)(void*, const char*, size_t);
    inline Assign AssignText = nullptr;
    inline Assign AppendText = nullptr;

    inline std::pair<int, int> Desktop()
    {
        auto [width, height] = GetDesktopRes();
        if (width <= 0 || height <= 0)
            return { 640, 480 };
        return { width, height };
    }

    // Distinct modes of the primary monitor; with more than the menu holds, those of the desktop's shape and the
    // largest others stay
    inline void BuildModes()
    {
        std::vector<std::pair<int, int>> modes;
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &mode); ++i)
        {
            const std::pair<int, int> size{ int(mode.dmPelsWidth), int(mode.dmPelsHeight) };
            if (mode.dmBitsPerPel >= 32 && size.first >= 640 && size.second >= 480 &&
                std::find(modes.begin(), modes.end(), size) == modes.end())
                modes.push_back(size);
        }
        const auto desktop = Desktop();
        if (std::find(modes.begin(), modes.end(), desktop) == modes.end())
            modes.push_back(desktop);
        const auto shape = [&](const std::pair<int, int>& size)
        {
            return std::abs(size.first * desktop.second - size.second * desktop.first) * 100 <= desktop.first * size.second;
        };
        std::stable_sort(modes.begin(), modes.end(), [&](const auto& a, const auto& b)
        {
            if ((a == desktop) != (b == desktop))
                return a == desktop;
            if (shape(a) != shape(b))
                return shape(a);
            return a.first * a.second > b.first * b.second;
        });
        if (modes.size() > MaximumModes)
            modes.resize(MaximumModes);
        std::sort(modes.begin(), modes.end(), [](const auto& a, const auto& b)
        {
            return a.first != b.first ? a.first < b.first : a.second < b.second;
        });
        Modes = std::move(modes);
        Labels.clear();
        for (size_t i = 0; i < Modes.size(); ++i)
        {
            if (Modes[i] == desktop)
                DesktopIndex = int(i);
            Labels += (i ? "$" : "") + std::to_string(Modes[i].first) + "x" + std::to_string(Modes[i].second);
        }
    }

    inline bool Valid(float value)
    {
        return value >= 0.0f && value < float(Modes.size()) && value == std::floor(value);
    }

    inline int Index()
    {
        const int index = *reinterpret_cast<int*>(*Settings + 0x200);
        return index >= 0 && index < int(Modes.size()) ? index : DesktopIndex;
    }

    inline std::pair<int, int> Size()
    {
        return Modes[Index()];
    }

    inline std::string Label(int index)
    {
        return std::to_string(Modes[index].first) + "x" + std::to_string(Modes[index].second);
    }

    inline bool __fastcall CheckValue(uint8_t* settings, void*, int id, float value)
    {
        if (id == ResolutionSetting)
            return Valid(value);
        return ValidHook.unsafe_thiscall<bool>(settings, id, value);
    }

    inline int __fastcall LoadValue(uint8_t* settings, void*, int id, float value)
    {
        if (id != ResolutionSetting)
            return LoadHook.unsafe_thiscall<int>(settings, id, value);
        // A value from settings.txt, else the game's default (Default settings, no settings.txt): the desktop's size
        *reinterpret_cast<int*>(settings + 0x200) = Parsed && Valid(value) ? int(value) : DesktopIndex;
        Parsed = false;
        settings[0x1B9] = 1;
        return 1;
    }

    // settings.txt values: a "WxH" value is a screen resolution, any other one keeps the game's comparison
    inline int __cdecl CompareValue(const char* value, const char* first)
    {
        int width = 0, height = 0;
        char rest = 0;
        if (value && sscanf_s(value, "%dx%d%c", &width, &height, &rest, 1u) == 2)
        {
            const auto found = std::find(Modes.begin(), Modes.end(), std::pair{ width, height });
            ParsedIndex = found != Modes.end() ? int(found - Modes.begin()) : DesktopIndex;
            Parsed = true;
            return 0;
        }
        return std::strcmp(value, first);
    }

    inline void Init()
    {
        BuildModes();

        auto pattern = hook::pattern("8B 0D ? ? ? ? 6A 0E E8");
        Settings = *pattern.get_first<uint8_t**>(2);

        ValidHook = safetyhook::create_inline(hook::pattern("55 8B EC 83 EC 28 89 4D E0 8B 45 08 C1 E0 04 8B 4D E0").get_first(), CheckValue);
        LoadHook = safetyhook::create_inline(hook::pattern("55 8B EC 83 EC 10 89 4D F8 8B 45 0C 50 8B 4D 08 51 8B 4D F8 E8").get_first(), LoadValue);

        // settings.txt reading: the first resolution comparison takes every "WxH" value
        pattern = hook::pattern("68 ? ? ? ? 8B 8D 5C FD FF FF 51 E8 ? ? ? ? 83 C4 08 85 C0 75 0C C7 45 A8 00 00 00 00");
        injector::MakeCALL(pattern.get_first(12), CompareValue, true);
        struct ParsedValue
        {
            void operator()(injector::reg_pack& regs)
            {
                *reinterpret_cast<int*>(regs.ebp - 0x58) = ParsedIndex;
            }
        }; injector::MakeInline<ParsedValue>(pattern.get_first(24), pattern.get_first(31));

        // settings.txt writing: the chosen size instead of the game's five names
        pattern = hook::pattern("8D 4D CC E8 ? ? ? ? 8B 85 EC FD FF FF 8B 88 00 02 00 00");
        AppendText = reinterpret_cast<Assign>(injector::GetBranchDestination(pattern.get_first(3)).as_int());
        struct SavedValue
        {
            void operator()(injector::reg_pack& regs)
            {
                const auto index = *reinterpret_cast<int*>(regs.eax + 0x200);
                const auto text = Label(index >= 0 && index < int(Modes.size()) ? index : DesktopIndex);
                AppendText(reinterpret_cast<void*>(regs.ebp - 0x34), text.c_str(), text.size());
                regs.ecx = 0xFF;
            }
        }; injector::MakeInline<SavedValue>(pattern.get_first(14), pattern.get_first(20));

        // Options menu: the resolution list's items
        pattern = hook::pattern("68 ? ? ? ? 8D 4D C4 E8 ? ? ? ? C7 45 FC 05 00 00 00 8D 4D C4 51 8B 95 58 FC FF FF 83 C2 68 52 8B 85 58 FC FF FF 83 C0 4C 50 8B 8D 58 FC FF FF E8");
        AssignText = reinterpret_cast<Assign>(injector::GetBranchDestination(pattern.get_first(8)).as_int());
        struct MenuItems
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = *reinterpret_cast<uint32_t*>(regs.ebp - 0x3A8);
                const auto items = reinterpret_cast<uint8_t*>(regs.ecx + 0x4C);
                const auto capacity = *reinterpret_cast<uint32_t*>(items + 24);
                const auto text = capacity < 16 ? reinterpret_cast<const char*>(items + 4) : *reinterpret_cast<const char**>(items + 4);
                if (text && std::strstr(text, "640x480") && std::strstr(text, "1600x1200"))
                    AssignText(items, Labels.c_str(), Labels.size());
            }
        }; injector::MakeInline<MenuItems>(pattern.get_first(44), pattern.get_first(50));
    }
}
