#include "CCamera.h"
#include "common.h"

extern RsGlobalType* RsGlobal;
extern int(__cdecl* CSprite2dDrawRect)(class CRect const &, class CRGBA const &);
extern uint32_t nHideAABug;
extern bool* bWideScreen;
extern uint32_t* BordersVar1;
extern uint32_t* BordersVar2;

void GetScreenRect(CRect& rect)
{
    float			fScreenRatio = *CDraw::pfScreenAspectRatio;
    float			dScreenHeightWeWannaCut = ((-9.0f / 16.0f) * fScreenRatio + 1.0f);
    float			dBorderProportionsFix = ((-144643.0f / 50000.0f) * fScreenRatio * fScreenRatio) + ((807321.0f / 100000.0f) * fScreenRatio) - (551143.0f / 100000.0f);

    if (dBorderProportionsFix < 0.0)
        dBorderProportionsFix = 0.0;

    if (dScreenHeightWeWannaCut > 0.0)
    {
        // Letterbox
        rect.m_fBottom = ((float)RsGlobal->MaximumHeight / 2) * (dScreenHeightWeWannaCut - dBorderProportionsFix);
        rect.m_fTop = (float)RsGlobal->MaximumHeight - (((float)RsGlobal->MaximumHeight / 2) * (dScreenHeightWeWannaCut + dBorderProportionsFix));
    }
    else
    {
        // Pillarbox
        dScreenHeightWeWannaCut = -dScreenHeightWeWannaCut;

        rect.m_fLeft = ((float)RsGlobal->MaximumWidth / 4) * dScreenHeightWeWannaCut;
        rect.m_fRight = (float)RsGlobal->MaximumWidth - ((float)RsGlobal->MaximumWidth / 4) * dScreenHeightWeWannaCut;
    }
}

void Hide1pxAABug()
{
    CSprite2dDrawRect(CRect(0.0f, -5.0f, (float)RsGlobal->MaximumWidth, 0.5f), CRGBA(0, 0, 0, 255));
    CSprite2dDrawRect(CRect(-5.0f, -1.0f, 0.5f, (float)RsGlobal->MaximumHeight), CRGBA(0, 0, 0, 255));
    if (nHideAABug == 2)
    {
        CSprite2dDrawRect(CRect(0.0f, (float)RsGlobal->MaximumHeight - 1.5f, (float)RsGlobal->MaximumWidth, (float)RsGlobal->MaximumHeight + 5.0f), CRGBA(0, 0, 0, 255));
        CSprite2dDrawRect(CRect((float)RsGlobal->MaximumWidth - 1.0f, 0.0f, (float)RsGlobal->MaximumWidth + 5.0f, (float)RsGlobal->MaximumHeight + 5.0f), CRGBA(0, 0, 0, 255));
    }
}

void CCamera::DrawBordersForWideScreen()
{
    if (*bWideScreen == false)
        return;

    CRect		ScreenRect;

    ScreenRect.m_fLeft = -1000.0f;
    ScreenRect.m_fBottom = -1000.0f;
    ScreenRect.m_fRight = -1000.0f;
    ScreenRect.m_fTop = -1000.0f;

    GetScreenRect(ScreenRect);

    if (!*BordersVar1 || *BordersVar1 == 2)
        *BordersVar2 = 80;

    // Letterbox
    if (ScreenRect.m_fBottom > 0.0 && ScreenRect.m_fTop > 0.0)
    {
        CSprite2dDrawRect(CRect(-5.0f, -5.0f, (float)RsGlobal->MaximumWidth + 5.0f, ScreenRect.m_fBottom), CRGBA(0, 0, 0, 255));
        CSprite2dDrawRect(CRect(-5.0f, ScreenRect.m_fTop, (float)RsGlobal->MaximumWidth + 5.0f, (float)RsGlobal->MaximumHeight + 5.0f), CRGBA(0, 0, 0, 255));
    }
    //Pillarbox
    else if (ScreenRect.m_fLeft > 0.0 && ScreenRect.m_fRight > 0.0)
    {
        CSprite2dDrawRect(CRect(-5.0f, -5.0f, ScreenRect.m_fLeft, (float)RsGlobal->MaximumHeight + 5.0f), CRGBA(0, 0, 0, 255));
        CSprite2dDrawRect(CRect(ScreenRect.m_fRight, -5.0f, (float)RsGlobal->MaximumWidth + 5.0f, (float)RsGlobal->MaximumHeight + 5.0f), CRGBA(0, 0, 0, 255));
    }
}