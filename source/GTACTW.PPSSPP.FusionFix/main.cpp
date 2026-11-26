#include "Game.hpp"
#include "Shared/Console/PatchNotice.hpp"
#include "Shared/Console/PortableCfg.hpp"
#include <cmath>
#include <cstdlib>
#include <limits>
extern "C"
{
    PSP_MODULE_INFO("GTACTW.FusionFix", PSP_MODULE_USER, 2, 0);
}
namespace ctw
{
    namespace
    {
        SafetyMipsInline radio;
        SafetyMipsMid loading, loaded, radioStation;
        injector::hook_back<void()> boot;
        unsigned lastSettingsPoll;
        void Trim(const char*& begin, const char*& end)
        {
            const auto blank = [](char ch)
            { return ch == ' ' || ch == '\t' || ch == '\r'; };
            while (begin < end && blank(*begin))
                ++begin;
            while (end > begin && blank(end[-1]))
                --end;
        }

        bool Equals(const char* begin, const char* end, const char* name)
        {
            if (size_t(end - begin) != std::strlen(name))
                return false;
            const auto lower = [](char ch)
            { return ch >= 'A' && ch <= 'Z' ? char(ch + ('a' - 'A')) : ch; };
            for (; begin != end; ++begin, ++name)
                if (lower(*begin) != lower(*name))
                    return false;
            return true;
        }

        int Radio()
        {
            return Vehicle() != 0;
        }

        void PlayableRadioStation(SafetyMipsContext& regs)
        {
            // Special vehicles use 32 for a missing radio. Keep that vehicle
            // value until the player tunes in, and feed the native silent
            // station to the audio system in the meantime.
            if (regs.a0 == 32)
                regs.a0 = at<unsigned>(regs.s2 + 0x2D58);
        }

        void InstallRadio()
        {
            constexpr auto cyclePattern = "01 00 A5 24 2A 20 85 00";
            constexpr uint32_t stationRead[]{ 0x90C403E3u, 0x00042600u, 0x00042603u,
                0x00042600u, 0x00042603u, 0x00042600u, 0x00042603u, 0x10000008u, 0xA2442CF2u };
            constexpr uintptr_t stationOffset = 0x3C;
            constexpr size_t requiredSize = stationOffset + sizeof(stationRead);
            const auto cycle = pattern.get_first(cyclePattern, 8);
            if (!cycle || pattern.get(1, cyclePattern, 8) || pattern.text_size < requiredSize ||
                cycle < pattern.text_addr || cycle - pattern.text_addr > pattern.text_size - requiredSize ||
                at<uint32_t>(cycle + 4) != 0x8E442D58u ||
                std::memcmp(reinterpret_cast<const void*>(cycle + stationOffset), stationRead, sizeof(stationRead)) != 0)
            {
                PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
                return;
            }

            radioStation = safetymips::create_mid<&PlayableRadioStation>(cycle + stationOffset + 4);
            radio = safetymips::create_inline(Address<0x0884F52C>(), Radio);
            // Remove the two conditional skip tests, retaining their delay slots.
            injector::MakeNOP(cycle);
            injector::MakeNOP(cycle + 16);
        }

        void Boot()
        {
            boot.fun();
            Unthrottle(false);
        }
        // Settings that apply without a restart.
        void ReadSettings()
        {
            ReadAspectSettings();
            ReadHudSettings();
            modernControls = Option("ModernControls", true);
            fastLoading = Option("UnthrottleEmuDuringLoading", true);
            if (!fastLoading)
                Unthrottle(false);
            char icons[16]{};
            SettingText("MAIN", "GamepadIcons", icons, sizeof(icons));
            SelectIcons(icons);
        }
        int Install()
        {
            if (!BeginWithNotice())
                return -1;
            if (!InitializeAddresses())
            {
                PatchNotice("Fusion Fix: unsupported game code; the fix is not installed. Use a supported game version.\n");
                return -1;
            }
            // Keep movie setup and the intervening loading/notice initialization. The
            // native completion call can stop the startup movie as soon as loading ends.
            if (Option("SkipIntro", true))
                injector::WriteMemory<uint32_t>(Address<0x088D0D4C>(), 0x34050001u); // li a1, 1
            InstallAspect();
#ifndef NDEBUG
            logger.WriteF("Aspect patches: %u", unsigned(injector::detail::startup.error));
#endif
            InstallInput();
            InstallCamera();
            InstallReplay();
            if (InstallHud())
                InstallVideo();
#ifndef NDEBUG
            logger.WriteF("HUD patches: %u", unsigned(injector::detail::startup.error));
#endif
            InstallIcons();
            ReadSettings();
            if (Option("RadioInAllVehicles", true))
                InstallRadio();
            boot.fun = injector::MakeCALL(Address<0x088CFC1C>(), Boot).get();
            // Run before each virtual loading notification. The game still calls
            // its original method with its own this adjustment and return value.
            loading = safetymips::create_mid(Address<0x08992490>(), [](SafetyMipsContext&)
                                             { if (fastLoading) Unthrottle(true); });
            loaded = safetymips::create_mid(Address<0x089924B0>(), [](SafetyMipsContext&)
                                            { Unthrottle(false); });
            const int result = Finish();
            if (result == 0 && fastLoading)
                Unthrottle(true);
            return result;
        }
    }

    bool SettingText(const char* section, const char* key, char* value, size_t capacity)
    {
        if (!inireader.iniBuf || inireader.iniBufSize <= 0)
            return false;
        const char* cursor = inireader.iniBuf;
        const char* end = cursor + inireader.iniBufSize;
        if (end - cursor >= 3 && std::memcmp(cursor, "\xEF\xBB\xBF", 3) == 0)
            cursor += 3;
        bool active = false;
        while (cursor < end)
        {
            const char* begin = cursor;
            while (cursor < end && *cursor != '\n')
                ++cursor;
            const char* stop = cursor;
            if (cursor < end)
                ++cursor;
            for (auto scan = begin; scan != stop; ++scan)
                if (*scan == ';' || *scan == '#')
                {
                    stop = scan;
                    break;
                }
            Trim(begin, stop);
            if (begin == stop)
                continue;
            if (*begin == '[')
            {
                active = stop[-1] == ']';
                if (active)
                {
                    ++begin;
                    --stop;
                    Trim(begin, stop);
                    active = Equals(begin, stop, section);
                }
                continue;
            }
            const auto equals = static_cast<const char*>(std::memchr(begin, '=', size_t(stop - begin)));
            if (!active || !equals)
                continue;
            auto keyEnd = equals;
            Trim(begin, keyEnd);
            if (!Equals(begin, keyEnd, key))
                continue;
            begin = equals + 1;
            Trim(begin, stop);
            const auto length = size_t(stop - begin);
            if (!length || length >= capacity)
                return false;
            std::memcpy(value, begin, length);
            value[length] = '\0';
            return true;
        }
        return false;
    }

    float SettingFloat(const char* section, const char* key, float fallback)
    {
        char text[48]{};
        if (!SettingText(section, key, text, sizeof(text)))
            return fallback;
        char* end{};
        const float value = std::strtof(text, &end);
        return end != text && !*end && std::isfinite(value) ? value : fallback;
    }

    int SettingInteger(const char* section, const char* key, int fallback)
    {
        // The settings editor may serialize an integral value with a decimal point.
        const float value = SettingFloat(section, key, float(fallback));
        if (double(value) < std::numeric_limits<int>::min() ||
            double(value) > std::numeric_limits<int>::max())
            return fallback;
        const int integral = int(value);
        return float(integral) == value ? integral : fallback;
    }

    bool Option(const char* key, bool fallback)
    {
        char value[8]{};
        if (!SettingText("MAIN", key, value, sizeof(value)))
            return fallback;
        const auto end = value + std::strlen(value);
        if (Equals(value, end, "1") || Equals(value, end, "true") || Equals(value, end, "yes") ||
            Equals(value, end, "on") || Equals(value, end, "y"))
            return true;
        if (Equals(value, end, "0") || Equals(value, end, "false") || Equals(value, end, "no") ||
            Equals(value, end, "off") || Equals(value, end, "n"))
            return false;
        return fallback;
    }
    void PollSettings()
    {
        const unsigned now = sceKernelGetSystemTimeLow();
        FlushCameraSave(now);
        if (now - lastSettingsPoll < 1000000u)
            return;
        lastSettingsPoll = now;
        if (!GameplayApp() && !CutsceneApp())
            return;

        // A same-length edit can keep the same memory-stick timestamp. Read the
        // current file and only apply it after a complete read.
        SceIoStat stat{};
        if (sceIoGetstat(cfgPath, &stat) < 0 || stat.st_size <= 0 || uint64_t(stat.st_size) >= IniCapacity)
            return;
        inireader.SetIniPath(cfgPath);
        if (inireader.iniBufSize == stat.st_size)
            ReadSettings();
    }
}
extern "C" int module_start(SceSize, void*)
{
    if (!console::portable::StartReported("CTW", ctw::cfgPath))
        return 0;
    // Only CFG is opened; the old fix INI is never read or migrated.
    console::portable::IniValue values[] = {
        {"MAIN", "SkipIntro", "1"},
        {"MAIN", "ForceAspectRatio", "Auto"},
        {"MAIN", "ModernControls", "1"},
        {"MAIN", "GamepadIcons", "Auto"},
        {"MAIN", "Enable3rdPersonCamera", "1"},
        {"MAIN", "RadioInAllVehicles", "1"},
        {"MAIN", "UnthrottleEmuDuringLoading", "1"},
        {"MAIN", "UnlockXinMissionsInReplayBoard", "0"},
        {"MAIN", "HudScale", "1.0"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"},
        {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "CutsceneBorders", "3"},
    };
    (void)console::portable::CompleteCfg(ctw::cfgPath, values);
    return ctw::Install();
}
