#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float* pfFieldOfView;
    float fFOVFactor;
    float fAspectRatio;
    float fUnk1;
    float fHudOffset;
    float f2DSpritesOffset;
    float f2DSpritesScale;
    float fHudHorScale;
    float fHudVerScale;
    uint16_t FrontendAspectRatioWidth;
    uint16_t FrontendAspectRatioHeight;
} Screen;

std::string szCustomUserFilesDirectory;
const char* __cdecl InitUserDirectories()
{
    CreateDirectoryA(szCustomUserFilesDirectory.c_str(), NULL);
    return szCustomUserFilesDirectory.c_str();
}

injector::hook_back<void(__cdecl*)(float, float, float, float, int, int, int, int, int)> hbDrawRect;
void __cdecl DrawRectHook(float fLeft, float fTop, float fRight, float fBottom, int r, int g, int b, int a, int pTexture)
{
    if (pTexture && fLeft == 0.0f && fTop == 0.0f && fRight == 1.0f && fBottom == 1.0f)
    {
        if (Screen.f2DSpritesOffset)
        {
            hbDrawRect.fun(fLeft, fTop, fRight, fBottom, 0, 0, 0, 255, pTexture);
            hbDrawRect.fun(Screen.f2DSpritesOffset, fTop, Screen.f2DSpritesScale, fBottom, r, g, b, a, pTexture);
        }
        else
        {
            auto w = (float)(*(uint16_t*)(pTexture + 12));
            auto h = (float)(*(uint16_t*)(pTexture + 16));
            if (w == h)
            {
                w = 4.0f;
                h = 3.0f;
            }
            auto f2DSpritesOffset = ((Screen.fWidth - Screen.fHeight * (w / h)) / 2.0f) / Screen.fWidth;
            auto f2DSpritesScale = 1.0f / (Screen.fAspectRatio / (w / h));
            hbDrawRect.fun(fLeft, fTop, fRight, fBottom, 0, 0, 0, 255, pTexture);
            hbDrawRect.fun(f2DSpritesOffset, fTop, f2DSpritesScale, fBottom, r, g, b, a, pTexture);
        }
        return;
    }
    return hbDrawRect.fun(fLeft, fTop, fRight, fBottom, r, g, b, a, pTexture);
}

injector::hook_back<int(__cdecl*)(float a1, float a2, float a3, float a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28)> hbDrawText;
int __cdecl DrawTextHook(float a1, float a2, float a3, float a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28)
{
    a1 += 0.5f; //test
    return hbDrawText.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28);
}

void Init()
{
    CIniReader iniReader("");
    Screen.fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);
    static bool bDisableGamepadInput = iniReader.ReadInteger("MAIN", "DisableGamepadInput", 0) != 0;
    szCustomUserFilesDirectory = iniReader.ReadString("MAIN", "CustomUserFilesDirectoryInGameDir", "");
    auto szFrontendAspectRatio = iniReader.ReadString("MAIN", "FrontendAspectRatio", "auto");
    if (strncmp(szFrontendAspectRatio.c_str(), "auto", 4) != 0)
    {
        Screen.FrontendAspectRatioWidth = std::stoi(szFrontendAspectRatio.c_str());
        Screen.FrontendAspectRatioHeight = std::stoi(strchr(szFrontendAspectRatio.c_str(), ':') + 1);
    }
    else
    {
        Screen.FrontendAspectRatioWidth = 0;
        Screen.FrontendAspectRatioHeight = 0;
    }

    auto pattern = hook::pattern("E8 ? ? ? ? 59 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8"); //0x5E2579
    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.Width = **(int32_t**)hook::get_pattern("8B 15 ? ? ? ? 8B 44 24 14 8B 0D", 2); //*(int *)0x829584;
            Screen.Height = **(int32_t**)hook::get_pattern("8B 15 ? ? ? ? 8B 44 24 14 8B 0D", 12); //*(int *)0x829588;
            Screen.fUnk1 = **(float**)hook::get_pattern("D8 35 ? ? ? ? D8 0D ? ? ? ? D8 3D ? ? ? ? D9 1D ? ? ? ?", 2);//*(float *)0x7D3450;
            Screen.pfFieldOfView = *(float**)hook::get_pattern("D8 0D ? ? ? ? DD DA D9 C2 D8", 2);//*(float *)0x715C94;

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHudHorScale = static_cast<float>(Screen.fHeight / ((Screen.fHeight / (640.0f / Screen.fAspectRatio)) / (Screen.fUnk1) * Screen.fWidth));
            Screen.fHudVerScale = 0.7f * (Screen.fAspectRatio / (4.0f / 3.0f));
            if (Screen.FrontendAspectRatioWidth && Screen.FrontendAspectRatioHeight)
            {
                Screen.f2DSpritesOffset = ((Screen.fWidth - Screen.fHeight * ((float)Screen.FrontendAspectRatioWidth / (float)Screen.FrontendAspectRatioHeight)) / 2.0f) / Screen.fWidth;
                Screen.f2DSpritesScale = 1.0f / (Screen.fAspectRatio / ((float)Screen.FrontendAspectRatioWidth / (float)Screen.FrontendAspectRatioHeight));
            }

            //Disable menu aspect ratio switch
            auto pattern = hook::pattern("83 3D ? ? ? ? 00 75 ? E8 ? ? ? ? 89 C0 83");
            injector::MakeNOP(pattern.get_first(0), 9, true); //0x5DAA20
            injector::WriteMemory<uint8_t>(pattern.get_first(16), 0xC3, true); //ret 0x5DAA30

            pattern = hook::pattern("D9 05 ? ? ? ? DD DB 8B 55 3C");
            injector::WriteMemory(*(uintptr_t*)pattern.get_first(2), Screen.fHudHorScale, true); //0x7D3458

            pattern = hook::pattern("83 3D ? ? ? ? 01 74 ? D9 05 ? ? ? ? D8 1D");
            injector::WriteMemory<uint8_t>(pattern.get_first(11 + 15), 0xEB, true); //0x475BF3
            injector::WriteMemory(pattern.get_first(11), &Screen.fHudVerScale, true); //0x475BE2 + 0x2
            pattern = hook::pattern("8B 15 ? ? ? ? 89 14 24 D9 04 24 D8 05");
            injector::WriteMemory(pattern.get_first(2), &Screen.fHudVerScale, true); //0x475C09 + 0x2
            pattern = hook::pattern("68 ? ? ? ? 55 E8 ? ? ? ? 59 59 E8");
            injector::WriteMemory(pattern.get_first(1), &Screen.fHudVerScale, true); //0x476246 + 0x1

            static float fScreenWidthScale = 0.546875f;
            pattern = hook::pattern("8B 0D ? ? ? ? 89 4C 24 04 D9 1C");
            injector::WriteMemory(pattern.get_first(2), &fScreenWidthScale, true); //0x475C1B + 0x2

            //2D Sprites scaling
            auto sub_5F96F0 = (uintptr_t)hook::get_pattern("53 55 83 EC 08 8B 6C 24 34", 0);
            auto sub_5F8D20 = (uintptr_t)hook::pattern("53 56 57 55 D9 44 24 20").count(2).get(1).get<uintptr_t>(0);
            pattern = hook::pattern("E8 ? ? ? ?"); //https://pastebin.com/F1z5i7sq
            for (size_t i = 0, j = 0, k = 0; i < pattern.size(); ++i)
            {
                auto addr = pattern.get(i).get<uint32_t>(0);
                auto dest = injector::GetBranchDestination(addr, true).as_int();
                if (dest == sub_5F96F0)
                {
                    ++j;
                    if (j != 2 && j != 15 && j != 19 && j != 20 && j != 21 && j != 22 && j != 23 && j != 25 && j != 35 && j != 40)
                    {
                        auto ptr = injector::MakeCALL(addr, DrawRectHook, true).get();
                        if (!hbDrawRect.fun)
                            hbDrawRect.fun = ptr;
                    }
                }
                //else
                //{
                //    if (dest == sub_5F8D20)
                //    {
                //        ++k;
                //        if (k == 3)
                //            hbDrawText.fun = injector::MakeCALL(addr, DrawTextHook, true).get();
                //    }
                //}
            }

            if (Screen.fFOVFactor)
            {
                pattern = hook::pattern("D8 35 ? ? ? ? D9 5C 24 04 52 53 E8"); //0x475C49
                struct FOVHook1
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)regs.esp *= Screen.fFOVFactor;
                        auto fov = *Screen.pfFieldOfView;
                        _asm fdiv fov
                        _asm fmul dword ptr ds : [Screen.fFOVFactor]
                    }
                }; injector::MakeInline<FOVHook1>(pattern.get_first(0), pattern.get_first(6));
            }

            if (!szCustomUserFilesDirectory.empty() && szCustomUserFilesDirectory != "0")
            {
                char moduleName[MAX_PATH];
                GetModuleFileNameA(NULL, moduleName, MAX_PATH);
                *(strrchr(moduleName, '\\') + 1) = '\0';
                szCustomUserFilesDirectory = moduleName + szCustomUserFilesDirectory;

                pattern = hook::pattern("E8 ? ? ? ? 50 8D 54 24 08 52 E8 ? ? ? ? 83 C4 0C 8D");
                injector::MakeCALL(pattern.get_first(0), InitUserDirectories, true); //0x605E0B
            }

            if (bDisableGamepadInput)
            {
                pattern = hook::pattern("89 15 ? ? ? ? 8B 4B 04 89 0D ? ? ? ? 81 0D ? ? ? ? 00 00 00 40");
                injector::MakeNOP(pattern.get_first(0), 6, true); //0x491DDE
                injector::MakeNOP(pattern.get_first(9), 16, true); //0x491DE7
            }
        }
    }; injector::MakeInline<AspectRatioHook>(pattern.get_first(0));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00").count_hint(1).empty(), 0x1100);
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