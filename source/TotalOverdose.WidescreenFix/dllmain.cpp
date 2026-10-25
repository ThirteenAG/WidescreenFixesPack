#include "stdafx.h"
#include "FixSettings.hpp"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int Width43;
    float fWidth43;
    float fHudScale;
} Screen;

void InstallMovieLayout()
{
    auto movie = hook::pattern("55 8B EC 83 E4 F8 83 EC 6C 53 56 8B F1 8A 46 24 84 C0 57 0F 85 ? ? ? ? 8A 46 25 84 C0 0F 84 ? ? ? ? A1 ? ? ? ? 50 FF 15");
    auto draw = hook::pattern("8B CB F3 0F 11 6C 24 4C F3 0F 11 5C 24 3C F3 0F 11 44 24 78 F3 0F 11 4C 24 7C E8 ? ? ? ? 5F 5E 5B 8B E5 5D C2 04 00");
    if (movie.size() != 1 || draw.size() != 1)
    {
        OutputDebugStringA("Total Overdose Widescreen Fix: movie layout unavailable.\n");
        return;
    }
    static auto decoder = *movie.get_first<uint32_t**>(37);
    static auto layout = safetyhook::create_mid(draw.get_first(26), [](SafetyHookContext& context)
    {
        const auto video = *decoder;
        if (!video || !video[0] || !video[1] || Screen.Width <= 0 || Screen.Height <= 0)
            return;
        static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        float scaleX = Screen.fWidth / static_cast<float>(video[0]);
        float scaleY = Screen.fHeight / static_cast<float>(video[1]);
        if (mode != 2)
            scaleX = scaleY = mode == 1 ? std::max(scaleX, scaleY) : std::min(scaleX, scaleY);
        const float width = video[0] * scaleX, height = video[1] * scaleY;
        const float left = (Screen.fWidth - width) * 0.5f;
        const float top = (Screen.fHeight - height) * 0.5f;
        // The movie player has already drawn the full black background. Preserve
        // its texture, UV coordinates, colour and decoder/completion handling.
        auto points = reinterpret_cast<float**>(context.esp + 4);
        points[0][0] = left;
        points[0][1] = top;
        points[1][0] = left;
        points[1][1] = top + height;
        points[2][0] = left + width;
        points[2][1] = top + height;
        points[3][0] = left + width;
        points[3][1] = top;
    });
}

uintptr_t StoryIntroTarget(const uint8_t* startup, const uint8_t* chain)
{
    auto address = [](const uint8_t* p)
    {
        return *reinterpret_cast<const uint32_t*>(p);
    };
    auto branch = [](const uint8_t* p)
    {
        return reinterpret_cast<uintptr_t>(p + 5) + *reinterpret_cast<const int32_t*>(p + 1);
    };
    auto sci = address(startup + 9);
    auto deadline = address(chain + 19);
    auto story = address(chain + 67);
    if (!sci || !deadline || !story || sci == deadline || sci == story || deadline == story ||
        address(chain + 24) != sci || address(chain + 72) != deadline ||
        address(startup + 2) != address(chain + 1) || address(chain + 1) != address(chain + 49) ||
        address(chain + 7) != address(chain + 55) ||
        branch(startup + 20) != branch(chain + 30) || branch(chain + 30) != branch(chain + 78) ||
        branch(chain + 38) != branch(chain + 86))
        return 0;
    return story;
}

void SkipStartupLogos()
{
    auto startup = hook::pattern("8B 45 04 85 C0 5D 75 0D 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 8B 15 ? ? ? ? 52 57 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 8B 43 3C");
    auto chains = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 74 24 08 50 51 68 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE 5E E9 ? ? ? ? CC CC CC CC CC A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 74 24 08 50 51 68 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE 5E E9 ? ? ? ?");
    if (startup.size() != 1)
    {
        OutputDebugStringA("Total Overdose Widescreen Fix: unsupported startup script.\n");
        return;
    }
    auto transition = startup.get_first<uint8_t>(21);
    uintptr_t story{};
    size_t matches{};
    for (size_t i = 0; i < chains.size(); ++i)
    {
        if (auto target = StoryIntroTarget(transition, chains.get(i).get<uint8_t>()))
        {
            story = target;
            ++matches;
        }
    }
    if (matches != 1)
    {
        OutputDebugStringA("Total Overdose Widescreen Fix: unsupported logo transition chain.\n");
        return;
    }
    // Retain the native script transition and the story intro after both logos.
    ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(transition + 9)} = static_cast<uint32_t>(story);
}

void Init()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
    {
        Screen.Width = 640;
        Screen.Height = 480;
    }

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    InstallMovieLayout();

    if (settings.Bool("MAIN.SkipIntro"))
        SkipStartupLogos();

    auto resolutions = hook::pattern("C7 00 ? ? ? ? C7 40 04 ? ? ? ? 88 58 08");
    auto outputResolution = hook::pattern("B9 ? ? ? ? 89 8E 28 03 00 00 B8 ? ? ? ? 89 86 2C 03 00 00");
    auto projectionAspect = hook::pattern("F3 0F 59 15 ? ? ? ? F3 0F 5C D8 5F");
    auto cameraAspect = hook::pattern("84 C0 F3 0F 10 05 ? ? ? ? 75 08 F3 0F 10 05");
    if (resolutions.size() != 5 || outputResolution.size() != 1 || projectionAspect.size() != 1 || cameraAspect.size() != 1)
    {
        OutputDebugStringA("Total Overdose Widescreen Fix: unsupported resolution/aspect signatures.\n");
        return;
    }
    for (size_t i = 0; i < resolutions.size(); ++i)
    {
        ProtectedGameRef<int32_t>{resolutions.get(i).get<int32_t>(2)} = Screen.Width;
        ProtectedGameRef<int32_t>{resolutions.get(i).get<int32_t>(9)} = Screen.Height;
    }
    ProtectedGameRef<int32_t>{outputResolution.get_first<int32_t>(1)} = Screen.Width;
    ProtectedGameRef<int32_t>{outputResolution.get_first<int32_t>(12)} = Screen.Height;
    ProtectedGameRef<float>{*projectionAspect.get_first<float*>(4)} = Screen.fAspectRatio;
    ProtectedGameRef<float>{*cameraAspect.get_first<float*>(6)} = Screen.fAspectRatio;
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("BF 94 00 00 00 8B C7 E8"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}