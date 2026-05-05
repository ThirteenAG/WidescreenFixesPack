module;

#include <stdafx.h>
#include <ranges>
#include "common.h"
#include "callargs.h"
#include <Zydis.h>

export module Frontend;

import Skeleton;
import Sprite2d;
import Draw;
import Menu;
import Timer;

auto INV_SCREEN_WIDTH = [](float fAspectRatio) { return (1.0f / 640.0f) / (fAspectRatio / (4.0f / 3.0f)); };

#pragma pack(push, 1)
export struct CFontDetails
{
    CRGBA color;
    float scaleX;
    float scaleY;
    float slant;
    float slantRefX;
    float slantRefY;
    uint8_t justify;
    uint8_t centre;
    uint8_t rightJustify;
    uint8_t background;
    uint8_t backgroundOnlyText;
    uint8_t proportional;
    uint8_t bIsShadow;
    uint8_t bFlash;
    uint8_t bBold;
    uint8_t pad41[3];
    float alphaFade;
    CRGBA backgroundColor;
    float wrapX;
    float centreSize;
    float rightJustifyWrap;
    uint16_t style;
    uint8_t bFontHalfTexture;
    uint8_t pad3B_3F[5];
    int16_t dropShadowPosition;
    CRGBA dropColor;
    uint8_t bFlashState;
    uint8_t pad67;
    int nFlashTimer;
    uint8_t anonymous_23;
    uint8_t pad6D_6F[3];
    uint32_t anonymous_25;
};
#pragma pack(pop)

export struct CFontRenderState
{
    uint32_t anonymous_0;
    float fTextPosX;
    float fTextPosY;
    float scaleX;
    float scaleY;
    CRGBA color;
    float fExtraSpace;
    float slant;
    float slantRefX;
    float slantRefY;
    uint8_t bIsShadow;
    uint8_t bFontHalfTexture;
    uint8_t proportional;
    uint8_t anonymous_14;
    int16_t style;
};

export union tFontRenderStatePointer
{
    CFontRenderState* pRenderState;
    wchar_t* pStr;

    void Align()
    {
        if ((uintptr_t)pStr % 4)
            pStr++;
    }
};

export GameRef<tFontRenderStatePointer> FontRenderStatePointer([]() -> tFontRenderStatePointer*
{
    auto pattern = hook::pattern("B8 ? ? ? ? 29 F2 81 FA ? ? ? ? 83 DA ? ? ? 83 C2");
    if (!pattern.empty())
        return *pattern.get_first<tFontRenderStatePointer*>(1);
    return nullptr;
});

export constexpr size_t FontRenderStateBufSize = 1024;
export uint8_t* FontRenderStateBuf = nullptr;

export int ReplaceTextShadowWithOutline = 0;

export namespace CFont
{
    void (__cdecl* DrawFonts)() = nullptr;
    void (__cdecl* SetColor)(CRGBA* color) = nullptr;
    wchar_t* (__cdecl* ParseToken)(wchar_t* s) = nullptr;

    GameRef<CFontRenderState> RenderState([]() -> CFontRenderState*
    {
        auto pattern = hook::pattern("A3 ? ? ? ? A0 ? ? ? ? D9 1D");
        if (!pattern.empty())
            return *pattern.get_first<CFontRenderState*>(1);
        return nullptr;
    });

    GameRef<CFontDetails> Details([]() -> CFontDetails*
    {
        auto pattern = hook::pattern("80 3D ? ? ? ? ? 75 ? 80 3D ? ? ? ? ? 75 ? 80 3D ? ? ? ? ? 0F 84");
        if (!pattern.empty())
            return *pattern.get_first<CFontDetails*>(2);
        return nullptr;
    });

    enum
    {
        FONT_BANK,
        FONT_STANDARD,
        FONT_HEADING,

        MAX_FONTS = FONT_HEADING
    };

    SafetyHookInline shPrintOutlinedString = {};

    void __cdecl PrintOutlinedString(float x, float y, uint32_t, wchar_t* start, wchar_t* end, float spwidth)
    {
        if (RenderState->style != Details->style)
        {
            CFont::DrawFonts();
            RenderState->style = Details->style;
        }

        float dropShadowPosition = Details->dropShadowPosition;
        if (dropShadowPosition != 0.0f && (Details->style == FONT_BANK || Details->style == FONT_STANDARD))
        {
            CRGBA color = Details->color;
            uint8_t originalIsShadow = Details->bIsShadow;

            // Disable original shadow recursion for all custom passes
            Details->dropShadowPosition = 0;

            if (ReplaceTextShadowWithOutline)
            {
                Details->color = Details->dropColor;
                Details->bIsShadow = true;

                struct Offset { float x, y; };
                static constexpr Offset offsets[] = {
                    { 1.f,  1.f }, { 1.f, -1.f },
                    { -1.f, 1.f }, { -1.f, -1.f },
                    { 2.f,  0.f }, { -2.f, 0.f },
                    { 0.f,  2.f }, { 0.f, -2.f }
                };

                const int offsetCount = (ReplaceTextShadowWithOutline > 1) ? 8 : 4;

                // Compensate alpha so N blended passes ~= original single-pass alpha
                const float targetAlpha = static_cast<float>(color.a) / 255.0f;
                const float perPassAlphaF = 1.0f - std::pow(1.0f - targetAlpha, 1.0f / static_cast<float>(offsetCount));
                const uint8_t perPassAlpha = static_cast<uint8_t>(std::clamp(perPassAlphaF * 255.0f, 0.0f, 255.0f));

                for (int i = 0; i < offsetCount; ++i)
                {
                    CRGBA outlineColor = Details->dropColor;
                    outlineColor.a = perPassAlpha; // key fix
                    Details->color = outlineColor;

                    const float outlineStrength = 0.25f;
                    const float ox = SCREEN_SCALE_X(dropShadowPosition * offsets[i].x * outlineStrength);
                    const float oy = SCREEN_SCALE_Y(dropShadowPosition * offsets[i].y * outlineStrength);

                    if (Details->slant != 0.0f)
                    {
                        Details->slantRefX += ox;
                        Details->slantRefY += oy;
                        PrintOutlinedString(x + ox, y + oy, Details->anonymous_25, start, end, spwidth);
                        Details->slantRefX -= ox;
                        Details->slantRefY -= oy;
                    }
                    else
                    {
                        PrintOutlinedString(x + ox, y + oy, Details->anonymous_25, start, end, spwidth);
                    }
                }
            }
            else
            {
                Details->color = Details->dropColor;
                Details->bIsShadow = true;

                if (Details->slant != 0.0f)
                {
                    Details->slantRefX += SCREEN_SCALE_X(dropShadowPosition);
                    Details->slantRefY += SCREEN_SCALE_Y(dropShadowPosition);
                    PrintOutlinedString(SCREEN_SCALE_X(dropShadowPosition) + x, SCREEN_SCALE_Y(dropShadowPosition) + y, Details->anonymous_25, start, end, spwidth);
                    Details->slantRefX -= SCREEN_SCALE_X(dropShadowPosition);
                    Details->slantRefY -= SCREEN_SCALE_Y(dropShadowPosition);
                }
                else
                {
                    PrintOutlinedString(SCREEN_SCALE_X(dropShadowPosition) + x, SCREEN_SCALE_Y(dropShadowPosition) + y, Details->anonymous_25, start, end, spwidth);
                }
            }

            Details->color = color;
            Details->dropShadowPosition = static_cast<int16_t>(dropShadowPosition);
            Details->bIsShadow = originalIsShadow;
        }

        if (FontRenderStatePointer->pStr >= (wchar_t*)(FontRenderStateBuf + FontRenderStateBufSize) - (end - start + 26))
            CFont::DrawFonts();

        CFontRenderState* pRenderState = FontRenderStatePointer->pRenderState;
        pRenderState->fTextPosX = x;
        pRenderState->fTextPosY = y;
        pRenderState->scaleX = Details->scaleX;
        pRenderState->scaleY = Details->scaleY;
        pRenderState->color = Details->color;
        pRenderState->fExtraSpace = spwidth;
        pRenderState->slant = Details->slant;
        pRenderState->slantRefX = Details->slantRefX;
        pRenderState->slantRefY = Details->slantRefY;
        pRenderState->bFontHalfTexture = Details->bFontHalfTexture;
        pRenderState->proportional = Details->proportional;
        pRenderState->style = Details->style;
        pRenderState->bIsShadow = Details->bIsShadow;
        FontRenderStatePointer->pRenderState++;

        for (auto s = start; s < end;)
        {
            if (*s == '~')
            {
                for (wchar_t* i = ParseToken(s); s != i; FontRenderStatePointer->pStr++)
                {
                    *FontRenderStatePointer->pStr = *(s++);
                }
                if (Details->bFlash)
                {
                    if (CTimer::m_snTimeInMilliseconds - Details->nFlashTimer > 300)
                    {
                        Details->bFlashState = !Details->bFlashState;
                        Details->nFlashTimer = CTimer::m_snTimeInMilliseconds;
                    }
                    Details->color.a = Details->bFlashState ? 0 : 255;
                }
            }
            else
            {
                *(FontRenderStatePointer->pStr++) = *(s++);
            }
        }

        *(FontRenderStatePointer->pStr++) = '\0';
        FontRenderStatePointer->Align();
    }
}

export enum FrontendClass
{
    eCDarkel,
    eCGarages,
    eCMenuManager,
    eLoadingScreen,
    eCRadar,
    eCHud,
    eCParticle,
    eCSpecialFX,
    ecMusicManager,
    eCSceneEdit,
    eConvertingTexturesScreen,
    eCReplay,

    FrontendClassCount
};

export std::array<ProtectedGameRef<float>, FrontendClassCount> ResXInvRefs;
export std::array<ProtectedGameRef<float>, FrontendClassCount> ResYInvRefs;

void StretchX(SafetyHookContext& ctx)
{
    float f = f = (CDraw::GetAspectRatio() / (4.0f / 3.0f));
    _asm {fdiv dword ptr[f]}
}

class Frontend
{
public:
    Frontend()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            static float fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.0f);
            static float fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.0f);
            static float fRadarWidth = iniReader.ReadFloat("MAIN", "RadarWidth", 94.0f);
            static float fRadarHeight = iniReader.ReadFloat("MAIN", "RadarHeight", 76.0f);
            static float fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 0.0f);

            auto pattern = hook::pattern("E8 ? ? ? ? 6A ? E8 ? ? ? ? 59 8D 8C 24 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? ? ? ? ? ? ? 59 50 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 50 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 3D");
            CFont::DrawFonts = (decltype(CFont::DrawFonts))injector::GetBranchDestination(pattern.get_first(0)).as_int();

            pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 59 3D ? ? ? ? ? ? ? ? 75 ? ? ? ? ? ? ? EB ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? A1");
            CFont::SetColor = (decltype(CFont::SetColor))injector::GetBranchDestination(pattern.get_first(0)).as_int();

            pattern = hook::pattern("E8 ? ? ? ? 39 C6 59 74");
            CFont::ParseToken = (decltype(CFont::ParseToken))injector::GetBranchDestination(pattern.get_first(0)).as_int();

            pattern = hook::pattern("A1 ? ? ? ? 8A 15 ? ? ? ? A3");
            FontRenderStateBuf = *pattern.get_first<uint8_t*>(1);

            pattern = hook::pattern("E8 ? ? ? ? 83 C4 18 80 3D ? ? ? ? ? D9 EE 89 74 24");
            CFont::shPrintOutlinedString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), CFont::PrintOutlinedString);

            constexpr auto INV_DEFAULT_SCREEN_HEIGHT = 1.0f / (float)DEFAULT_SCREEN_HEIGHT;
            constexpr auto INV_DEFAULT_SCREEN_HEIGHT_MENU = 1.0f / 448.0f;

            auto SetResX = [](hook::pattern pattern, FrontendClass id)
            {
                if (pattern.empty()) return;
                ResXInvRefs[id].SetAddress(*pattern.get_first<float*>(2));
            };

            auto SetResY = [](hook::pattern pattern, FrontendClass id)
            {
                if (pattern.empty()) return;
                ResYInvRefs[id].SetAddress(*pattern.get_first<float*>(2));
            };

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 81 FB B8 0B 00 00");
            SetResX(pattern, FrontendClass::eCDarkel);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 81 FB B8 0B 00 00");
            SetResY(pattern, FrontendClass::eCDarkel);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1");
            SetResX(pattern, FrontendClass::eCGarages);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1");
            SetResY(pattern, FrontendClass::eCGarages);

            //flt_68AEDC
            //CCamera::DrawBordersForWideScreen
            //CCamera::GetScreenRect

            pattern = hook::pattern("D8 0D ? ? ? ? 89 74 24 ? DA 4C 24 ? DD DA 89 74 24");
            SetResX(pattern, FrontendClass::eCMenuManager);
            pattern = hook::pattern("D8 0D ? ? ? ? DA 4C 24 ? DD D9 FF 34 24");
            SetResY(pattern, FrontendClass::eCMenuManager);

            pattern = hook::pattern("D8 0D ? ? ? ? DD DB D9 05 ? ? ? ? D8 CB DD DA D9 C1 D8 25");
            SetResX(pattern, FrontendClass::eLoadingScreen);
            pattern = hook::pattern("D8 0D ? ? ? ? DD DC D9 05 ? ? ? ? D8 CC DA 6C 24 ? 89 4C 24");
            SetResY(pattern, FrontendClass::eLoadingScreen);

            pattern = hook::pattern("D8 0D ? ? ? ? DE C9 D9 5C 24 ? D9 05 ? ? ? ? D8 C9");
            SetResX(pattern, FrontendClass::eCRadar);
            pattern = hook::pattern("D8 0D ? ? ? ? DE C9 EB 75");
            SetResY(pattern, FrontendClass::eCRadar);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 8D ? ? ? ? D9 1C 24 E8 ? ? ? ? 8B 95");
            SetResX(pattern, FrontendClass::eCHud);
            pattern = hook::pattern("D8 0D ? ? ? ? D9 05 ? ? ? ? D8 A5 ? ? ? ? DE C9 DA 6C 24 ? D9 1C 24 A1");
            SetResY(pattern, FrontendClass::eCHud);

            pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 43 ? D8 8D ? ? ? ? D8 CA D8 03");
            SetResX(pattern, FrontendClass::eCParticle);
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 43 ? D8 8D ? ? ? ? D8 C9 D8 6B ? D9 7C 24 ? 8B 44 24 ? 80 4C 24 ? ? D9 6C 24 ? 89 44 24 ? DB 5C 24 ? 8B 44 24 ? D9 6C 24 ? 89 44 24 ? D9 43 ? D8 8D ? ? ? ? D8 CA D8 03");
            SetResY(pattern, FrontendClass::eCParticle);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC 89 44 24 ? 50 DB 44 24 ? D9 1C 24 E8 ? ? ? ? 59 E8 ? ? ? ? E8 ? ? ? ? 8D 4C 24 ? 68 C8 00 00 00 6A 00");
            SetResX(pattern, FrontendClass::eCSpecialFX);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC 89 44 24 ? 50 DB 44 24 ? D9 1C 24 E8 ? ? ? ? 59 E8 ? ? ? ? E8 ? ? ? ? 8D 4C 24 ? 68 C8 00 00 00 6A 00");
            SetResY(pattern, FrontendClass::eCSpecialFX);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A ? E8 ? ? ? ? 59");
            SetResX(pattern, FrontendClass::ecMusicManager);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A");
            SetResY(pattern, FrontendClass::ecMusicManager);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? FF 35 ? ? ? ? E8 ? ? ? ? 59 E8 ? ? ? ? 6A 01");
            SetResX(pattern, FrontendClass::eCSceneEdit);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? FF 35 ? ? ? ? E8 ? ? ? ? 59 E8 ? ? ? ? 6A 01");
            SetResY(pattern, FrontendClass::eCSceneEdit);

            //flt_6D72CC

            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 DD DA D9 C1");
            SetResX(pattern, FrontendClass::eConvertingTexturesScreen);
            pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 05 ? ? ? ? D8 CA D9 5C 24 ? D9 05");
            SetResY(pattern, FrontendClass::eConvertingTexturesScreen);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC 89 04 24");
            SetResX(pattern, FrontendClass::eCReplay);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C0 EC 89 04 24");
            SetResY(pattern, FrontendClass::eCReplay);

            // Brief additional fix
            static float fBriefWrapMult = 0.875f;
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 50 D9 14 24 DD D8");
            injector::WriteMemory(pattern.get_first(2), &fBriefWrapMult, true);

            static float fBriefXMult = 0.15625f;
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 68 ? ? ? ? FF 74 24 ? 50 D9 14 24 DD D8 E8 ? ? ? ? 0F BF C0");
            injector::WriteMemory(pattern.get_first(2), &fBriefXMult, true);

            pattern = hook::pattern("D8 0D ? ? ? ? DD DA A1 ? ? ? ? 3D C0 01 00 00 75 ? D9 44 24");
            injector::WriteMemory(pattern.get_first(2), &fBriefXMult, true);

            // Stats additional fix
            static float fStatsXMult = 0.171875f;
            static float fStatsWrapMult = 0.82343751f;
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 68 ? ? ? ? FF 74 24 ? 50 D9 14 24 DD D8 E8 ? ? ? ? 83 C4 0C");
            injector::WriteMemory(pattern.count(2).get(0).get<float*>(2), &fStatsXMult, true);
            injector::WriteMemory(pattern.count(2).get(1).get<float*>(2), &fStatsWrapMult, true);

            // Map Legend additional fix
            static float fLegendLeft = 0.25f;
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 C0 D9 7C 24 ? 8B 44 24 ? 80 4C 24 ? ? D9 6C 24 ? 89 44 24 ? 31 FF");
            injector::WriteMemory(pattern.get_first(2), &fLegendLeft, true);

            static float fLegendRight = 0.171875f;
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 C0 D9 7C 24 ? 8B 44 24 ? 80 4C 24 ? ? D9 6C 24 ? 89 44 24 ? A1 ? ? ? ? DB 5C 24 ? D9 6C 24 ? 3D C0 01 00 00");
            injector::WriteMemory(pattern.get_first(2), &fLegendRight, true);

            onResChange() += [](int Width, int Height)
            {
                float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

                for (auto [index, resXInv] : std::views::enumerate(ResXInvRefs))
                {
                    resXInv = INV_SCREEN_WIDTH(fAspectRatio);
                }

                for (auto [index, resYInv] : std::views::enumerate(ResYInvRefs))
                {
                    if (index != FrontendClass::eCMenuManager)
                        resYInv = INV_DEFAULT_SCREEN_HEIGHT;
                    else
                        resYInv = INV_DEFAULT_SCREEN_HEIGHT_MENU;
                }

                float boxW = fmin(static_cast<float>(Width), static_cast<float>(Height) * (4.0f / 3.0f));
                float offset = (static_cast<float>(Width) - boxW) * 0.5f;
                fBriefXMult = (offset + boxW * (100.0f / 640.0f)) / static_cast<float>(Width);
                fBriefWrapMult = (offset + boxW * (560.0f / 640.0f)) / static_cast<float>(Width);

                fStatsXMult = (offset + boxW * (110.0f / 640.0f)) / static_cast<float>(Width);
                fStatsWrapMult = (offset + boxW * (530.0f / 640.0f)) / static_cast<float>(Width);

                fLegendLeft = (offset + boxW * (160.0f / 640.0f)) / static_cast<float>(Width);
                fLegendRight = (offset + boxW * (350.0f / 640.0f)) / static_cast<float>(Width);
            };

            std::vector<void*> StretchXRefs;

            auto ptr = &RsGlobal->maximumWidth;
            pattern = hook::pattern(pattern_str(0xA1, to_bytes(ptr))); //mov eax
            pattern.for_each_result([&](hook::pattern_match match)
            {
                StretchXRefs.push_back(match.get<void*>());
            });

            pattern = hook::pattern(pattern_str(0x8B, '?', to_bytes(ptr))); //mov
            pattern.for_each_result([&](hook::pattern_match match)
            {
                StretchXRefs.push_back(match.get<void*>());
            });

            // Store hooks to keep them alive
            static std::vector<SafetyHookMid> stretchHooks;

            for (auto address : StretchXRefs)
            {
                ZydisDecoder decoder;
                ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

                ZydisDecodedInstruction instruction;
                ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

                bool foundMov = false;
                bool foundCmp = false;
                bool foundFmul = false;
                bool foundFimul = false;  // NEW: separate flag
                ZydisRegister targetRegister = ZYDIS_REGISTER_NONE;
                void* fmulOperandAddress = nullptr;
                uint8_t* fmulInstructionAddress = nullptr;

                uint8_t* currentAddress = (uint8_t*)address;
                const int maxInstructions = 30;

                for (int instrCount = 0; instrCount < maxInstructions && !foundFmul; instrCount++)
                {
                    ZyanStatus status = ZydisDecoderDecodeFull(
                        &decoder,
                        currentAddress,
                        ZYDIS_MAX_INSTRUCTION_LENGTH,
                        &instruction,
                        operands
                    );

                    if (!ZYAN_SUCCESS(status))
                        break;

                    if (instrCount == 0)
                    {
                        if (instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
                            instruction.operand_count >= 2 &&
                            operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                            operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
                        {
                            foundMov = true;
                            targetRegister = operands[0].reg.value;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (foundMov && !foundCmp &&
                        instruction.mnemonic == ZYDIS_MNEMONIC_CMP &&
                        instruction.operand_count >= 2)
                    {
                        if (operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                            operands[0].reg.value == targetRegister &&
                            operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                            operands[1].imm.value.u == 0x280)
                        {
                            foundCmp = true;
                        }
                    }

                    if (foundMov && foundCmp &&
                        instruction.mnemonic == ZYDIS_MNEMONIC_FMUL &&
                        instruction.operand_count >= 1)
                    {
                        if (operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
                        {
                            foundFmul = true;
                            fmulInstructionAddress = currentAddress;

                            ZyanU64 operandAddr = 0;
                            ZydisCalcAbsoluteAddress(&instruction, &operands[0],
                                (ZyanU64)currentAddress, &operandAddr);
                            fmulOperandAddress = (void*)operandAddr;

                            // Check if followed by FIMUL
                            uint8_t* nextAddr = currentAddress + instruction.length;
                            ZydisDecodedInstruction nextInstr;
                            ZydisDecodedOperand nextOps[ZYDIS_MAX_OPERAND_COUNT];

                            for (int j = 0; j < 5; j++)
                            {
                                if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, nextAddr,
                                    ZYDIS_MAX_INSTRUCTION_LENGTH, &nextInstr, nextOps)))
                                {
                                    if (nextInstr.mnemonic == ZYDIS_MNEMONIC_FIMUL)
                                    {
                                        foundFimul = true;
                                        break;
                                    }
                                    nextAddr += nextInstr.length;
                                }
                                else
                                    break;
                            }
                        }
                        break;
                    }

                    currentAddress += instruction.length;
                }

                if (foundMov && foundCmp && foundFmul)
                {
                    const float menu_scaling_floats[] = {
                        0.0009375000373f,   // fMenuTextScale
                        0.00062499999f,     // fMenuControlsOptionsTextScale
                        0.00039062501f,     // fMenuControlsOptionsTextScale2
                        0.0046875002f,      // fMenuBarsScale
                        0.0014062499f,      // fMenuHeadersSkinsScale
                        0.2453125f,         // fvcLogoScale
                        0.001015625f,       // fMapLegendTextScale
                        0.000859375f,       // fMapLegendIconsTextScale
                        0.00046875002f,     // fMapBottomTextScale
                        0.00075000001f,     // fSaveLoadListTextScale
                        0.00065625005f,     // fLoadingGameTextScale
                        0.0546875f,         // fCursorScale
                        0.0703125f,         // fCursorShadowScale
                        0.00067187502f,     // fStatsText
                        0.00078125001f,     // fStatsHeader
                        0.09375f,           // fRadioIcons
                    };

                    auto IsMenuScalingFloat = [&menu_scaling_floats](float value) -> bool
                    {
                        for (float target : menu_scaling_floats)
                        {
                            if (std::abs(value - target) <= FLT_EPSILON)
                                return true;
                        }
                        return false;
                    };

                    if (IsMenuScalingFloat(*(float*)fmulOperandAddress))
                    {
                        stretchHooks.emplace_back(safetyhook::create_mid(fmulInstructionAddress, StretchX));
                    }
                }
            }

            //Menu fix
            static float fDefaultScale = 0.0015625f;

            auto DrawBackgroundStart = (uintptr_t)hook::pattern("D8 0D ? ? ? ? 89 44 24 ? DA 4C 24 ? DD DC 8B 15").get_first(0);
            auto DrawBackgroundEnd = (uintptr_t)hook::pattern("D8 0D ? ? ? ? 89 4C 24 ? DA 4C 24 ? DD DA D9 05 ? ? ? ? 81 FB C0 01 00 00 DD D9 74 ? D8 0D ? ? ? ? 89 5C 24 ? DA 4C 24 ? 8D 84 24").get_first(6);
            pattern = hook::pattern(DrawBackgroundStart, DrawBackgroundEnd, pattern_str(0xD8, 0x0D, to_bytes(*(void**)(DrawBackgroundStart + 2))));
            pattern.for_each_result([&](hook::pattern_match match)
            {
                injector::WriteMemory(match.get<void*>(2), &fDefaultScale, true);
            });

            auto CMenuManagerDrawStart = (uintptr_t)hook::pattern("D8 0D ? ? ? ? 89 BC 24 ? ? ? ? DA 8C 24 ? ? ? ? DD DD").get_first(0);
            auto CMenuManagerDrawEnd = (uintptr_t)hook::pattern("D8 0D ? ? ? ? 89 94 24 ? ? ? ? DA 8C 24 ? ? ? ? DD DB 81 FE C0 01 00 00").get_first(6);
            pattern = hook::pattern(CMenuManagerDrawStart, CMenuManagerDrawEnd, pattern_str(0xD8, 0x0D, to_bytes(*(void**)(CMenuManagerDrawStart + 2))));
            pattern.for_each_result([&](hook::pattern_match match)
            {
                injector::WriteMemory(match.get<void*>(2), &fDefaultScale, true);
            });

            //Map fix
            pattern = hook::pattern("D9 43 ? D9 05 ? ? ? ? D8 C9 D8 0D");
            static auto PrintMapHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                FrontendMenuManager->m_fMapCenterX = ((1.0f / INV_DEFAULT_SCREEN_HEIGHT_MENU) * CDraw::GetAspectRatio()) / 2.0f;
            });

            auto PrintMapStart = (uintptr_t)hook::pattern("D8 0D ? ? ? ? 89 54 24 ? DA 4C 24 ? DD DA 89 54 24").get_first(0);
            auto PrintMapEnd = (uintptr_t)hook::pattern("D8 0D ? ? ? ? 89 4C 24 ? DA 4C 24 ? DD DC D9 C2 D8 44 24").get_first(6);
            pattern = hook::pattern(PrintMapStart, PrintMapEnd, pattern_str(0xD8, 0x0D, to_bytes(*(void**)(PrintMapStart + 2))));
            pattern.for_each_result([&](hook::pattern_match match)
            {
                injector::WriteMemory(match.get<void*>(2), ResYInvRefs[FrontendClass::eCRadar].get_ptr(), true);
            });

            if (fHudWidthScale || fHudHeightScale)
            {
                fHudWidthScale = std::clamp(fHudWidthScale, 0.25f, 4.0f);
                fHudHeightScale = std::clamp(fHudHeightScale, 0.25f, 4.0f);

                static float fCustomWideScreenWidthScaleDown = 1.0f / 640.0f;
                static float fCustomWideScreenHeightScaleDown = 1.0f / 480.0f;

                onResChange() += [](int Width, int Height)
                {
                    float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                    fCustomWideScreenWidthScaleDown = INV_SCREEN_WIDTH(fAspectRatio) * fHudWidthScale;
                    fCustomWideScreenHeightScaleDown = INV_DEFAULT_SCREEN_HEIGHT * fHudHeightScale;
                };

                auto temp1 = *hook::pattern("D8 0D ? ? ? ? D8 8D ? ? ? ? D9 1C 24 E8 ? ? ? ? 8B 95").count(1).get(0).get<float*>(2);
                auto DrawHudHorScalePattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp1)));

                auto temp2 = *hook::pattern("D8 0D ? ? ? ? D8 8D ? ? ? ? D8 0D").count(1).get(0).get<float*>(2);
                auto DrawHudVerScalePattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp2)));

                for (size_t i = 0; i < DrawHudHorScalePattern.size(); i++)
                {
                    if (i > 3 && i != 20 && i != 66 && i != 67 && i != 15 && i != 16 && i != 17) //15 16 17 - crosshair
                    {
                        uint32_t* pCustomScaleHor = DrawHudHorScalePattern.get(i).get<uint32_t>(2);
                        injector::WriteMemory(pCustomScaleHor, &fCustomWideScreenWidthScaleDown, true);
                    }
                }

                for (size_t i = 0; i < DrawHudVerScalePattern.size(); i++)
                {
                    if (i > 1 && i != 18 && i != 57 && i != 58 && i != 13 && i != 14 && i != 15) //13 14 15 - crosshair
                    {
                        uint32_t* pCustomScaleVer = DrawHudVerScalePattern.get(i).get<uint32_t>(2);
                        injector::WriteMemory(pCustomScaleVer, &fCustomWideScreenHeightScaleDown, true);
                    }
                }

                auto temp3 = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 81 FB B8 0B 00 00").count(1).get(0).get<float*>(2);
                auto CDarkelDrawMessagesPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp3)));

                auto temp4 = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 81 FB B8 0B 00 00").count(1).get(0).get<uint32_t*>(2);
                auto CDarkelDrawMessagesPattern2 = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp4)));

                CDarkelDrawMessagesPattern.for_each_result([&](const hook::pattern_match& match)
                {
                    injector::WriteMemory(match.get<void*>(2), &fCustomWideScreenWidthScaleDown, true);
                });

                CDarkelDrawMessagesPattern2.for_each_result([&](const hook::pattern_match& match)
                {
                    injector::WriteMemory(match.get<void*>(2), &fCustomWideScreenHeightScaleDown, true);
                });
            }

            static std::vector<ProtectedGameRef<float>> RadarWidthRefs;
            static std::vector<ProtectedGameRef<float>> RadarHeightRefs;

            {
                auto pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 C2 D8 C9 D8 0D ? ? ? ? D9 C1 D8 0D ? ? ? ? D8 05 ? ? ? ? DE C1 D9 5C 24");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarWidthRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D8 0D ? ? ? ? D8 05 ? ? ? ? D8 05 ? ? ? ? D9 9C 24 ? ? ? ? D9 05 ? ? ? ? D8 CA D8 C1 D8 05 ? ? ? ? D9 9C 24 ? ? ? ? DE D9 DD D8 E8 ? ? ? ? 8D 8C 24");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarWidthRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 D9 14 24 FF 35 ? ? ? ? 56 DD D8 E8 ? ? ? ? 83 C4 14 85 C0 74 ? B8 01 00 00 00 E9 E6 00 00 00");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarWidthRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 DD DB D9 C1 D8 CB D8 0D ? ? ? ? D9 C3 D8 0D ? ? ? ? D9 05 ? ? ? ? D8 CB DA 2C 24 DE C1 DE E1 D9 5C 24");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarHeightRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D9 05 ? ? ? ? D8 CA D8 C1 D8 05 ? ? ? ? D9 9C 24 ? ? ? ? DE D9 DD D8 E8 ? ? ? ? 8D 8C 24");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarHeightRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 D9 05 ? ? ? ? D8 CA DA 6C 24 ? DD DA D8 C1 D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 D9 14 24 FF 35 ? ? ? ? 56 DD D8 E8 ? ? ? ? 83 C4 14 85 C0 74 ? B8 01 00 00 00 E9 E6 00 00 00");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarHeightRefs.push_back(std::move(ref));
                }
            }

            for (auto& radarWidthRef : RadarWidthRefs)
            {
                radarWidthRef = fRadarWidth * (((float)DEFAULT_SCREEN_WIDTH / (float)DEFAULT_SCREEN_HEIGHT) / (640.0f / 448.0f));
            }

            for (auto& radarHeightRef : RadarHeightRefs)
            {
                radarHeightRef = fRadarHeight;
            }

            if (fSubtitlesScale)
            {
                fSubtitlesScale = std::clamp(fSubtitlesScale, 0.25f, 4.0f);

                ProtectedGameRef<float> SubtitlesWidth;
                pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? A1");
                SubtitlesWidth.SetAddress(*pattern.get_first<float*>(2));

                ProtectedGameRef<float> SubtitlesWidth2;
                pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 8B 3D");
                SubtitlesWidth2.SetAddress(*pattern.get_first<float*>(2));

                ProtectedGameRef<float> SubtitlesHeight;
                pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? A1");
                SubtitlesHeight.SetAddress(*pattern.get_first<float*>(2));

                SubtitlesWidth *= fSubtitlesScale;
                SubtitlesWidth2 *= fSubtitlesScale;
                SubtitlesHeight *= fSubtitlesScale;
            }
        };
    }
} Frontend;