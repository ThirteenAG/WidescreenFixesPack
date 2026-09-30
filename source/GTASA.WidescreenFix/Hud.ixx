module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Hud;

import Skeleton;
import Draw;
import Sprite2d;
import Camera;
import Frontend;
import Menu;

union tScriptParam
{
    unsigned int uParam;
    int iParam;
    float fParam;
    void* pParam;
    char* szParam;
};

class CRunningScript
{
public:
    CRunningScript* m_pNext;
    CRunningScript* m_pPrev;
    char            m_szName[8];
    unsigned char* m_pBaseIP;
    unsigned char* m_pCurrentIP;
    unsigned char* m_apStack[8];
    unsigned short  m_nSP;
private:
    char _pad3A[2];
public:
    tScriptParam	m_aLocalVars[32];
    int             m_anTimers[2];
    bool            m_bIsActive;
    bool            m_bCondResult;
    bool            m_bUseMissionCleanup;
    bool            m_bIsExternal;
    bool            m_bTextBlockOverride;
private:
    char _padC9[3];
public:
    int             m_nWakeTime;
    unsigned short  m_nLogicalOp;
    bool            m_bNotFlag;
    bool            m_bWastedBustedCheck;
    bool            m_bWastedOrBusted;
private:
    char _padD5[3];
public:
    unsigned char* m_pSceneSkipIP;
    bool            m_bIsMission;
private:
    char _padDD[3];
};
static_assert(offsetof(CRunningScript, m_pCurrentIP) == 0x14);

enum class ScriptLayout : uint8_t { Center, LeftPanels, RightPanels, WindowPanels, Table };

// These scripts draw HUD panels, rather than a board/game in a fixed canvas.
// Record the producer at submission time: several scripts can draw in one frame.
static ScriptLayout GetScriptLayout(const CRunningScript* script)
{
    const size_t len = std::find(script->m_szName, script->m_szName + 8, '\0') - script->m_szName;
    const std::string_view name(script->m_szName, len);
    for (auto hudScript : { "pool2", "lowr", "wof", "sweet6" })
    {
        if (name.size() == strlen(hudScript) && _strnicmp(name.data(), hudScript, name.size()) == 0)
            return ScriptLayout::LeftPanels;
    }
    for (auto hudScript : { "toreno1", "tria", "mtbiker", "cprace" })
    {
        if (name.size() == strlen(hudScript) && _strnicmp(name.data(), hudScript, name.size()) == 0)
            return ScriptLayout::RightPanels;
    }
    for (auto hudScript : { "blackj", "roulete" })
    {
        if (name.size() == strlen(hudScript) && _strnicmp(name.data(), hudScript, name.size()) == 0)
            return ScriptLayout::WindowPanels;
    }
    if (name.size() == 3 && _strnicmp(name.data(), "otb", 3) == 0)
        return ScriptLayout::Table;
    return ScriptLayout::Center;
}

struct ScriptText
{
    CVector2D scale;
    CRGBA color;
    bool justify, centered, background, backgroundOnlyText;
    float wrapX, centerSize;
    CRGBA backgroundColor;
    uint8_t proportional;
    uint8_t dropColor[4];
    uint8_t shadow, outline, beforeFade, rightJustify;
    int32_t font;
    CVector2D position;
    char key[8];
    int32_t numbers[2];
};
static_assert(sizeof(ScriptText) == 0x44);
static_assert(offsetof(ScriptText, position) == 0x2C);

struct ScriptRectangle
{
    int32_t type;
    uint8_t beforeFade, unused;
    int16_t texture;
    CVector2D cornerA, cornerB;
    float angle;
    CRGBA color;
    char key[8];
    int16_t unused2;
    char message[8];
    int16_t unused3;
    int32_t alignment, style;
};
static_assert(sizeof(ScriptRectangle) == 0x3C);
static_assert(offsetof(ScriptRectangle, cornerA) == 0x08);

GameRef<std::array<ScriptText, 96>> ScriptTexts;
GameRef<std::array<ScriptRectangle, 128>> ScriptRectangles;
GameRef<uint16_t> ScriptTextCount;
GameRef<uint16_t> ScriptRectangleCount;
std::array<ScriptLayout, 96> ScriptTextLayouts{};
std::array<ScriptLayout, 128> ScriptRectangleLayouts{};
std::array<const CRunningScript*, 96> ScriptTextOwners{};
std::array<const CRunningScript*, 128> ScriptRectangleOwners{};

std::array<SafetyHookInline, 24> ScriptCommandHooks;
template<size_t Index>
char __fastcall ProcessScriptCommand(CRunningScript* script, void*, int32_t opcode)
{
    // Only drawing opcodes need ownership tracking; keep the usual dispatch cheap.
    const bool drawsText = opcode == 0x033E || opcode == 0x045A || opcode == 0x045B || opcode == 0x051F || opcode == 0x07FC;
    const bool drawsRect = opcode == 0x038D || opcode == 0x038E || opcode == 0x074B || opcode == 0x0880 || opcode == 0x08AE || opcode == 0x0937;
    if (!drawsText && !drawsRect)
        return ScriptCommandHooks[Index].unsafe_thiscall<char>(script, opcode);

    const auto textIndex = ScriptTextCount.get();
    const auto rectIndex = ScriptRectangleCount.get();
    const auto layout = GetScriptLayout(script);
    const char result = ScriptCommandHooks[Index].unsafe_thiscall<char>(script, opcode);
    for (size_t i = textIndex; i < std::min<size_t>(ScriptTextCount.get(), ScriptTextLayouts.size()); ++i)
    {
        ScriptTextLayouts[i] = layout;
        ScriptTextOwners[i] = script;
    }
    for (size_t i = rectIndex; i < std::min<size_t>(ScriptRectangleCount.get(), ScriptRectangleLayouts.size()); ++i)
    {
        ScriptRectangleLayouts[i] = layout;
        ScriptRectangleOwners[i] = script;
    }
    return result;
}

bool bScriptShadowToOutline = false;
float (__cdecl* GetStringWidth)(const char*, bool, bool) = nullptr;
injector::hook_back<float(__cdecl*)(uint8_t)> hbGetScriptLetterSize;
float __cdecl GetScriptLetterSize(uint8_t letter)
{
    const auto index = ScriptTextCount.get();
    if (index >= ScriptTexts->size())
        return hbGetScriptLetterSize.fun(letter);

    // Match the outline used by DrawScriptText. Leave GetStringWidth's entry
    // intact: the Hoodlum executable uses its bytes in protection callbacks.
    auto& text = (*ScriptTexts)[index];
    const auto outline = text.outline;
    if (!outline && bScriptShadowToOutline) text.outline = text.shadow;
    const float width = hbGetScriptLetterSize.fun(letter);
    text.outline = outline;
    return width;
}

void* ScriptTextDictionary = nullptr;
const char* (__fastcall* GetScriptString)(void*, void*, const char*) = nullptr;
void (__cdecl* InsertScriptNumbers)(const char*, int, int, int, int, int, int, char*) = nullptr;

static float GetScriptTextWidth(size_t index)
{
    const auto& text = (*ScriptTexts)[index];
    char key[9] = {};
    memcpy(key, text.key, sizeof(text.key));
    const char* string = GetScriptString(ScriptTextDictionary, nullptr, key);
    char formatted[400];
    InsertScriptNumbers(string, text.numbers[0], text.numbers[1], -1, -1, -1, -1, formatted);
    // The game's script-width routine reads the style of the current entry.
    const auto count = ScriptTextCount.get();
    *ScriptTextCount = static_cast<uint16_t>(index);
    const float width = GetStringWidth(formatted, true, true);
    *ScriptTextCount = count;
    return width;
}

static void FitScriptText(size_t index)
{
    auto& text = (*ScriptTexts)[index];
    if (text.centered || text.rightJustify || text.justify) return;
    float available = 0.0f;
    const std::string_view key(text.key, std::find(text.key, text.key + 8, '\0') - text.key);
    if (ScriptTextLayouts[index] == ScriptLayout::LeftPanels && (key == "PL_05" || key == "PL_06"))
    {
        // Pool puts ball numbers on the next fixed baseline. Keep this label
        // on one line, using the localized text and the script's own wrap edge.
        available = text.wrapX - text.position.x - 0.5f;
    }
    else if (ScriptTextLayouts[index] == ScriptLayout::Table)
    {
        // Inside Track names and right-aligned odds share a row. Fit the name
        // to the actual odds width, including substituted numbers and outlines.
        float right = text.wrapX;
        for (size_t i = 0; i < ScriptTexts->size(); ++i)
        {
            const auto& column = (*ScriptTexts)[i];
            if (!column.key[0] || !column.rightJustify || column.beforeFade != text.beforeFade
                || ScriptTextOwners[i] != ScriptTextOwners[index] || column.position.y != text.position.y
                || column.position.x <= text.position.x) continue;
            right = std::min(right, column.position.x - GetScriptTextWidth(i) - 8.0f);
        }
        if (right < text.wrapX) available = right - text.position.x;
    }
    if (available > 0.0f)
    {
        const float width = GetScriptTextWidth(index);
        if (width > available) text.scale.x *= available / width;
    }
}

GameRef<float*> HelpTextScaleX;

static float GetScriptLeftOffset()
{
    // Help text starts at 34 HUD units; its background extends five pixels left.
    // Use the game's current scale, including HudWidthScale, to align 29-unit
    // script panels with that edge while preserving their relative spacing.
    const float helpBoxMargin = SCREEN_WIDTH * *HelpTextScaleX.get() * 34.0f - 5.0f;
    return -fWidescreenSCMOffset + helpBoxMargin - SCREEN_SCALE_X(29.0f);
}

static float GetScriptOffset(ScriptLayout layout, float left, float right)
{
    if (layout == ScriptLayout::LeftPanels && right < 240.0f)
        return GetScriptLeftOffset();
    if (layout == ScriptLayout::RightPanels && left > 400.0f)
        return -2.0f * fWidescreenHudOffset43 + fWidescreenSCMOffset;
    return -fWidescreenHudOffset43;
}

static float GetScriptRectangleOffset(size_t index, const ScriptRectangle& rect, float scaleX)
{
    // TITLE_AND_MESSAGE uses cornerB.x for the colour, not a coordinate.
    const float right = rect.type == 1 ? rect.cornerA.x : rect.cornerB.x;
    const float leftX = std::min(rect.cornerA.x, right) / scaleX;
    const float rightX = std::max(rect.cornerA.x, right) / scaleX;
    auto layout = ScriptRectangleLayouts[index];
    if (layout == ScriptLayout::WindowPanels && rect.type == 2)
    {
        if (rightX < 240.0f) layout = ScriptLayout::LeftPanels;
        else if (leftX > 400.0f) layout = ScriptLayout::RightPanels;
    }
    return GetScriptOffset(layout, leftX, rightX);
}

static float GetScriptTextOffset(size_t index, const ScriptText& text, float scaleX)
{
    if (ScriptTextLayouts[index] != ScriptLayout::Center)
    {
        // Keep each window's text with its background. This also preserves the
        // right-hand column of centered race results and casino game boards.
        const float x = text.position.x * scaleX;
        const float y = text.position.y * SCREEN_HEIGHT / 448.0f;
        for (size_t i = 0; i < ScriptRectangles->size(); ++i)
        {
            const auto& rect = (*ScriptRectangles)[i];
            if (rect.type != 2 || rect.beforeFade != text.beforeFade || ScriptRectangleOwners[i] != ScriptTextOwners[index]) continue;
            if (x >= std::min(rect.cornerA.x, rect.cornerB.x) && x <= std::max(rect.cornerA.x, rect.cornerB.x)
                && y >= std::min(rect.cornerA.y, rect.cornerB.y) && y <= std::max(rect.cornerA.y, rect.cornerB.y))
                return GetScriptRectangleOffset(i, rect, scaleX);
        }
    }
    return GetScriptOffset(ScriptTextLayouts[index], text.position.x, text.position.x);
}

export namespace CFont
{
    void (__cdecl* DrawFonts)() = nullptr;
}

export namespace CHud
{
    SafetyHookInline shDrawScriptText = {};
    void __cdecl DrawScriptText(char priority)
    {
        CFont::DrawFonts();
        // Text is stored in script units; rectangles were scaled by the opcode.
        // Move the records before layout so clipping, wrapping and backgrounds agree.
        auto texts = ScriptTexts.get();
        auto rectangles = ScriptRectangles.get();
        const float scaleX = SCREEN_SCALE_X(1.0f);
        for (size_t i = 0; i < texts.size(); ++i)
        {
            auto& text = (*ScriptTexts)[i];
            if (!text.key[0] || text.beforeFade != priority) continue;
            FitScriptText(i);
            const float offset = GetScriptTextOffset(i, text, scaleX) / scaleX;
            text.position.x += offset;
            if (!text.centered && !text.rightJustify) text.wrapX += offset;
        }
        for (size_t i = 0; i < rectangles.size(); ++i)
        {
            auto& rect = (*ScriptRectangles)[i];
            if (!rect.type || rect.beforeFade != priority) continue;
            const float offset = GetScriptRectangleOffset(i, rect, scaleX);
            rect.cornerA.x += offset;
            if (rect.type != 1) rect.cornerB.x += offset;
        }
        shDrawScriptText.unsafe_ccall(priority);
        CFont::DrawFonts();
        *ScriptTexts = texts;
        *ScriptRectangles = rectangles;
    }
}

class HudDrawScope
{
    float previousOffset;
public:
    HudDrawScope(float offset) : previousOffset(g_drawOffsetX)
    {
        CFont::DrawFonts();
        g_drawOffsetX = offset;
    }
    ~HudDrawScope()
    {
        CFont::DrawFonts();
        g_drawOffsetX = previousOffset;
    }
};

std::array<SafetyHookInline, 8> HudDrawHooks;
template<size_t Index, bool Left>
void __cdecl DrawHudElement()
{
    HudDrawScope scope(Left ? -fWidescreenHudOffset : fWidescreenHudOffset);
    HudDrawHooks[Index].unsafe_ccall();
}

struct ScriptMenu
{
    uint8_t unused[0x40];
    uint8_t type;
    uint8_t unused2[0x3B5];
    uint8_t rows, columns;
    bool interactive[4];
    float columnWidth[4];
    CVector2D position;
    bool background;
    int8_t selectedRow, acceptedRow;
};
static_assert(sizeof(ScriptMenu) == 0x418);
static_assert(offsetof(ScriptMenu, position) == 0x40C);
GameRef<std::array<ScriptMenu*, 2>> ScriptMenus;
GameRef<std::array<bool, 2>> ScriptMenusInUse;
std::array<SafetyHookInline, 2> ScriptMenuDrawHooks;
template<size_t Index>
void __cdecl DisplayScriptMenu(uint8_t id, uint8_t bright)
{
    auto panel = id < 2 ? (*ScriptMenus)[id] : nullptr;
    float offset = -fWidescreenHudOffset43;
    if (panel && !FrontendMenuManager->m_bDrawingMap)
    {
        // Stock shop/gym panels start at 29/31; result tables remain centered.
        const float x = panel->position.x / SCREEN_SCALE_X(1.0f);
        if (x < 160.0f) offset = GetScriptLeftOffset();
        else if (x > 480.0f) offset = -2.0f * fWidescreenHudOffset43 + fWidescreenSCMOffset;
    }
    else if (FrontendMenuManager->m_bDrawingMap)
        offset = 0.0f;
    CFont::DrawFonts();
    const float originalX = panel ? panel->position.x : 0.0f;
    if (panel) panel->position.x = originalX + offset;
    ScriptMenuDrawHooks[Index].unsafe_ccall(id, bright);
    CFont::DrawFonts();
    if (panel) panel->position.x = originalX;
}

SafetyHookInline shPrintRadioStationList = {};
void __fastcall PrintRadioStationList(void* MenuManager, void* edx)
{
    HudDrawScope scope(-fWidescreenHudOffset43);
    shPrintRadioStationList.unsafe_fastcall(MenuManager, edx);
}

injector::hook_back<void(__cdecl*)(CRect*, uint8_t*)> hbDrawRect;
void __cdecl DrawRect(CRect* rect, uint8_t* rgbaColor)
{
    CRect copy = *rect;
    hbDrawRect.fun(&copy, rgbaColor);
}

SafetyHookInline shPrintStats = {};
void __fastcall PrintStats(CMenuManager* menu, void* edx)
{
    HudDrawScope scope(-fWidescreenHudOffset43);
    shPrintStats.unsafe_fastcall(menu, edx);
}

SafetyHookInline shPrintBriefs = {};
void __fastcall PrintBriefs(CMenuManager* menu, void* edx)
{
    HudDrawScope scope(-fWidescreenHudOffset43);
    shPrintBriefs.unsafe_fastcall(menu, edx);
}

class Hud
{
public:
    Hud()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");

            auto bScalingMode = iniReader.ReadInteger("MAIN", "ScalingMode", 1) != 0;
            if (!bScalingMode)
                return;

            auto pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 ? A0 ? ? ? ? 84 C0 75 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
            CFont::DrawFonts = (decltype(CFont::DrawFonts))injector::GetBranchDestination(pattern.get_first(0)).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? A0 ? ? ? ? 84 C0 75 ? E8");
            CHud::shDrawScriptText = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CHud::DrawScriptText);

            pattern = hook::pattern("E8 ? ? ? ? EB ? 8B CD E8 ? ? ? ? EB");
            shPrintRadioStationList = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintRadioStationList);

            pattern = hook::pattern("E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? 8A 85");
            shPrintStats = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintStats);

            pattern = hook::pattern("E8 ? ? ? ? EB ? 8B CD E8 ? ? ? ? 84 DB");
            shPrintBriefs = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintBriefs);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8B 7C 24 ? 85 FF 0F 84 ? ? ? ? ? ? ? 0F 84");
            hbDrawRect.fun = injector::MakeCALL(pattern.get_first(), DrawRect, true).get();

            ScriptTexts.SetAddress(reinterpret_cast<std::array<ScriptText, 96>*>(0xA913E8));
            ScriptRectangles.SetAddress(reinterpret_cast<std::array<ScriptRectangle, 128>*>(0xA92D68));
            ScriptTextCount.SetAddress(reinterpret_cast<uint16_t*>(0xA44B68));
            ScriptRectangleCount.SetAddress(reinterpret_cast<uint16_t*>(0xA44B5C));
            // Read the live operand: Frontend may replace this scale at startup.
            HelpTextScaleX.SetAddress(0x58BFFC + 2);
            ScriptMenus.SetAddress(0xBA82D8);
            ScriptMenusInUse.SetAddress(0xBA82E0);

            // Menu opcodes store pixel coordinates. Keep already-open panels at
            // their original script positions when switching resolution.
            static int menuScreenHeight = RsGlobal->height;
            onResChange() += [](int Width, int Height)
            {
                if (menuScreenHeight > 0 && menuScreenHeight != Height)
                {
                    const float scale = static_cast<float>(Height) / menuScreenHeight;
                    for (size_t i = 0; i < ScriptMenus->size(); ++i)
                    {
                        if (!(*ScriptMenusInUse)[i] || !(*ScriptMenus)[i]) continue;
                        auto& menu = *(*ScriptMenus)[i];
                        menu.position.x *= scale;
                        menu.position.y *= scale;
                        const size_t columns = menu.type == 1 ? 1 : std::min<size_t>(menu.columns, 4);
                        for (size_t column = 0; column < columns; ++column) menu.columnWidth[column] *= scale;
                    }
                }
                menuScreenHeight = Height;
            };

            // Process inlines ProcessOneCommand. Hook the native handlers:
            // CLEO replaces every dispatch-table entry with a shared wrapper,
            // so the live table cannot be used to find these function entries.
            ScriptCommandHooks[8] = safetyhook::create_inline(0x481300, ProcessScriptCommand<8>);
            ScriptCommandHooks[9] = safetyhook::create_inline(0x483BD0, ProcessScriptCommand<9>);
            ScriptCommandHooks[11] = safetyhook::create_inline(0x48A320, ProcessScriptCommand<11>);
            ScriptCommandHooks[13] = safetyhook::create_inline(0x48CDD0, ProcessScriptCommand<13>);
            ScriptCommandHooks[18] = safetyhook::create_inline(0x46D050, ProcessScriptCommand<18>);
            ScriptCommandHooks[20] = safetyhook::create_inline(0x472310, ProcessScriptCommand<20>);
            ScriptCommandHooks[21] = safetyhook::create_inline(0x470A90, ProcessScriptCommand<21>);
            ScriptCommandHooks[22] = safetyhook::create_inline(0x474900, ProcessScriptCommand<22>);
            ScriptCommandHooks[23] = safetyhook::create_inline(0x4762D0, ProcessScriptCommand<23>);
            bScriptShadowToOutline = iniReader.ReadInteger("MISC", "ReplaceTextShadowWithOutline", 1) != 0;
            GetStringWidth = reinterpret_cast<decltype(GetStringWidth)>(0x71A0E0);
            hbGetScriptLetterSize.fun = injector::MakeCALL(0x71A1DF, GetScriptLetterSize, true).get();
            ScriptTextDictionary = injector::ReadMemory<void*>(0x58C1AE + 1, true);
            GetScriptString = (decltype(GetScriptString))injector::GetBranchDestination(0x58C1B3).as_int();
            InsertScriptNumbers = (decltype(InsertScriptNumbers))injector::GetBranchDestination(0x58C1CE).as_int();

            // Scope each element independently, including its buffered text.
            pattern = hook::pattern("81 EC A0 01 00 00 0F B6 05 ? ? ? ? 69 C0 90 01 00 00");
            HudDrawHooks[0] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<0, false>);
            pattern = hook::pattern("A1 ? ? ? ? 83 EC 34 83 F8 01 0F 84 ? ? ? ? 83 F8 02");
            HudDrawHooks[1] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<1, true>);
            pattern = hook::pattern("A0 ? ? ? ? 83 EC 28 3C 01 0F 84 ? ? ? ? A0");
            HudDrawHooks[2] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<2, true>);
            pattern = hook::pattern("A0 ? ? ? ? 83 EC 18 84 C0 53 55 56 57 0F 84");
            HudDrawHooks[3] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<3, true>);
            pattern = hook::pattern("51 8B 15 ? ? ? ? 57 33 FF 3B D7 0F 84 ? ? ? ? A1");
            HudDrawHooks[4] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<4, false>);
            pattern = hook::pattern("51 8B 15 ? ? ? ? 57 33 FF 3B D7 75 1B 89 3D");
            HudDrawHooks[5] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<5, false>);
            pattern = hook::pattern("A0 ? ? ? ? 81 EC F0 00 00 00 84 C0 74 0D A0");
            HudDrawHooks[6] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<6, false>);
            pattern = hook::pattern("83 EC 1C DB 05 ? ? ? ? 68 FF 00 00 00 68 FF 00 00 00");
            HudDrawHooks[7] = safetyhook::create_inline(pattern.get_first(), DrawHudElement<7, true>);

            pattern = hook::pattern("81 EC D8 01 00 00 DB 05 ? ? ? ? C6 44 24 14 00");
            ScriptMenuDrawHooks[0] = safetyhook::create_inline(pattern.get_first(), DisplayScriptMenu<0>);
            pattern = hook::pattern("83 EC 44 8A 44 24 4C 84 C0 C6 44 24 00 00 75 05");
            ScriptMenuDrawHooks[1] = safetyhook::create_inline(pattern.get_first(), DisplayScriptMenu<1>);
        };
    }
} Hud;