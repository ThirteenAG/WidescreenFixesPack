#include "game.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

extern int nMenuAlignment;

void __cdecl CDraw__CalculateAspectRatio();
inline float getDynamicScreenFieldOfView(float fFactor);
void setScreenFieldOfView(float fFactor);
void FiretruckAmblncFix();
void MenuFix();
void RsSelectDeviceHook();
void HudFix();
void ApplyINIchanges();
void DrawBordersForWideScreen_patch();
int SetDropShadowPosition(short);
void RadarScaling_compat_patch();

//////steam/////
void __cdecl CDraw__CalculateAspectRatio_steam();
inline float getDynamicScreenFieldOfView_steam(float fFactor);
void setScreenFieldOfView_steam(float fFactor);
void FiretruckAmblncFix_steam();
void MenuFix_steam();
void RsSelectDeviceHook_steam();
void HudFix_steam();
void ApplyINIchanges_steam();
void DrawBordersForWideScreen_patch_steam();