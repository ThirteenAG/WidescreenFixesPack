#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudOffset;
} Screen;

struct HudQuad
{
    float x[4];
    float y[4];
    float u[4];
    float v[4];
    uint32_t color[4];
    int32_t texture;
    uint32_t blend;
    uint8_t state[2];
};

template<size_t index>
struct HudVertexHook
{
    void operator()(injector::reg_pack& regs)
    {
        const auto* quad = reinterpret_cast<const HudQuad*>(regs.eax);
        const auto buffer = *reinterpret_cast<uintptr_t*>(regs.esi + 0x14);
        const auto count = *reinterpret_cast<uint32_t*>(regs.esi);
        auto* vertex = reinterpret_cast<float*>(buffer + count * 0x1C);
        vertex[0] = quad->x[index] - 0.5f;
        vertex[1] = quad->y[index] - 0.5f;
    }
};

int32_t(__cdecl* QueueHudQuad)(HudQuad*);

namespace Textures
{
    enum Type : uint8_t { Other, Menu, Backdrop, Scope, Binoculars };
    Type Types[250]{}; //texture manager capacity
    SafetyHookInline RegisterTexture;

    int32_t __cdecl Register(const char* name, int32_t group, bool* created)
    {
        bool added = false;
        int32_t id = RegisterTexture.unsafe_ccall<int32_t>(name, group, &added);
        if (id == -1)
            return id;

        if (created)
            *created = added;

        if (added)
        {
            static constexpr struct { const char* name; Type type; } textures[] = {
                { "\\Splash\\legal.dds", Backdrop },
                { "\\Splash\\mc2logo.dds", Backdrop },
                { "\\Splash\\namcolog.dds", Backdrop },
                { "\\Splash\\frontscr.dds", Menu },
                { "\\Splash\\backscr.dds", Backdrop },
                { "\\Splash\\oldmenu1.dds", Backdrop },
                { "\\Splash\\profskin.dds", Menu },
                { "\\Splash\\profedit.dds", Menu },
                { "\\Splash\\profsel.dds", Menu },
                { "\\Splash\\Menu25.dds", Menu },
                { "\\HUD\\mauserpc.tga", Scope },
                { "\\HUD\\naganpc.tga", Scope },
                { "\\HUD\\springpc.tga", Scope },
                { "\\HUD\\Binos.tga", Binoculars }
            };
            Types[id] = Other; //handles can get reused after unloading
            for (const auto& texture : textures)
            {
                if (_stricmp(name, texture.name) == 0)
                {
                    Types[id] = texture.type;
                    break;
                }
            }
        }
        return id;
    }
}

namespace UI
{
    enum DrawMode { Game, Menu };
    DrawMode Mode = Game;
    bool CenteredText = false;
    int32_t FontWidth;
    SafetyHookInline MenuDraw, MenuDrawOverlay, PrintText, PixelClip, DrawLine;
    SafetyHookMid MouseUpdate;
    void(__cdecl* SetViewingArea)(const float*);

    template <DrawMode drawMode, SafetyHookInline& hook, typename Result = int32_t, typename... Args>
    Result __cdecl Draw(Args... args)
    {
        auto mode = std::exchange(Mode, drawMode);
        auto result = hook.template unsafe_ccall<Result>(args...);
        Mode = mode;
        return result;
    }

    SafetyHookInline LoadingFrame, LoadingText, LoadingProgress, LoadingPercent;
    int32_t* LoadingTexture = nullptr;
    void(__cdecl* FlushQuads)();

    void DrawBackdrop(uint32_t color, bool bordersOnly = false)
    {
        const float width = bordersOnly ? Screen.fHudOffset : Screen.fWidth;
        HudQuad background = { { 0.0f, width, width, 0.0f }, { 0.0f, 0.0f, Screen.fHeight, Screen.fHeight } };
        std::fill(std::begin(background.color), std::end(background.color), color & 0xFF000000);
        background.texture = -1;
        background.blend = 1;
        background.state[0] = background.state[1] = 1;
        QueueHudQuad(&background);
        if (bordersOnly) //cover map icons outside the frame
        {
            background.x[0] = background.x[3] = Screen.fWidth - width;
            background.x[1] = background.x[2] = Screen.fWidth;
            QueueHudQuad(&background);
        }
        FlushQuads(); //backdrop before foreground so the level map can be visible
    }

    void UpdateMouse(injector::reg_pack& regs)
    {
        auto& x = *reinterpret_cast<float*>(regs.esp + sizeof(uintptr_t));
        x -= Screen.fHudOffset;
    }

    char __cdecl Print(const wchar_t* text, const float* clip, float x, float y,
        float spacing, float scale, uint32_t color, uint32_t format)
    {
        auto centered = std::exchange(CenteredText, Mode != Game || (format & 1));
        auto width = std::exchange(FontWidth, CenteredText ? Screen.Width43 : Screen.Width);
        auto result = PrintText.unsafe_ccall<char>(text, clip, x, y, spacing, scale, color, format);
        FontWidth = width;
        CenteredText = centered;
        return result;
    }

    float* __cdecl GetPixelClip(float* rect, float* x, float* y, const float* clip, uint32_t format)
    {
        auto width = std::exchange(FontWidth, (Mode != Game || (format & 1)) ? Screen.Width43 : Screen.Width);
        auto result = PixelClip.unsafe_ccall<float*>(rect, x, y, clip, format);
        FontWidth = width;
        return result;
    }

    void __cdecl Line(const float* from, const float* to, uint32_t color)
    {
        const float offset = Mode != Game ? Screen.fHudOffset : 0.0f;
        const float a[] = { from[0] + offset, from[1] };
        const float b[] = { to[0] + offset, to[1] };
        DrawLine.unsafe_ccall(a, b, color);
    }

    void __cdecl ProfileViewingArea(const float* area)
    {
        const float ratio = Screen.fWidth43 / Screen.fWidth;
        const float viewport[] = { area[0] * ratio, area[1],
            area[2] * ratio + Screen.fHudOffset / Screen.fWidth, area[3] };
        SetViewingArea(viewport);
    }

    void RedirectWidth(const uint8_t* begin, const uint8_t* end, uintptr_t nativeWidth, const int32_t* width)
    {
        hook::range_pattern((uintptr_t)begin, (uintptr_t)end, pattern_str(to_bytes(nativeWidth))).for_each_result(
            [=](hook::pattern_match match) { injector::WriteMemory(match.get<uint32_t>(), width, true); });
    }
}

inline bool Near(float a, float b)
{
    return fabsf(a - b) < 2.0f;
}

int32_t __cdecl sub_48B140Hook(HudQuad* quad)
{
    const auto type = (uint32_t)quad->texture < _countof(Textures::Types) ? Textures::Types[quad->texture] : Textures::Other;
    float scale = (UI::Mode != UI::Game || UI::CenteredText) ? 1.0f : 0.0f;
    if (!scale && UI::LoadingTexture && quad->texture != -1 && Screen.fHudOffset > 0.0f &&
        Near(quad->x[0], 0.0f) && Near(quad->x[1], Screen.fWidth))
    {
        bool loading = quad->texture == *UI::LoadingTexture;
        if (loading || type == Textures::Menu || type == Textures::Backdrop)
        {
            if (loading || type == Textures::Backdrop)
                UI::DrawBackdrop(quad->color[0]);

            scale = Screen.fWidth43 / Screen.fWidth;
        }
    }

    if (scale)
    {
        HudQuad centered{};
        memcpy(&centered, quad, 0x5A);
        for (float& x : centered.x)
            x = Screen.fHudOffset + x * scale;
        return QueueHudQuad(&centered);
    }

    const float right = Screen.fWidth - Screen.fHudOffset;

    if (Screen.fHudOffset > 0.0f && Near(quad->x[0], Screen.fHudOffset) &&
        (type == Textures::Scope || type == Textures::Binoculars))
    {
        HudQuad Strip{};
        memcpy(&Strip, quad, 0x5A);

        const bool binoculars = type == Textures::Binoculars;
        const float uWidth = quad->u[1] - quad->u[0];
        const float scopeBorder = 32.5f / 1024.0f;
        const float scopeInset = uWidth * scopeBorder;

        const float leftU = binoculars ? 0.5f / 512.0f : quad->u[0] + scopeInset;
        const float rightU = binoculars ? 0.5f / 512.0f : quad->u[1] - scopeInset;

        const float borderWidth = binoculars ? 0.0f : (right - Screen.fHudOffset) * scopeBorder;
        const float centerLeft = Screen.fHudOffset + borderWidth;
        const float centerRight = right - borderWidth;

        Strip.x[0] = Strip.x[3] = 0.0f;
        Strip.x[1] = Strip.x[2] = centerLeft;
        Strip.u[0] = Strip.u[1] = Strip.u[2] = Strip.u[3] = leftU;
        QueueHudQuad(&Strip);

        Strip.x[0] = Strip.x[3] = centerRight;
        Strip.x[1] = Strip.x[2] = Screen.fWidth;
        Strip.u[0] = Strip.u[1] = Strip.u[2] = Strip.u[3] = rightU;
        QueueHudQuad(&Strip);

        HudQuad Center{};
        memcpy(&Center, quad, 0x5A);

        Center.x[0] = Center.x[3] = centerLeft;
        Center.x[1] = Center.x[2] = centerRight;
        if (!binoculars)
        {
            Center.u[0] = Center.u[3] = leftU;
            Center.u[1] = Center.u[2] = rightU;
        }
        return QueueHudQuad(&Center);
    }
    return QueueHudQuad(quad);
}

SafetyHookMid ReticleHorizontal[3], ReticleVertical[3];

inline float ReticleX(float x)
{
    return 0.5f + (x - 0.5f) * (Screen.fWidth - 2.0f * Screen.fHudOffset) / Screen.fWidth;
}

void AdjustReticleHorizontal(injector::reg_pack& regs)
{
    auto* args = reinterpret_cast<float*>(regs.esp);
    args[0] = ReticleX(args[0]);
    args[1] = ReticleX(args[1]);
}
void AdjustReticleVertical(injector::reg_pack& regs)
{
    auto& x = *reinterpret_cast<float*>(regs.esp);
    x = ReticleX(x);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    float FOV = iniReader.ReadFloat("MAIN", "FOV", 75.f);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    auto pattern = hook::pattern("C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? 50 A1"); //622B17
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(6), Screen.Height, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(16), Screen.Width, true);

    pattern = hook::pattern("8B ? ? ? ? ? 8B ? ? ? ? ? 8B ? 99 2B ? D1 ? 89"); //48B0D6
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.Width;
            regs.esi = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12));

    pattern = hook::pattern("8B ? ? ? 57 8B ? ? ? 50 6A"); //48B076
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edi = Screen.Width;
            regs.esi = Screen.Height;
        }
    }; injector::MakeInline<SetResHook2>(pattern.count(1).get(0).get<uint32_t>(1), pattern.count(1).get(0).get<uint32_t>(9));
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x57, true); //push edi

    pattern = hook::pattern("D9 ? ? ? 8B ? ? ? D9 ? ? ? ? ? A3 ? ? ? ? DA"); //4152C0
    struct SetScalingHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.eax = Screen.fAspectRatio;
        }
    }; injector::MakeInline<SetScalingHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(10), Screen.fAspectRatio, true);
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(15), Screen.fAspectRatio, true);

    pattern = hook::pattern("D8 ? ? ? ? ? 83 ? ? 6A ? 68 ? ? ? ? 51 D9 ? ? E8 ? ? ? ? 83 ? ? 83"); //4A15F6
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(2), AdjustFOV(FOV, Screen.fAspectRatio), true);

    Screen.fHudOffset = (Screen.fWidth - Screen.Width43) / 2.0f;

    pattern = hook::pattern("51 56 8B ? ? ? 85 ? 0F ? ? ? ? ? 80"); //408EE0
    Textures::RegisterTexture = safetyhook::create_inline(pattern.get_first(), Textures::Register);

    pattern = hook::pattern("56 8B ? 56 E8 ? ? ? ? 8A"); //48B140
    uint8_t* drawHudQuad = pattern.count(1).get(0).get<uint8_t>(0);
    QueueHudQuad = reinterpret_cast<decltype(QueueHudQuad)>(injector::GetBranchDestination(drawHudQuad + 4, true).as_int());
    injector::MakeCALL(drawHudQuad + 4, sub_48B140Hook, true);

    const auto quadVertices = hook::get_pattern<uint8_t>("8B ? 89 ? ? 8B ? 8B ? ? 6B ? ? 8B"); //487E97
    injector::MakeInline<HudVertexHook<0>>(quadVertices, quadVertices + 0x14);
    injector::MakeInline<HudVertexHook<1>>(quadVertices + 0x69, quadVertices + 0x7E);
    injector::MakeInline<HudVertexHook<2>>(quadVertices + 0xD0, quadVertices + 0xE5);
    injector::MakeInline<HudVertexHook<3>>(quadVertices + 0x12F, quadVertices + 0x14C); //Replace the native XYZRHW coordinate writes with half-pixel alignment.

    pattern = hook::pattern("83 ? ? 56 8B ? 8B ? ? 85 ? 57 74 ? 8B ? 85"); //507C40
    uint8_t* drawScopeReticle = pattern.count(1).get(0).get<uint8_t>(0);
    static constexpr int32_t horizontalCalls[] = { 0x31F, 0x3BC, 0x3E9 };
    static constexpr int32_t verticalCalls[] = { 0x33C, 0x401, 0x416 };
    for (size_t i = 0; i < _countof(ReticleHorizontal); ++i)
        ReticleHorizontal[i] = safetyhook::create_mid(drawScopeReticle + horizontalCalls[i], AdjustReticleHorizontal);
    for (size_t i = 0; i < _countof(ReticleVertical); ++i)
        ReticleVertical[i] = safetyhook::create_mid(drawScopeReticle + verticalCalls[i], AdjustReticleVertical);

    static float fHudScale2 = (0.0009765625f / Screen.fAspectRatio) * (4.0f / 3.0f);
    pattern = hook::pattern("D8 ? ? ? ? ? D9 ? ? D9 ? ? ? D8 ? ? ? ? ? D9 ? ? ? ? ? D9 ? ? ? D8"); //502232
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fHudScale2, true); //text size and radar arrows

    pattern = hook::pattern("8B ? ? 89 ? ? 83 ? ? ? ? ? ? 75 ? A1"); //5021C2
    struct HudHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x28) += Screen.fHudOffset;
            *(float*)(regs.esi + 0x30) -= Screen.fHudOffset * 2.0f;

            regs.eax = *(uintptr_t*)(regs.edi + 0x4);
            *(uintptr_t*)(regs.esi + 0x44) = regs.eax;
        }
    }; injector::MakeInline<HudHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

    const auto fontClip = hook::get_pattern<uint8_t>("8B ? ? ? 83 ? ? 53 8B ? ? ? 85 ? 56"); //40E450
    const auto printText = hook::get_pattern<uint8_t>("83 ? ? A0 ? ? ? ? 84 ? 55 57"); //40F570
    const auto nativeWidth = *hook::get_pattern<uintptr_t>("DB ? ? ? ? ? D8 ? D9 ? DB ? ? ? ? ? D8 ? D9", 2); //40E4FD
    UI::RedirectWidth(fontClip, printText + 0x650, nativeWidth, &UI::FontWidth); //clip, glyphs and text output; end before text height
    UI::PixelClip = safetyhook::create_inline(fontClip, UI::GetPixelClip);
    UI::PrintText = safetyhook::create_inline(printText, UI::Print);

    const auto menuDraw = hook::get_pattern<uint8_t>("8B 0D ? ? ? ? E9 ? ? ? ? ? ? ? ? ? 8B 0D"); //4819F0
    const auto menuItems = hook::get_pattern<uint8_t>("51 56 8B ? 8B ? ? 50 E8 ? ? ? ? 0F ? ? ? 8B"); //47DFC0
    const auto frontendItems = hook::get_pattern<uint8_t>("56 57 8B ? 8B ? FF ? ? 8B ? ? 8B ? 3B"); //4AF0A0
    const auto frontendLayout = hook::get_pattern<uint8_t>("A1 ? ? ? ? 56 8B ? ? ? 50 8B"); //4D5DF0
    const auto frontendEnd = hook::get_pattern<uint8_t>("83 ? ? A1 ? ? ? ? 89 ? ? ? A0 ? ? ? ? 84 ? 0F ? ? ? ? ? A1 ? ? ? ? 84"); //4FE850

    const auto menuFontScale = hook::get_pattern<uint8_t>("DB ? ? ? ? ? D8 ? ? ? ? ? DB ? ? ? ? ? D8 ? ? ? ? ? D9"); //4D4340
    UI::RedirectWidth(menuItems, menuDraw, nativeWidth, &Screen.Width43); //controls and hitboxes; end before menu dispatch
    UI::RedirectWidth(frontendItems, menuFontScale + 0x30, nativeWidth, &Screen.Width43); //frontend controls through font scale
    UI::RedirectWidth(frontendLayout, frontendEnd, nativeWidth, &Screen.Width43); //frontend layout through menu text sizing
    UI::MenuDraw = safetyhook::create_inline(menuDraw, UI::Draw<UI::Menu, UI::MenuDraw>);
    UI::MenuDrawOverlay = safetyhook::create_inline(menuDraw + 0x10, UI::Draw<UI::Menu, UI::MenuDrawOverlay, char>);

    const auto loadingImage = hook::get_pattern<uint8_t>("A1 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 6A"); //4D49E0
    UI::LoadingTexture = *reinterpret_cast<int32_t**>(loadingImage + 1);
    UI::FlushQuads = reinterpret_cast<decltype(UI::FlushQuads)>(injector::GetBranchDestination(loadingImage + 0x66, true).as_int());

    const auto loadingFrame = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? 56 8B ? ? ? 83"); //4D4770
    const auto loadingText = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? A1 ? ? ? ? 56"); //4D4840
    const auto loadingProgress = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? 83 ? ? DB ? ? ? ? ? 8D"); //4D4930
    const auto loadingPercent = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? 56 83"); //4D4CF0
    UI::RedirectWidth(loadingFrame, loadingImage, nativeWidth, &Screen.Width43); //frame, text and progress; exclude background image
    UI::RedirectWidth(loadingPercent, loadingPercent + 0x160, nativeWidth, &Screen.Width43); //percentage overlay; end before loading setup
    UI::LoadingFrame = safetyhook::create_inline(loadingFrame, UI::Draw<UI::Menu, UI::LoadingFrame, char, int32_t>);
    UI::LoadingText = safetyhook::create_inline(loadingText, UI::Draw<UI::Menu, UI::LoadingText, int32_t, int32_t>);
    UI::LoadingProgress = safetyhook::create_inline(loadingProgress, UI::Draw<UI::Menu, UI::LoadingProgress, int32_t, float>);
    UI::LoadingPercent = safetyhook::create_inline(loadingPercent, UI::Draw<UI::Menu, UI::LoadingPercent, int32_t, float>);

    pattern = hook::pattern("A0 ? ? ? ? 83 ? ? 84 ? 0F ? ? ? ? ? DB"); //481B50
    UI::MouseUpdate = safetyhook::create_mid(pattern.get_first(), UI::UpdateMouse);
    pattern = hook::pattern("8B ? ? ? 50 8B ? ? ? 8B ? ? 8B ? 8B ? ? ? 51"); //486BB0
    UI::DrawLine = safetyhook::create_inline(pattern.get_first(), UI::Line);

    pattern = hook::pattern("8D ? ? ? 52 E8 ? ? ? ? 83 ? ? 8B ? E8"); //4C7F24
    auto profileViewportCall = pattern.get_first<uint8_t>(5);
    UI::SetViewingArea = reinterpret_cast<decltype(UI::SetViewingArea)>(injector::GetBranchDestination(profileViewportCall, true).as_int());
    injector::MakeCALL(profileViewportCall, UI::ProfileViewingArea, true); //profile viewport
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("BF ? ? ? ? 8B ? E8 ? ? ? ? 89 ? ? 8B"));
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