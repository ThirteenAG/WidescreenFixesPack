module;

#include <stdafx.h>
#include <LEDEffects.h>

export module LED;

std::string currentGogglesLight = "LightGreen";
int __cdecl appStricmpHook(const char* String1, const char* String2)
{
    auto ret = _stricmp(String1, String2);
    if (!ret)
        currentGogglesLight = String1;
    else
        currentGogglesLight = "LightGreen";
    return ret;
}

export void InitLED()
{
    CIniReader iniReader("");
    auto bLightSyncRGB = iniReader.ReadInteger("LOGITECH", "LightSyncRGB", 1);

    if (bLightSyncRGB)
    {
        // LoadoutCommon::FsBaseLoadout::GetSuitLightColor
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 85 C0 0F 84 ? ? ? ? 46 83 FE 0A");
        injector::MakeCALL(pattern.get_first(0), appStricmpHook, true);

        LEDEffects::Inject([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            static const std::unordered_map<std::string_view, std::tuple<int, int, int>> colors =
            {
                { "LightGreen",      { 17, 138, 41 } },
                { "LightPaleOrange", { 158, 118, 29 } },
                { "LightRed",        { 163, 18, 15 } },
                { "LightBlue",       { 24, 70, 213 } },
                { "LightPink",       { 163, 33, 116 } },
                { "LightPurple",     { 111, 34, 210 } },
                { "LightWhite",      { 162, 182, 208 } },
                { "LightGold",       { 145, 172, 30 } },
                { "LightOrange",     { 161, 89, 12 } },
                { "LightPaleBlue",   { 23, 159, 205 } },
            };

            auto it = colors.find(currentGogglesLight);
            auto [r, g, b] = it != colors.end() ? it->second : colors.at("LightGreen");
            auto [R, G, B] = LEDEffects::RGBtoPercent(r, g, b, 1.0f);
            LEDEffects::SetLighting(R, G, B, false, false, false);
        });
    }
}
