module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Sprite2d;

import Skeleton;
import Camera;
import Draw;
import Timer;
import Menu;

using RwIm2DVertex = void;

class CSprite2d
{
public:
    RwTexture* m_pTexture;
};

float s_bordersMult = 0.0f;
float s_barHeight = 0.0f;
float s_pillarWidth = 0.0f;
bool s_hasPillar = false;
bool s_hasLetterbox = false;
uint32_t s_lastActiveMs = 0;
uint32_t s_lastTickMs = 0;
uint32_t s_lastExitDrawMs = 0;
constexpr float BORDER_ANIM_SECS = 0.35f;

export bool g_isMouseCursor = false;
bool g_skipXCorrection = false;
bool g_isFullscreen = false;
bool g_hasTexture = false;
uint8_t g_alpha = 255;
CRect g_contentRect = {};
bool g_drawingBars = false;

static inline float CorrectX(float x)
{
    #if 0
    float arScale = DEFAULT_ASPECT_RATIO / SCREEN_ASPECT_RATIO;
    float offset = (SCREEN_WIDTH - SCREEN_WIDTH * arScale) / 2.0f;
    return offset + x * arScale;
    #endif

    return x;
}

static inline bool IsFullscreen(const CRect* r)
{
    return r->left <= 0.5f
        && r->top <= 0.5f
        && r->right >= SCREEN_WIDTH - 0.5f
        && r->bottom >= SCREEN_HEIGHT - 0.5f;
}

export bool g_wantsToMoveHudLeft = false;
export bool g_wantsToMoveHudRight = false;

static inline float GetHudOffset()
{
    if (g_wantsToMoveHudLeft)  return -fWidescreenHudOffset;
    if (g_wantsToMoveHudRight) return  fWidescreenHudOffset;
    return 0.0f;
}

static inline CRect* ScaleRect(CRect* r)
{
    float offset = GetHudOffset();
    if (g_skipXCorrection)
    {
        *r = CRect(r->left + offset, r->bottom, r->right + offset, r->top);
        return r;
    }

    *r = CRect(CorrectX(r->left) + offset, r->bottom, CorrectX(r->right) + offset, r->top);
    return r;
}

static CRect ComputeContentRect(CSprite2d* sprite2d, const CRect* rect)
{
    float w = DEFAULT_ASPECT_RATIO, h = 1.0f; // default 4:3

    RwRaster* pRaster = RwTextureGetRaster(sprite2d->m_pTexture);
    if (pRaster)
    {
        int32_t texW = RwRasterGetWidth(pRaster);
        int32_t texH = RwRasterGetHeight(pRaster);
        if (texW > 0 && texH > 0 && texW != texH && texW / texH != 2)
        {
            w = (float)texW;
            h = (float)texH;
        }
    }

    float aspect = w / h;
    float centerX = SCREEN_WIDTH / 2.0f;
    float halfW = (SCREEN_HEIGHT * aspect) / 2.0f;

    return CRect(centerX - halfW, rect->bottom, centerX + halfW, rect->top);
}

export SafetyHookInline shDrawRect1 = {};
static void DrawPillarBars(const CRect& content, float top, float bottom)
{
    g_drawingBars = true;

    uint8_t black[4] = { 0, 0, 0, g_alpha };
    CRect left (0.0f, top, content.left, bottom);
    CRect right(content.right, top, SCREEN_WIDTH, bottom);

    shDrawRect1.unsafe_ccall(&left, black);
    shDrawRect1.unsafe_ccall(&right, black);

    g_drawingBars = false;
}

void TickBorderAnim(uint32_t now, bool active)
{
    if (!s_lastTickMs) { s_lastTickMs = now; return; }
    float dt = std::min((now - s_lastTickMs) / 1000.0f, 0.1f);
    s_lastTickMs = now;
    if (active)
        s_bordersMult = std::min(s_bordersMult + dt / BORDER_ANIM_SECS, 1.0f);
    else
        s_bordersMult = std::max(s_bordersMult - dt / BORDER_ANIM_SECS, 0.0f);
}

void RenderBorderBars()
{
    g_drawingBars = true;
    float m = s_bordersMult;

    if (s_hasLetterbox)
    {
        float h = s_barHeight * m;
        CRect top(0.0f, h - SCREEN_SCALE_Y(8.0f), SCREEN_WIDTH, 0.0f);
        CRect bottom(0.0f, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - h - SCREEN_SCALE_Y(8.0f));
        CRGBA black(0, 0, 0, 255);
        shDrawRect1.unsafe_ccall(&top, &black);
        shDrawRect1.unsafe_ccall(&bottom, &black);
    }

    if (s_hasPillar)
    {
        float w = s_pillarWidth * m;
        CRect left(0.0f, 0.0f, w, SCREEN_HEIGHT);
        CRect right(SCREEN_WIDTH - w, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
        CRGBA black(0, 0, 0, 255);
        shDrawRect1.unsafe_ccall(&left, &black);
        shDrawRect1.unsafe_ccall(&right, &black);
    }

    g_drawingBars = false;
}

SafetyHookInline shDraw1 = {};
void __fastcall Draw1(CSprite2d* sprite2d, void* edx, CRect* rect, CRGBA* col)
{
    g_isFullscreen = IsFullscreen(rect);
    g_hasTexture = sprite2d->m_pTexture != nullptr;
    g_alpha = reinterpret_cast<uint8_t*>(col)[3];
    if (g_isFullscreen && g_hasTexture) g_contentRect = ComputeContentRect(sprite2d, rect);
    shDraw1.unsafe_fastcall(sprite2d, edx, rect, col);
    if (g_isFullscreen && g_hasTexture) DrawPillarBars(g_contentRect, rect->top, rect->bottom);
    g_isFullscreen = false; g_hasTexture = false;
}

SafetyHookInline shDraw2 = {};
void __fastcall Draw2(CSprite2d* sprite2d, void* edx, CRect* rect, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom)
{
    g_isFullscreen = IsFullscreen(rect);
    g_hasTexture = sprite2d->m_pTexture != nullptr;
    g_alpha = reinterpret_cast<uint8_t*>(colorLeft)[3];
    if (g_isFullscreen && g_hasTexture) g_contentRect = ComputeContentRect(sprite2d, rect);
    shDraw2.unsafe_fastcall(sprite2d, edx, rect, colorLeft, colorTop, colorRight, colorBottom);
    if (g_isFullscreen && g_hasTexture) DrawPillarBars(g_contentRect, rect->top, rect->bottom);
    g_isFullscreen = false; g_hasTexture = false;
}

SafetyHookInline shDraw3 = {};
void __fastcall Draw3(CSprite2d* sprite2d, void* edx, CRect* rect, CRGBA* col,
    float u0, float v0, float u1, float v1, float u3, float v3, float u2, float v2)
{
    g_isFullscreen = IsFullscreen(rect);
    g_hasTexture = sprite2d->m_pTexture != nullptr;
    g_alpha = reinterpret_cast<uint8_t*>(col)[3];
    if (g_isFullscreen && g_hasTexture) g_contentRect = ComputeContentRect(sprite2d, rect);
    shDraw3.unsafe_fastcall(sprite2d, edx, rect, col, u0, v0, u1, v1, u3, v3, u2, v2);
    if (g_isFullscreen && g_hasTexture) DrawPillarBars(g_contentRect, rect->top, rect->bottom);
    g_isFullscreen = false; g_hasTexture = false;
}

SafetyHookInline shDraw4 = {};
void __fastcall Draw4(CSprite2d* sprite2d, void* edx, float x, float y, float w, float h, CRGBA* color)
{
    g_hasTexture = sprite2d->m_pTexture != nullptr;
    shDraw4.unsafe_fastcall(sprite2d, edx, x, y, w, h, color);
    g_hasTexture = false;
}

SafetyHookInline shDraw5 = {};
void __fastcall Draw5(CSprite2d* sprite2d, void* edx, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, CRGBA* color)
{
    g_hasTexture = sprite2d->m_pTexture != nullptr;
    shDraw5.unsafe_fastcall(sprite2d, edx, x1, y1, x2, y2, x3, y3, x4, y4, color);
    g_hasTexture = false;
}

void __cdecl DrawRect1(CRect* r, CRGBA* col)
{
    g_isFullscreen = IsFullscreen(r);
    shDrawRect1.unsafe_ccall(r, col);
    g_isFullscreen = false;
}

SafetyHookInline shDrawRect2 = {};
void __cdecl DrawRect2(CRect* r, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom)
{
    g_skipXCorrection = true;
    shDrawRect2.unsafe_ccall(r, colorLeft, colorTop, colorRight, colorBottom);
    g_skipXCorrection = false;
}

SafetyHookInline shDrawRectXLU = {};
void __cdecl DrawRectXLU(CRect* r, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom)
{
    g_skipXCorrection = true;
    shDrawRectXLU.unsafe_ccall(r, colorLeft, colorTop, colorRight, colorBottom);
    g_skipXCorrection = false;
}

SafetyHookInline shAddSpriteToBank = {};
void __cdecl AddSpriteToBank(int index, CRect* r, CRGBA* color, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    return shAddSpriteToBank.unsafe_ccall(index, r, color, x1, y1, x2, y2, x3, y3, x4, y4);
}

SafetyHookInline shSetVertices1 = {};
void __cdecl SetVertices1(CRect* posn, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4)
{
    if (g_isFullscreen && g_hasTexture)
        *posn = g_contentRect;
    else
        ScaleRect(posn);
    return shSetVertices1.unsafe_ccall(posn, colorLeft, colorTop, colorRight, colorBottom, u1, v1, u2, v2, u3, v3, u4, v4);
}

SafetyHookInline shSetVertices2 = {};
void __cdecl SetVertices2(CRect* posn, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom, unsigned int _far)
{
    if (g_isMouseCursor)
    {
        return shSetVertices2.unsafe_ccall(posn, colorLeft, colorTop, colorRight, colorBottom, _far);
    }

    if (g_drawingBars)
    {
        // bar rects already in screen space, pass through
    }
    else if (g_isFullscreen && g_hasTexture)
    {
        // textured fullscreen: apply texture-aspect-derived rect
        *posn = g_contentRect;
    }
    else if (!g_isFullscreen)
    {
        // normal UI element: scale
        ScaleRect(posn);
    }
    // else: untextured fullscreen (fade/overlay): stretch, don't touch
    return shSetVertices2.unsafe_ccall(posn, colorLeft, colorTop, colorRight, colorBottom, _far);
}

SafetyHookInline shSetVertices3 = {};
void __cdecl SetVertices3(RwIm2DVertex* vertices, CRect* posn, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4)
{
    return shSetVertices3.unsafe_ccall(vertices, ScaleRect(posn), colorLeft, colorTop, colorRight, colorBottom, u1, v1, u2, v2, u3, v3, u4, v4);
}

SafetyHookInline shSetVertices4 = {};
void __cdecl SetVertices4(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, CRGBA* colorLeft, CRGBA* colorTop, CRGBA* colorRight, CRGBA* colorBottom)
{
    float off = GetHudOffset();
    return shSetVertices4.unsafe_ccall(CorrectX(x1) + off, y1, CorrectX(x2) + off, y2, CorrectX(x3) + off, y3, CorrectX(x4) + off, y4, colorLeft, colorTop, colorRight, colorBottom);
}

SafetyHookInline shSetVertices5 = {};
void __cdecl SetVertices5(int numVerts, float* posn, float* texCoors, CRGBA* color)
{
    float off = GetHudOffset();
    for (int i = 0; i < numVerts; i++)
        posn[i * 2] = CorrectX(posn[i * 2]) + off;
    return shSetVertices5.unsafe_ccall(numVerts, posn, texCoors, color);
}

SafetyHookInline shSetMaskVertices = {};
void __cdecl SetMaskVertices(int numVerts, float* posn)
{
    float off = GetHudOffset();
    for (int i = 0; i < numVerts; i++)
        posn[i * 2] = CorrectX(posn[i * 2]) + off;
    return shSetMaskVertices.unsafe_ccall(numVerts, posn);
}

SafetyHookInline shShowRadarTrace{};
void __cdecl ShowRadarTrace(float x, float y, unsigned int size, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    g_skipXCorrection = true;
    shShowRadarTrace.unsafe_ccall(CorrectX(x), y, size, red, green, blue, alpha);
    g_skipXCorrection = false;
}

SafetyHookInline shDrawBordersForWideScreen{};
void __fastcall DrawBordersForWideScreen(CCamera* camera, void* edx)
{
    g_skipXCorrection = true;

    uint32_t now = CTimer::m_snTimeInMilliseconds;

    bool widescreenOn = camera->m_WideScreenOn;
    bool isFading = camera->GetFading();
    float fadeAlpha = camera->m_fFLOATingFade;

    static bool  s_prevIsFading = false;
    static bool  s_wasGradualFade = false;

    bool fadeJustStarted = !s_prevIsFading && isFading;
    bool fadeJustEnded = s_prevIsFading && !isFading;

    bool isInstantPop = false;

    if (fadeJustStarted)
    {
        if (fadeAlpha > 50.0f)
        {
            isInstantPop = true;
            s_wasGradualFade = false;
        }
        else
        {
            s_wasGradualFade = true;
        }
    }

    bool shouldBeActive = widescreenOn && !isFading;

    if (isInstantPop)
    {
        s_bordersMult = 0.0f;
    }
    else if (fadeJustEnded && s_wasGradualFade)
    {
        shouldBeActive = false;
    }

    TickBorderAnim(now, shouldBeActive);

    if (shouldBeActive)
    {
        float reductionPercent = camera->m_ScreenReductionPercentage;

        const float targetAspect = DEFAULT_ASPECT_RATIO;
        const float screenAspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

        float visibleFraction = std::max(1.0f - reductionPercent / 100.0f, 0.01f);
        float contentAspect = targetAspect / visibleFraction;

        s_hasLetterbox = false;
        s_hasPillar = false;
        s_barHeight = 0.0f;
        s_pillarWidth = 0.0f;

        if (screenAspect > contentAspect)
        {
            // Pillarbox
            s_hasPillar = true;
            float contentWidth = SCREEN_HEIGHT * contentAspect;
            s_pillarWidth = (SCREEN_WIDTH - contentWidth) * 0.5f;
        }
        else
        {
            // Letterbox
            s_hasLetterbox = true;
            float contentHeight = SCREEN_WIDTH / contentAspect;
            s_barHeight = (SCREEN_HEIGHT - contentHeight) * 0.5f;
        }

        if (camera->m_BlurType == MOTION_BLUR_NONE || camera->m_BlurType == MOTION_BLUR_LIGHT_SCENE)
            camera->m_imotionBlurAddAlpha = 80;

        s_lastActiveMs = now;
    }

    // Render whenever the animation multiplier is still visible
    auto pref = CMenuManager::m_PrefsUseWideScreen.get();
    if (pref && s_bordersMult > 0.01f)
    {
        bool renderBorders = false;
        if (pref == CutsceneBordersMode::Letterbox && s_hasLetterbox)
            renderBorders = true;
        else if (pref == CutsceneBordersMode::Pillarbox && s_hasPillar)
            renderBorders = true;
        else if (pref == CutsceneBordersMode::Both)
            renderBorders = true;

        if (renderBorders)
            RenderBorderBars();
    }

    s_prevIsFading = isFading;

    g_skipXCorrection = false;
}

class Sprite2d
{
public:
    Sprite2d()
    {
        WFP::onGameInitEvent() += []()
        {
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 89 D9 83 C4 34");
                shSetVertices1 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetVertices1);

                pattern = hook::pattern("E8 ? ? ? ? 89 D9 83 C4 ? E8 ? ? ? ? 6A ? 68 ? ? ? ? 6A ? E8 ? ? ? ? 83 C4 ? 83 C4");
                shSetVertices2 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetVertices2);

                pattern = hook::pattern("E8 ? ? ? ? 8B 04 9D ? ? ? ? FF 04 9D");
                shSetVertices3 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetVertices3);

                pattern = hook::pattern("E8 ? ? ? ? 89 D9 83 C4 30");
                shSetVertices4 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetVertices4);

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 83 7C 24 ? ? 7E");
                shSetVertices5 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetVertices5);

                pattern = hook::pattern("E8 ? ? ? ? 59 59 6A ? 68 ? ? ? ? 6A ? E8 ? ? ? ? 43");
                shSetMaskVertices = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetMaskVertices);
            }

            {
                auto pattern = hook::pattern("E8 ? ? ? ? EB ? 8D 44 20 ? B9");
                shDraw1 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Draw1);

                pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? FF 35 ? ? ? ? FF 35 ? ? ? ? E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? FF 35");
                shDraw2 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Draw2);

                pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 89 C0 8D 40 ? 8D 8C 24 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 50");
                shDraw3 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Draw3);

                pattern = hook::pattern("E8 ? ? ? ? 8D 4C 24 ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8D 44 24 ? 8D 8B");
                shDraw4 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Draw4);

                pattern = hook::pattern("FF 74 24 ? E8 ? ? ? ? 83 C4 ? 5D 5E");
                shDraw5 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(4)).as_int(), Draw5);

                pattern = hook::pattern("E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 8B 83 ? ? ? ? 59 48");
                shDrawRect1 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), DrawRect1);

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E9 ? ? ? ? 0F BF B4 24");
                shDrawRect2 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), DrawRect2);

                pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
                shDrawRectXLU = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), DrawRectXLU);

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? ? ? ? ? ? ? ? ? ? ? ? ? 50 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 50");
                shAddSpriteToBank = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AddSpriteToBank);
            }

            {
                auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E9 ? ? ? ? 8D 84 20 ? ? ? ? FF 74 24");
                shShowRadarTrace = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), ShowRadarTrace);
            }

            {
                auto pattern = hook::pattern("74 ? B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 85 C0");
                injector::MakeNOP(pattern.get_first(), 2, true);

                pattern = hook::pattern("74 ? B9 ? ? ? ? E8 ? ? ? ? 8B 35");
                injector::MakeNOP(pattern.get_first(), 2, true);

                pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 85 C0 74 ? 0F BE 90");
                shDrawBordersForWideScreen = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), DrawBordersForWideScreen);
            }

            {
                // Draw black border for sniper and rocket launcher
                auto pattern = hook::pattern("E8 ? ? ? ? 59 59 B9 ? ? ? ? E8");
                static auto Render2dStuff = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    CRGBA black(0, 0, 0, 255);
                    g_skipXCorrection = true;
                    g_drawingBars = true;

                    // Sniper
                    CRect t1(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT / 2 - SCREEN_SCALE_Y(210));
                    CRect t2(0.0f, SCREEN_HEIGHT / 2 + SCREEN_SCALE_Y(210), SCREEN_WIDTH, SCREEN_HEIGHT);
                    CRect t3(0.0f, 0.0f, SCREEN_WIDTH / 2 - SCREEN_SCALE_X(210), SCREEN_HEIGHT);
                    CRect t4(SCREEN_WIDTH / 2 + SCREEN_SCALE_X(210), 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
                    shDrawRect1.unsafe_ccall(&t1, &black);
                    shDrawRect1.unsafe_ccall(&t2, &black);
                    shDrawRect1.unsafe_ccall(&t3, &black);
                    shDrawRect1.unsafe_ccall(&t4, &black);

                    g_drawingBars = false;
                    g_skipXCorrection = false;
                });
            }
        };
    }
} Sprite2d;