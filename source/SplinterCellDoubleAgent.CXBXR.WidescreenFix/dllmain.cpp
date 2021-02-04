#include "stdafx.h"
#include "cxbxr/cxbxr.h"

struct cxbxr
{
    uintptr_t begin;
    uintptr_t end;
} cxbxr;

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
} Screen;

uint32_t dword_28EE54;
uint32_t dword_24024B;
void __fastcall RenderHud(DWORD* _this, void* edx, float a2, float a3, float a4, float a5, int a6, float a7, float a8, float a9, float a10, int a11, float a12, float a13, float a14, float a15, int a16, float a17, float a18, float a19, float a20, int a21, int a22, int a23, float a24)
{
    ///////////////
    a2 /= 1.3333f;
    a2 += 106.3333f;
    //a3 /= 2.0f;
    a7 /= 1.3333f;
    a7 += 106.3333f;
    //a8 /= 2.0f;
    a12 /= 1.3333f;
    a12 += 106.3333f;
    //a13 /= 2.0f;
    a17 /= 1.3333f;
    a17 += 106.3333f;
    //a18 /= 2.0f;
    ///////////////

    injector::thiscall<uint32_t(DWORD*, int, int)>::call(dword_28EE54, _this, 0, a23);

    float v32 = a17;
    float v33 = a18;
    float v34 = a7;
    float v35 = a8;
    float v37 = a12;
    float v38 = a13;
    float v40 = a2;
    float v41 = a3;

    if (a24 != 0.0f)
    {
        float v45 = (((a2 + a7) + a12) + a17) * 0.25f;
        float v44 = (((a3 + a8) + a13) + a18) * 0.25f;
        float a23a = cos(a24);
        float a24a = sin(a24);
        v40 = (((a2 - v45) * a23a) - ((a3 - v44) * a24a)) + v45;
        v41 = (((a3 - v44) * a23a) + ((a2 - v45) * a24a)) + v44;
        v35 = (((a8 - v44) * a23a) + ((a7 - v45) * a24a)) + v44;
        v34 = (((a7 - v45) * a23a) - ((a8 - v44) * a24a)) + v45;
        v37 = (((a12 - v45) * a23a) - ((a13 - v44) * a24a)) + v45;
        v38 = (((a13 - v44) * a23a) + ((a12 - v45) * a24a)) + v44;
        v32 = (((a17 - v45) * a23a) - ((a18 - v44) * a24a)) + v45;
        v33 = (((a18 - v44) * a23a) + ((a17 - v45) * a24a)) + v44;
    }

    auto v26 = injector::cstd<uint32_t(int, DWORD*)>::call(dword_24024B, 24, _this + 45);
    if (v26)
    {
        *(float*)(v26 + 0x00) = v40;
        *(float*)(v26 + 0x04) = v41;
        *(DWORD*)(v26 + 0x08) = a22;
        *(DWORD*)(v26 + 0x0C) = a6;
        *(float*)(v26 + 0x10) = a4;
        *(float*)(v26 + 0x14) = a5;
    }
    auto v27 = injector::cstd<uint32_t(int, DWORD*)>::call(dword_24024B, 24, _this + 45);
    if (v27)
    {
        *(float*)(v27 + 0x00) = v34;
        *(float*)(v27 + 0x04) = v35;
        *(DWORD*)(v27 + 0x08) = a22;
        *(DWORD*)(v27 + 0x0C) = a11;
        *(float*)(v27 + 0x10) = a9;
        *(float*)(v27 + 0x14) = a10;
    }
    auto v28 = injector::cstd<uint32_t(int, DWORD*)>::call(dword_24024B, 24, _this + 45);
    if (v28)
    {
        *(float*)(v28 + 0x00) = v37;
        *(float*)(v28 + 0x04) = v38;
        *(float*)(v28 + 0x08) = a14;
        *(DWORD*)(v28 + 0x0C) = a22;
        *(DWORD*)(v28 + 0x10) = a16;
        *(float*)(v28 + 0x14) = a15;
    }
    auto v29 = injector::cstd<uint32_t(int, DWORD*)>::call(dword_24024B, 24, _this + 45);
    if (v29)
    {
        *(float*)(v29 + 0x00) = v40;
        *(float*)(v29 + 0x04) = v41;
        *(float*)(v29 + 0x08) = a4;
        *(DWORD*)(v29 + 0x0C) = a22;
        *(DWORD*)(v29 + 0x10) = a6;
        *(float*)(v29 + 0x14) = a5;
    }
    auto v30 = injector::cstd<uint32_t(int, DWORD*)>::call(dword_24024B, 24, _this + 45);
    if (v30)
    {
        *(float*)(v30 + 0x00) = v37;
        *(float*)(v30 + 0x04) = v38;
        *(DWORD*)(v30 + 0x08) = a22;
        *(DWORD*)(v30 + 0x0C) = a16;
        *(float*)(v30 + 0x10) = a14;
        *(float*)(v30 + 0x14) = a15;
    }
    auto v31 = injector::cstd<uint32_t(int, DWORD*)>::call(dword_24024B, 24, _this + 45);
    if (v31)
    {
        *(float*)(v31 + 0x00) = v32;
        *(float*)(v31 + 0x04) = v33;
        *(DWORD*)(v31 + 0x08) = a22;
        *(DWORD*)(v31 + 0x0C) = a21;
        *(float*)(v31 + 0x10) = a19;
        *(float*)(v31 + 0x14) = a20;
    }
    _this[8] += 2;
}


void Init()
{
    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScale = Screen.fAspectRatio / (4.0f / 3.0f);

    auto rpattern = hook::range_pattern(cxbxr.begin, cxbxr.end, "E8 ? ? ? ? 80 3D ? ? ? ? ? 74 17");
    injector::MakeCALL(rpattern.get_first(0), &RenderHud, true); //0x2A1181
    rpattern = hook::range_pattern(cxbxr.begin, cxbxr.end, "E8 ? ? ? ? F6 43 0C 01");
    injector::MakeCALL(rpattern.get_first(0), &RenderHud, true); //0x2B6040
    rpattern = hook::range_pattern(cxbxr.begin, cxbxr.end, "E8 ? ? ? ? 8B 45 BC 66 83 38 20");
    injector::MakeCALL(rpattern.get_first(0), &RenderHud, true); //0x2B610C

    rpattern = hook::range_pattern(cxbxr.begin, cxbxr.end, "53 8B 5C 24 0C 56 57 8B 7C 24 10 8B F1 39 7E 18 75 05 39 5E 1C");
    dword_28EE54 = (uint32_t)rpattern.get_first(0);
    rpattern = hook::range_pattern(cxbxr.begin, cxbxr.end, "56 8B 74 24 0C 6A 18 6A 01 8B CE E8 ? ? ? ? 8B 0E 8D 04 40 8D 04 C1 5E C3");
    dword_24024B = (uint32_t)rpattern.get_first(0);
}

CEXP void InitializeASI()
{
    auto pid = getParentPID(GetCurrentProcessId());
    auto name = getProcessName(pid).stem();
    if (name == "cxbx" || name == "cxbxr-ldr")
    {
        std::tie(Screen.Width, Screen.Height) = getWindowDimensions(pid);
        std::call_once(CallbackHandler::flag, []()
        {
            cxbxr.begin = (uintptr_t)GetModuleHandle(nullptr);
            PIMAGE_DOS_HEADER dosHeader = (IMAGE_DOS_HEADER*)(cxbxr.begin + 0);
            PIMAGE_NT_HEADERS ntHeader = (IMAGE_NT_HEADERS*)(cxbxr.begin + dosHeader->e_lfanew);
            cxbxr.end = cxbxr.begin + ntHeader->OptionalHeader.SizeOfImage;

            CallbackHandler::RegisterCallback(Init, hook::range_pattern(cxbxr.begin, cxbxr.end, "E8 ? ? ? ? 80 3D ? ? ? ? ? 74 17 8B 46 5C 8B 88 ? ? ? ? 6A 01"));
        });
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
        {
            InitializeASI();
        }
    }
    return TRUE;
}