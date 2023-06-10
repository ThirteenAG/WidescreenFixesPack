#include "stdafx.h"
#include <d3d9.h>
#include <vector>
#include <D3DX9Shader.h>
#pragma comment(lib, "D3dx9.lib")

bool IsSplitScreenActive()
{
    auto ptr = *(uint32_t*)0x17CF454;
    return ptr && *(uint32_t*)(ptr + 0x86C) == 1;
}

int32_t GetResX()
{
    return *(int32_t*)(*(uint32_t*)0x186E8BC + 0x120);
}

int32_t GetResY()
{
    return *(int32_t*)(*(uint32_t*)0x186E8BC + 0x124);
}

int32_t GetRelativeResX()
{
    return *(int32_t*)(*(uint32_t*)0x186E8BC + 0x128);
}

int32_t GetRelativeResY()
{
    return *(int32_t*)(*(uint32_t*)0x186E8BC + 0x12C);
}

int32_t GetCurrentSplitScreenResX(int32_t val = 0)
{
    static int32_t CurrentSplitScreenResX = GetResX();
    if (val)
        CurrentSplitScreenResX = val;
    return CurrentSplitScreenResX;
}

int32_t GetCurrentSplitScreenResY(int32_t val = 0)
{
    static int32_t CurrentSplitScreenResY = GetResY() / 2;
    if (val)
        CurrentSplitScreenResY = val;
    return CurrentSplitScreenResY;
}

int32_t GetNativeSplitScreenResX(int32_t val = 0)
{
    static int32_t NativeSplitScreenResX = GetResX();
    if (val)
        NativeSplitScreenResX = val;
    return NativeSplitScreenResX;
}

int32_t GetNativeSplitScreenResY(int32_t val = 0)
{
    static int32_t NativeSplitScreenResY = GetResY() / 2;
    if (val)
        NativeSplitScreenResY = val;
    return NativeSplitScreenResY;
}

float GetDiff()
{
    return (float)GetCurrentSplitScreenResX() / (float)GetNativeSplitScreenResX();
}

int32_t GetRelativeSplitScreenResX()
{
    return (int32_t)((float)GetRelativeResX() / ((float)GetResX() / (float)GetCurrentSplitScreenResX())); //800
}

int32_t GetRelativeSplitScreenResY()
{
    return int32_t((float)GetRelativeResY() / 2.0f); //360
}

int32_t GetHudOffset()
{
    if (IsSplitScreenActive())
        return (int32_t)((((float)GetCurrentSplitScreenResX() - ((float)GetNativeSplitScreenResY() * ((float)GetNativeSplitScreenResX() / (float)GetNativeSplitScreenResY())))) / 2.0f);
    else
        return 0;
}

void __fastcall sub_4F8C60(int _this, int edx, int a2, int32_t* a3)
{
    if (*(uint8_t*)(_this + 28) || *(uint8_t*)0x186E5D0)
        EnterCriticalSection((LPCRITICAL_SECTION)(_this + 4));
    *(uint8_t*)(400 * a2 + _this + 67) = 1;
    auto v4 = (int32_t*)(_this + 400 * a2);
    v4[18] = a3[0];
    v4[19] = a3[1];
    v4[20] = a3[2];
    v4[21] = a3[3];

    if (a2 == 0)
    {
        v4[18] = 0;
        v4[19] = 0;
        v4[20] = GetResX();
        v4[21] = GetResY() / 2;
    }
    else if (a2 == 1)
    {
        v4[18] = 0;
        v4[19] = GetResY() / 2;
        v4[20] = GetResX();
        v4[21] = GetResY();
    }

    GetNativeSplitScreenResX(a3[2] - a3[0]);
    GetNativeSplitScreenResY(a3[3] - a3[1]);
    GetCurrentSplitScreenResX(v4[20] - v4[18]);
    GetCurrentSplitScreenResY(v4[21] - v4[19]);

    if (*(uint8_t*)(_this + 28) || *(uint8_t*)0x186E5D0)
        LeaveCriticalSection((LPCRITICAL_SECTION)(_this + 4));
}

float __cdecl sub_974C80(int a1)
{
    if (IsSplitScreenActive())
        return (float)(*(uint32_t*)(*(uint32_t*)0x186E23C + 80) - *(uint32_t*)(*(uint32_t*)0x186E23C + 72)) * (float)a1 * (1.0f / GetRelativeSplitScreenResX());
    else
        return (float)(a1 * *(uint32_t*)(*(uint32_t*)0x186E8BC + 288)) * ((1.0f / GetRelativeResX()));
}

float __cdecl sub_974CD0(int a1)
{
    if (IsSplitScreenActive())
        return (float)(*(uint32_t*)(*(uint32_t*)0x186E23C + 80) - *(uint32_t*)(*(uint32_t*)0x186E23C + 72))
            * GetRelativeSplitScreenResY()
            * (1.0f / GetRelativeSplitScreenResX())
            * (float)a1
            * (1.0f / GetRelativeSplitScreenResY());
    else
        return (float)(a1 * *(uint32_t*)(*(uint32_t*)0x186E8BC + 292)) * (1.0f / GetRelativeResY());
}

float __stdcall sub_55DB40(int a1)
{
    return sub_974C80(a1);
}

float __stdcall sub_55DBA0(int a1)
{
    return sub_974CD0(a1);
}

float __cdecl sub_974C80_center(int a1)
{
    return sub_974C80(a1) + GetHudOffset();
}

float __stdcall sub_55DB40_center(int a1)
{
    return sub_974C80(a1) + GetHudOffset();
}

float __stdcall sub_55DB40_stretch(int a1)
{
    return sub_974C80(a1) + (GetHudOffset() * 2.0f);
}

void __stdcall sub_58DDF0(uint32_t * a1, int* a2, int a3, uint16_t a4)
{
    int v4; // eax
    float v5; // st7
    int v6; // esi
    int v7; // ecx
    float v8; // st7
    int v9; // eax
    float v10; // st7
    float v11; // st7
    int v12; // [esp-20h] [ebp-2Ch]
    int v13; // [esp-1Ch] [ebp-28h]
    int v14; // [esp-18h] [ebp-24h]
    int v15; // [esp-18h] [ebp-24h]
    int v16; // [esp-14h] [ebp-20h]
    int v17; // [esp-10h] [ebp-1Ch]
    float v18; // [esp+4h] [ebp-8h]

    if (IsSplitScreenActive())
    {
        *a1 = (int)sub_974C80(*a1);
        *a2 = (int)sub_974CD0(*a2);
        *(float*)(a3 + 16) = sub_974C80((int)*(float*)(a3 + 16));
        *(float*)(a3 + 20) = sub_974CD0((int)*(float*)(a3 + 20));
        v4 = (int)sub_974C80(*(uint32_t*)(a3 + 8));
        v14 = *(uint32_t*)(a3 + 12);
        *(uint32_t*)(a3 + 8) = v4;
        *(uint32_t*)(a3 + 12) = (int)sub_974CD0(v14);
        v5 = sub_974C80(*(uint32_t*)(a3 + 32));
        v12 = *(uint32_t*)(a3 + 36);
        *(uint32_t*)(a3 + 32) = (int32_t)v5;
        *(uint32_t*)(a3 + 36) = (int32_t)sub_974CD0(v12);
    }
    else
    {
        v6 = *(uint32_t*)(*(uint32_t*)0x186E8BC + 292);
        v18 = 0.5625f / (float)((float)v6 / (float)*(int*)(*(uint32_t*)0x186E8BC + 288));
        *a1 = (int)sub_974C80(*a1);
        v7 = (int)sub_974CD0(*a2);
        *a2 = v7;
        if (a4)
            *a2 = (int)(float)((float)((float)(v7 - v6 / 2) * v18) + (float)(v6 / 2));
        *(float*)(a3 + 16) = sub_974C80((int)*(float*)(a3 + 16));
        v8 = sub_974CD0((int)*(float*)(a3 + 20));
        v17 = *(uint32_t*)(a3 + 8);
        *(float*)(a3 + 20) = v8 * v18;
        v9 = (int)sub_974C80(v17);
        v16 = *(uint32_t*)(a3 + 12);
        *(uint32_t*)(a3 + 8) = v9;
        v10 = sub_974CD0(v16);
        v15 = *(uint32_t*)(a3 + 32);
        *(uint32_t*)(a3 + 12) = (int)(float)((float)(int)v10 * v18);
        v11 = sub_974C80(v15);
        v13 = *(uint32_t*)(a3 + 36);
        *(uint32_t*)(a3 + 32) = (int32_t)v11;
        *(uint32_t*)(a3 + 36) = (int32_t)sub_974CD0(v13);
    }
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bDisableDamageOverlay = iniReader.ReadInteger("MAIN", "DisableDamageOverlay", 1) != 0;
    auto bDisableFilmGrain = iniReader.ReadInteger("MAIN", "DisableFilmGrain", 1) != 0;
    auto bDisableFade = iniReader.ReadInteger("MAIN", "DisableFade", 0) != 0;
    auto bDisableGUICommandFar = iniReader.ReadInteger("MAIN", "DisableGUICommandFar", 0) != 0;
    
    if (bSkipIntro)
    {
        injector::MakeNOP(0x97BE91, 6);
        injector::MakeJMP(0x97BE91, 0x97BFDA);

        injector::WriteMemory<uint16_t>(0xD23CFB, 0xE990, true);
    }

    injector::MakeCALL(0x50076B, sub_4F8C60, true);
    injector::MakeCALL(0x50079E, sub_4F8C60, true);

    //hud fix
    injector::MakeJMP(0x974C80, sub_974C80, true);
    injector::MakeJMP(0x974CD0, sub_974CD0, true);

    injector::MakeJMP(0x55DB40, sub_55DB40, true);
    injector::MakeJMP(0x55DBA0, sub_55DBA0, true);

    //disable shader overlays (don't scale to fullscreen)
    {
        static bool bDisableShader = false;
        struct SetVertexShaderConstantFHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (IsSplitScreenActive())
                {
                    auto pConstantData = (float*)regs.eax;
                    if ((fabs(pConstantData[0] - (1.0f / (float)GetResX())) < FLT_EPSILON))
                    {
                        bDisableShader = true;
                    }
                }
                regs.ecx = *(uint32_t*)(regs.ecx + 0x178);
            }
        }; injector::MakeInline<SetVertexShaderConstantFHook>(0xF3CD03, 0xF3CD03+6);

        static std::vector<uint8_t> pbFunc;
        struct SetPixelShaderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (bDisableShader && IsSplitScreenActive())
                {
                    auto pShader = (IDirect3DPixelShader9*)regs.eax;
                    if (pShader != nullptr)
                    {
                        UINT len;
                        pShader->GetFunction(nullptr, &len);
                        if (pbFunc.size() < len)
                            pbFunc.resize(len);
        
                        pShader->GetFunction(pbFunc.data(), &len);
        
                        uint32_t crc32(uint32_t crc, const void* buf, size_t size);
                        auto crc = crc32(0, pbFunc.data(), len);
        
                        if (crc == 0x498080AC || crc == 0x793BE067 || crc == 0xFD473559) // overlay on low health
                        {
                            unsigned char dummyShader[156] = {
                                0x00, 0x03, 0xFF, 0xFF, 0xFE, 0xFF, 0x16, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
                                0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x01, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
                                0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53,
                                0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65,
                                0x72, 0x20, 0x39, 0x2E, 0x32, 0x39, 0x2E, 0x39, 0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31, 0x00,
                                0x51, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0F, 0x80,
                                0x00, 0x00, 0x00, 0xA0, 0x41, 0x00, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x80, 0x01, 0x00, 0x00, 0x02,
                                0x00, 0x08, 0x0F, 0x80, 0x00, 0x00, 0x55, 0xA0, 0xFF, 0xFF, 0x00, 0x00
                            };

                            static IDirect3DPixelShader9* g_pPixelShader = nullptr;
                            if (!g_pPixelShader)
                            {
                                auto g_pd3dDevice = (IDirect3DDevice9*)regs.ebp;
                                auto hr = g_pd3dDevice->CreatePixelShader((DWORD*)&dummyShader[0], &g_pPixelShader);
                                if (SUCCEEDED(hr))
                                {
                                    regs.eax = (uint32_t)g_pPixelShader;
                                }
                            }
                            else
                                regs.eax = (uint32_t)g_pPixelShader;
                        }
                        //pbFunc.clear();
                    }
                }
                *(uint32_t*)(regs.edi + 0x28) = regs.eax;
                regs.ecx = *(uint32_t*)(regs.ebp + 0x0);
            }
        }; injector::MakeInline<SetPixelShaderHook>(0xF3CA8E, 0xF3CA8E + 6);

        //struct SetVertexShaderHook
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        if (bDisableShader && IsSplitScreenActive())
        //        {
        //            auto pShader = (IDirect3DVertexShader9*)regs.eax;
        //            if (pShader != nullptr)
        //            {
        //                UINT len;
        //                pShader->GetFunction(nullptr, &len);
        //                if (pbFunc.size() < len)
        //                    pbFunc.resize(len);
        //
        //                pShader->GetFunction(pbFunc.data(), &len);
        //
        //                uint32_t crc32(uint32_t crc, const void* buf, size_t size);
        //                auto crc = crc32(0, pbFunc.data(), len);
        //
        //                if (crc == 0x4F0EE939)
        //                {
        //                    regs.eax = 0;
        //                }
        //
        //                pbFunc.clear();
        //            }
        //            bDisableShader = false;
        //        }
        //        *(uint32_t*)(regs.edi + 0x24) = regs.eax;
        //        regs.ecx = *(uint32_t*)(regs.ebp + 0x0);
        //    }
        //}; injector::MakeInline<SetVertexShaderHook>(0xF3CA40, 0xF3CA40 + 6);
    }

    {
        //injector::MakeCALL(0x5103C3, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(0x58DE0F, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DE38, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DE52, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DE74, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DF11, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DF6D, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DF8B, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DFBE, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(0x58E052, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E06C, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E08E, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E127, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E145, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E178, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x96CB66, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80 //press ok to continue
        //injector::MakeCALL(0x96CBB4, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x97A188, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80 // skills background
        //injector::MakeCALL(0x97A1A2, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(0x97A787, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x97A7A1, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x98B263, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x98B898, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x990887, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ADB0B, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ADD6B, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ADE29, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9AE18F, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9BDF69, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9BF6EF, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9C86F4, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9C94C3, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9CF095, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9CF1F9, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9D9325, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9E8123, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ECFEB, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ED413, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ED9E0, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA115FA, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA116F0, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA1611E, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA1A84A, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80

        injector::MakeCALL(0x55DCC7, sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(0x55DCE5, sub_55DB40_stretch, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55DD52, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55DD8F, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(0x55DE28, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55DE46, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55DEDC, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55DF52, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55DFE8, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55E006, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(0x55E09C, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
        //injector::MakeCALL(0x55E0C8, sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40
    }
    
    //3d blips fix
    injector::MakeNOP(0x97D139, 2);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("85 C0 74 CB 8B 0D"));
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

uint32_t crc32_tab[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32(uint32_t crc, const void* buf, size_t size)
{
    const uint8_t* p;

    p = (uint8_t*)buf;
    crc = crc ^ ~0U;

    while (size--)
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

    return crc ^ ~0U;
}