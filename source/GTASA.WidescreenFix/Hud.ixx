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

CRunningScript*** ppActiveScripts = nullptr;

bool IsScriptActiveByName(const char* scriptName)
{
    if (!scriptName || !ppActiveScripts || !*ppActiveScripts)
        return false;

    auto pActiveScripts = *ppActiveScripts;

    for (CRunningScript* s = *pActiveScripts; s; s = s->m_pNext)
    {
        const size_t len = std::find(s->m_szName, s->m_szName + sizeof(s->m_szName), '\0') - s->m_szName;
        const std::string_view name(s->m_szName, len);

        if (name == scriptName)
            return true;
    }

    return false;
}

static const std::vector<std::string_view> kPillarBarScripts = {
    "none",
    "bandit",
    "dual",
    "vidpok",
    "bskool",
    "boat",
    "grav",
    "shtr",
    "parach",
    "dskool",
};

static bool IsAnyScriptActive(const std::vector<std::string_view>& names)
{
    for (const auto name : names)
    {
        if (IsScriptActiveByName(name.data()))
            return true;
    }
    return false;
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
        g_needsToMoveHudLeft = true;
        g_needsToMoveHudRight = false;
        shDrawScriptText.unsafe_ccall(priority);
        CFont::DrawFonts();
        g_needsToMoveHudRight = false;
        g_needsToMoveHudLeft = false;
    }
}

SafetyHookInline shPrintRadioStationList = {};
void __fastcall PrintRadioStationList(void* MenuManager, void* edx)
{
    CFont::DrawFonts();
    g_needsToMoveHudLeft = true;
    g_needsToMoveHudRight = false;
    shPrintRadioStationList.unsafe_fastcall(MenuManager, edx);
    CFont::DrawFonts();
    g_needsToMoveHudRight = false;
    g_needsToMoveHudLeft = false;
}

injector::hook_back<void(__cdecl*)(uint8_t)> hbCMenuSystem__Process;
void __cdecl CMenuSystem__Process(uint8_t panelId)
{
    CFont::DrawFonts();
    g_needsToMoveHudLeft = true;
    g_needsToMoveHudRight = false;
    hbCMenuSystem__Process.fun(panelId);
    CFont::DrawFonts();
    g_needsToMoveHudRight = false;
    g_needsToMoveHudLeft = false;
}

injector::hook_back<void(__cdecl*)(CRect*, uint8_t*)> hbDrawRect;
void __cdecl DrawRect(CRect* rect, uint8_t* rgbaColor)
{
    CRect copy = *rect;
    hbDrawRect.fun(&copy, rgbaColor);
}

SafetyHookInline shPrintMap = {};
void __fastcall PrintMap(CMenuManager* menu, void* edx)
{
    CFont::DrawFonts();
    g_needsToMoveHudLeft = true;
    g_needsToMoveHudRight = false;
    shPrintMap.unsafe_fastcall(menu, edx);
    DrawPillarBars43();
    CFont::DrawFonts();
    g_needsToMoveHudRight = false;
    g_needsToMoveHudLeft = false;
}

SafetyHookInline shAdditionalOptionInput = {};
void __fastcall AdditionalOptionInput(CMenuManager* menu, void* edx, uint8_t* a2, uint8_t* a3)
{
    auto m_nCurrentMenuPage = *(int8_t*)((uintptr_t)menu + 0x15D);

    if (m_nCurrentMenuPage == SCREEN_MAP)
        menu->m_nMousePosX -= static_cast<int32_t>(std::abs(fWidescreenHudOffset43));
    shAdditionalOptionInput.unsafe_fastcall(menu, edx, a2, a3);
    if (m_nCurrentMenuPage == SCREEN_MAP)
        menu->m_nMousePosX += static_cast<int32_t>(std::abs(fWidescreenHudOffset43));
}

SafetyHookInline shPrintStats = {};
void __fastcall PrintStats(CMenuManager* menu, void* edx)
{
    CFont::DrawFonts();
    g_needsToMoveHudLeft = true;
    g_needsToMoveHudRight = false;
    shPrintStats.unsafe_fastcall(menu, edx);
    CFont::DrawFonts();
    g_needsToMoveHudRight = false;
    g_needsToMoveHudLeft = false;
}

SafetyHookInline shPrintBriefs = {};
void __fastcall PrintBriefs(CMenuManager* menu, void* edx)
{
    CFont::DrawFonts();
    g_needsToMoveHudLeft = true;
    g_needsToMoveHudRight = false;
    shPrintBriefs.unsafe_fastcall(menu, edx);
    CFont::DrawFonts();
    g_needsToMoveHudRight = false;
    g_needsToMoveHudLeft = false;
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

            auto pattern = hook::pattern("A1 ? ? ? ? 83 C4 ? 80 CB");
            ppActiveScripts = pattern.get_first<CRunningScript**>(1);

            pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 ? A0 ? ? ? ? 84 C0 75 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
            CFont::DrawFonts = (decltype(CFont::DrawFonts))injector::GetBranchDestination(pattern.get_first(0)).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? A0 ? ? ? ? 84 C0 75 ? E8");
            CHud::shDrawScriptText = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CHud::DrawScriptText);

            pattern = hook::pattern("E8 ? ? ? ? EB ? 8B CD E8 ? ? ? ? EB");
            shPrintRadioStationList = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintRadioStationList);

            pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 ? 8B 4C 24 ? 8B 54 24 ? 89 46 ? 89 4E ? 89 56");
            shPrintMap = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintMap);

            pattern = hook::pattern("E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? 8A 85");
            shPrintStats = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintStats);

            pattern = hook::pattern("E8 ? ? ? ? EB ? 8B CD E8 ? ? ? ? 84 DB");
            shPrintBriefs = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), PrintBriefs);

            pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 84 C0 74 ? C6 44 24");
            shAdditionalOptionInput = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AdditionalOptionInput);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 6A ? E8 ? ? ? ? 83 C4 ? A0");
            hbCMenuSystem__Process.fun = injector::MakeCALL(pattern.get_first(), CMenuSystem__Process, true).get();

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8B 7C 24 ? 85 FF 0F 84 ? ? ? ? ? ? ? 0F 84");
            hbDrawRect.fun = injector::MakeCALL(pattern.get_first(), DrawRect, true).get();

            //pattern = hook::pattern("E9 ? ? ? ? ? ? ? 8B 46 ? 68");
            //static auto DrawScriptSpritesAndRectanglesCase1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //{
            //    CFont::DrawFonts();
            //});

            //pattern = hook::pattern("E9 ? ? ? ? ? ? ? 8B 56 ? ? ? ? ? ? ? ? 8D 46");
            //static auto DrawScriptSpritesAndRectanglesCase2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //{
            //    CFont::DrawFonts();
            //});

            //pattern = hook::pattern("E9 ? ? ? ? ? ? ? 8B 56 ? 8D 46");
            //static auto DrawScriptSpritesAndRectanglesCase3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //{
            //    if (IsAnyScriptActive(kPillarBarScripts))
            //        DrawPillarBars43();
            //});
            //
            //pattern = hook::pattern("E9 ? ? ? ? ? ? ? 8D 56");
            //static auto DrawScriptSpritesAndRectanglesCase4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            //{
            //    if (IsAnyScriptActive(kPillarBarScripts))
            //        DrawPillarBars43();
            //});
        };
    }
} Hud;