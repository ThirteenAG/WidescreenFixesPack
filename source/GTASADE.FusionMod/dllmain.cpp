#include "stdafx.h"
#include <string_view>
#include "kananlib.hpp"
#include "GTA/ue.hpp"

enum eGameState
{
    GS_START_UP = 0,
    GS_INIT_LOGO_MPEG,
    GS_LOGO_MPEG,
    GS_INIT_INTRO_MPEG,
    GS_INTRO_MPEG,
    GS_INIT_ONCE,
    GS_INIT_FRONTEND,
    GS_FRONTEND,
    GS_INIT_PLAYING_GAME,
    GS_PLAYING_GAME,
};

float fHudScale = 1.0f;
float fRadarScale = 1.0f;

uint32_t* OnAMissionFlag;
uint8_t* ScriptSpace;
uint8_t* m_WideScreenOn;
bool IsPlayerOnAMission()
{
    return *OnAMissionFlag && (ScriptSpace[*OnAMissionFlag] == 1);
}

namespace UUI_LegalScreen
{
    using UUI_LegalScreen = void;

    SafetyHookInline shShouldShow{};
    void ShouldShow(UUI_LegalScreen* LegalScreen, void* a2, int8_t* out)
    {
        shShouldShow.fastcall(LegalScreen, a2, out);
        *out = 0;
    }
}

HWND ghWnd;
void LockCursor()
{
    if (ghWnd == GetForegroundWindow())
    {
        RECT windowRect;
        POINT pt = { 0, 0 };

        GetClientRect(ghWnd, &windowRect);
        ClientToScreen(ghWnd, &pt);

        windowRect.left += pt.x + 1;
        windowRect.top += pt.y + 1;
        windowRect.right += pt.x - 1;
        windowRect.bottom += pt.y - 1;

        ClipCursor(&windowRect);
    }
    else
    {
        ClipCursor(NULL);
    }
}

SafetyHookInline shWndProc{};
LRESULT CustomWndProc(void* _this, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ghWnd = hWnd;
    auto res = shWndProc.fastcall<LRESULT>(_this, hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_ACTIVATEAPP:
    case WM_SETFOCUS:
    {
        LockCursor();
        break;
    }
    case WM_KILLFOCUS:
    case WM_CLOSE:
    case WM_DESTROY:
    {
        ClipCursor(NULL);
        break;
    }
    default:
        break;
    }

    return res;
}

bool bIsDefaultWidget = false;
SafetyHookInline shParseWidgetPositionLineFromFile{};
void ParseWidgetPositionLineFromFile(const char* widget, char bDefault)
{
    bIsDefaultWidget = bDefault;
    shParseWidgetPositionLineFromFile.fastcall(widget, bDefault);
}

int vscanf_hook(char* Buffer, char* Format, ...)
{
    std::string temp;
    std::string_view buf(Buffer);
    if (buf.contains("WIDGET_POSITION_PLAYER_INFO") || buf.contains("WIDGET_POSITION_RADAR"))
    {
        std::string PositionAndScale(255, '\0');
        float OriginX, OriginY, ScaleX, ScaleY;
        static float rOriginX2, rOriginY2, rScaleX2, rScaleY2;
        static float pOriginX2, pOriginY2, pScaleX2, pScaleY2;
        std::string Separator(255, '\0');
        std::string Name(255, '\0');
        auto r = sscanf(Buffer, Format, PositionAndScale.data(), &OriginX, &OriginY, &ScaleX, &ScaleY, Separator.data(), Name.data());
        if (r == 7)
        {
            if (buf.contains("WIDGET_POSITION_PLAYER_INFO"))
            {
                if (bIsDefaultWidget)
                {
                    pOriginX2 = OriginX;
                    pOriginY2 = OriginY;
                    pScaleX2 = ScaleX;
                    pScaleY2 = ScaleY;
                }
                else
                {
                    OriginX = pOriginX2;
                    OriginY = pOriginY2;
                    ScaleX = pScaleX2;
                    ScaleY = pScaleY2;
                }

                OriginX = OriginX + (ScaleX - (ScaleX * fHudScale));
                OriginY = OriginY - (ScaleY - (ScaleY * fHudScale));
                ScaleX *= fHudScale;
                ScaleY *= fHudScale;
            }
            else if (buf.contains("WIDGET_POSITION_RADAR"))
            {
                if (bIsDefaultWidget)
                {
                    rOriginX2 = OriginX;
                    rOriginY2 = OriginY;
                    rScaleX2 = ScaleX;
                    rScaleY2 = ScaleY;
                }
                else
                {
                    OriginX = rOriginX2;
                    OriginY = rOriginY2;
                    ScaleX = rScaleX2;
                    ScaleY = rScaleY2;
                }

                OriginX = OriginX - (ScaleX - (ScaleX * fRadarScale));
                OriginY = OriginY + (ScaleY - (ScaleY * fRadarScale));
                ScaleX *= fRadarScale;
                ScaleY *= fRadarScale;
            }

            PositionAndScale.resize(PositionAndScale.find('\0'));
            Separator.resize(Separator.find('\0'));
            Name.resize(Name.find('\0'));
            temp += PositionAndScale;
            temp += " ";
            temp += std::to_string(OriginX);
            temp += " ";
            temp += std::to_string(OriginY);
            temp += " ";
            temp += std::to_string(ScaleX);
            temp += " ";
            temp += std::to_string(ScaleY);
            temp += " ";
            temp += Separator;
            temp += " ";
            temp += Name;
            Buffer = temp.data();
        }
    }

    int result;
    va_list arglist;
    va_start(arglist, Format);
    result = vsscanf(Buffer, Format, arglist);
    va_end(arglist);
    return result;
}

bool bIsFollowPed = false;
SafetyHookInline shProcess_AimWeapon{};
void Process_AimWeapon(void* cam, const void* ThisCamsTarget, float TargetOrientation, float SpeedVar, float SpeedVarDesired)
{
    bIsFollowPed = false;
    shProcess_AimWeapon.fastcall(cam, ThisCamsTarget, TargetOrientation, SpeedVar, SpeedVarDesired);
}

uintptr_t TheCamera;
SafetyHookInline shProcess_FollowPed_SA{};
void Process_FollowPed_SA(void* cam, const void* ThisCamsTarget, float TargetOrientation, float SpeedVar, float SpeedVarDesired, bool bScriptSetAngles)
{
    uint8_t m_bFading = 0;
    if (TheCamera)
        m_bFading = *(uint8_t*)(TheCamera + 0x55);

    bIsFollowPed = true;
    if (bScriptSetAngles || m_bFading)
        return shProcess_FollowPed_SA.fastcall(cam, ThisCamsTarget, TargetOrientation, SpeedVar, SpeedVarDesired, bScriptSetAngles);
    else
        return shProcess_AimWeapon.fastcall(cam, ThisCamsTarget, TargetOrientation, SpeedVar, SpeedVarDesired);
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bSkipMenu = iniReader.ReadInteger("MAIN", "SkipMenu", 1) != 0;
    static auto nIniSaveSlot = (int32_t)iniReader.ReadInteger("MAIN", "SaveSlot", 6) - 1;
    fHudScale = iniReader.ReadFloat("MAIN", "HudScale", 0.8f);
    fRadarScale = iniReader.ReadFloat("MAIN", "RadarScale", 0.75f);
    static auto bImproveCameraPC = iniReader.ReadInteger("MAIN", "ImproveCameraPC", 1) != 0;
    static auto fCenteringDelay = iniReader.ReadFloat("MAIN", "CenteringDelay", 5.0f);
    if (fHudScale <= 0.0f) 
        fHudScale = 1.0f;
    if (fRadarScale <= 0.0f) 
        fRadarScale = 1.0f;
    if (nIniSaveSlot < 1 || nIniSaveSlot > 8)
        nIniSaveSlot = 5;

    if (bSkipIntro)
    {
        auto AcceptPrivacyPolicy = utility::scan_string(utility::get_executable(), "AcceptPrivacyPolicy", true);
        if (AcceptPrivacyPolicy)
        {
            auto AcceptPrivacyPolicyXrefs = utility::scan_references(utility::get_executable(), AcceptPrivacyPolicy.value());

            for (auto& it : AcceptPrivacyPolicyXrefs)
            {
                auto ref = utility::scan_reference(utility::get_executable(), it);
                if (ref)
                {
                    auto res = utility::resolve_displacement(ref.value() - 3);
                    if (res)
                    {
                        struct UUI_LegalScreenRef
                        {
                            const char* AcceptPrivacyPolicyStr;
                            void* AcceptPrivacyPolicy;
                            const char* CanSkipStr;
                            void* CanSkip;
                            const char* HasAcceptedPrivacyPolicyStr;
                            void* HasAcceptedPrivacyPolicy;
                            const char* ShouldShowStr;
                            void* ShouldShow;
                        };

                        auto pLegalScreen = reinterpret_cast<UUI_LegalScreenRef*>(*res);

                        if (std::string_view(pLegalScreen->ShouldShowStr) == "ShouldShow")
                        {
                            UUI_LegalScreen::shShouldShow = safetyhook::create_inline(pLegalScreen->ShouldShow, UUI_LegalScreen::ShouldShow);
                            break;
                        }
                    }
                }
            }
        }
    }

    if (bSkipMenu)
    {
        static uint8_t* gGameState = utility::GetSymbolFromFuncStart<decltype(gGameState)>("menu_mainpresents", [](INSTRUX& ix) { return ix.Instruction == ND_INS_CMP && ix.Operands[0].Type == ND_OP_MEM && ix.Operands[1].Type == ND_OP_IMM && ix.Operands[1].Info.Immediate.Imm == GS_PLAYING_GAME; });
        static void(*USanAndreasInterfaceResumeGameFromMenu)(void*) = nullptr;
        static void* CGenericGameStorageCheckSlotDataValid = nullptr;

        auto s = utility::scan_string(utility::get_executable(), "%s%i%s", true);
        if (s)
        {
            auto xrefs = utility::scan_displacement_references(utility::get_executable(), s.value());
            for (auto& it2 : xrefs)
            {
                INSTRUX ix{};
                auto ip = it2;

                while (true)
                {
                    const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

                    if (!ND_SUCCESS(status)) {
                        break;
                    }

                    if (ix.Instruction == ND_INS_MOV && ix.Operands[1].Type == ND_OP_IMM && ix.Operands[1].Info.Immediate.Imm == 7)
                    {
                        auto disp = utility::resolve_displacement(ip);
                        if (disp)
                        {
                            auto start = utility::find_function_start(ip);
                            if (start)
                            {
                                CGenericGameStorageCheckSlotDataValid = reinterpret_cast<void*>(start.value());
                                break;
                            }
                        }
                    }

                    ip += ix.Length;
                }

                if (CGenericGameStorageCheckSlotDataValid)
                    break;
            }
        }

        if (CGenericGameStorageCheckSlotDataValid)
        {
            auto xrefs = utility::scan_displacement_references(utility::get_executable(), (uintptr_t)CGenericGameStorageCheckSlotDataValid);
            for (auto& it : xrefs)
            {
                auto start = utility::find_function_start(it);
                if (start)
                {
                    INSTRUX ix{};
                    auto ip = start.value();

                    while (true)
                    {
                        const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

                        if (!ND_SUCCESS(status)) {
                            break;
                        }

                        if (ix.Instruction == ND_INS_CMP && ix.Operands[0].Type == ND_OP_MEM && ix.Operands[1].Type == ND_OP_IMM && ix.Operands[1].Info.Immediate.Imm == GS_PLAYING_GAME)
                        {
                            if (ip < it)
                            {
                                auto disp = utility::resolve_displacement(ip);
                                if (disp && disp.value() == (uintptr_t)gGameState)
                                {
                                    auto start = utility::find_function_start(ip);
                                    if (start)
                                    {
                                        if (!utility::find_function_start_with_call(start.value()).has_value())
                                        {
                                            USanAndreasInterfaceResumeGameFromMenu = reinterpret_cast<decltype(USanAndreasInterfaceResumeGameFromMenu)>(start.value());
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        ip += ix.Length;
                    }
                }
            }
        }

        static uint8_t* byte_1450605B4 = utility::GetSymbolFromFuncStart<decltype(gGameState)>(L"SocialClubSignIn", [](INSTRUX& ix) { return ix.Instruction == ND_INS_MOVZX && ix.Operands[0].Type == ND_OP_REG && ix.Operands[1].Type == ND_OP_MEM; });

        if (gGameState && USanAndreasInterfaceResumeGameFromMenu && byte_1450605B4)
        {
            auto strref = utility::find_virtual_function_from_string_ref(utility::get_executable(), L"open /game/Maps/MainMenu", false);
            if (strref)
            {
                static safetyhook::MidHook USanAndreasInterfaceUpdateHook = {};
                USanAndreasInterfaceUpdateHook = safetyhook::create_mid(strref.value(), [](SafetyHookContext& regs)
                {
                    if (*gGameState == 7 && (*byte_1450605B4 & 2) != 0)
                    {
                        USanAndreasInterfaceResumeGameFromMenu((void*)regs.rcx);
                    }
                    else if (*gGameState > 7)
                    {
                        USanAndreasInterfaceUpdateHook = {};
                    }
                });
            }
        }
    }

    static float* fTimeStep = utility::GetSymbolFromFuncStart<decltype(fTimeStep)>("tank_fire", [](INSTRUX& ix) { return ix.Instruction == ND_INS_MOVSS; });
    ScriptSpace = utility::GetSymbolFromFuncStart<decltype(ScriptSpace)>("mainV1.scm", [](INSTRUX& ix) { return ix.Instruction == ND_INS_LEA && ix.Operands[0].Type == ND_OP_REG && ix.Operands[1].Type == ND_OP_MEM; });
    OnAMissionFlag = utility::GetSymbolFromFuncStart<decltype(OnAMissionFlag)>("GW_ATK", [](INSTRUX& ix) { return ix.Instruction == ND_INS_MOV && ix.Operands[0].Type == ND_OP_REG && ix.Operands[1].Type == ND_OP_MEM; });
    m_WideScreenOn = utility::GetSymbolFromFuncStart<decltype(m_WideScreenOn)>("%02d:%02d", [](INSTRUX ix) { return ix.Instruction == ND_INS_CMP && ix.Operands[0].Type == ND_OP_MEM && ix.Operands[1].Type == ND_OP_IMM; });
    static void (*CHud_SetHelpMessage)(FString* a1, int a2, char a3, char a4) = utility::GetFunctionCallAfterStringParam<decltype(CHud_SetHelpMessage)>("GW_ATK", 1);
    static void(*SaveToSlot)(int, int) = nullptr;

    auto s = utility::find_function_from_string_ref(utility::get_executable(), "%s%i_%s.%s", true);
    if (s)
    {
        auto xrefs = utility::scan_displacement_references(utility::get_executable(), s.value());
        for (auto& it : xrefs)
        {
            const auto decoded = utility::decode_one((uint8_t*)it + 4);

            if (decoded)
            {
                if (decoded->Instruction == ND_INS_ADD)
                {
                    auto func = utility::find_function_start(it + 4);
                    if (func)
                    {
                        SaveToSlot = reinterpret_cast<decltype(SaveToSlot)>(func.value());
                        break;
                    }
                }
            }
        }
    }

    static void* DoGameSpecificStuffBeforeSaveTime = nullptr;
    static void* ConfirmSave = utility::GetSymbolFromFuncStart<decltype(ConfirmSave)>("FES_SSC", [](INSTRUX& ix) { return ix.Instruction == ND_INS_CALLNR; });
    if (ConfirmSave)
    {
        INSTRUX ix{};
        auto ip = (uintptr_t)ConfirmSave;

        while (true)
        {
            const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

            if (!ND_SUCCESS(status)) {
                break;
            }

            if (ix.Instruction == ND_INS_MOV && ix.Operands[1].Type == ND_OP_IMM && ix.Operands[1].Info.Immediate.Imm == 360)
            {
                DoGameSpecificStuffBeforeSaveTime = reinterpret_cast<decltype(DoGameSpecificStuffBeforeSaveTime)>(ip + ix.Imm1Offset);
                break;
            }

            ip += ix.Length;
        }
    }

    if (DoGameSpecificStuffBeforeSaveTime)
    {
        auto COnscreenTimerProcessForDisplay = utility::find_function_from_string_ref(utility::get_executable(), "%02d:%02d", true);

        if (COnscreenTimerProcessForDisplay)
        {
            static auto Render2dStuffHook = safetyhook::create_mid(COnscreenTimerProcessForDisplay.value(), [](SafetyHookContext& regs)
            {
                static bool bF5LastState = false;
                bool bF5CurState = GetKeyState(VK_F5) & 0x8000;
                
                if (!bF5LastState && bF5CurState)
                {
                    if (!IsPlayerOnAMission() && !*m_WideScreenOn)
                    {
                        injector::WriteMemory<uint32_t>(DoGameSpecificStuffBeforeSaveTime, 0, true);
                        SaveToSlot(0x708, nIniSaveSlot);
                        injector::WriteMemory<uint32_t>(DoGameSpecificStuffBeforeSaveTime, 360, true);
                
                        FString test;
                        test.Data = const_cast<wchar_t*>(L"FESZ_WR");
                        test.Max = *test.Data ? int(std::wcslen(test.Data)) + 1 : 0;
                        test.Count = test.Max;
                
                        CHud_SetHelpMessage(&test, test.Count, 0, 0);
                    }
                }
                
                bF5LastState = bF5CurState;
            });
        }
    }

    //if (fHudScale != 1.0f || fRadarScale != 1.0f)
    {
        auto str = utility::scan_string(utility::get_executable(), "%s %f %f %f %f %s %s", true);
        if (str)
        {
            auto strref = utility::scan_reference(utility::get_executable(), str.value());

            if (strref)
            {
                auto start = utility::find_function_start_with_call(strref.value());
                if (start)
                {
                    shParseWidgetPositionLineFromFile = safetyhook::create_inline(start.value(), ParseWidgetPositionLineFromFile);

                    INSTRUX ix{};
                    auto ip = strref.value();

                    while (true)
                    {
                        const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 100, ND_CODE_64, ND_DATA_64);

                        if (!ND_SUCCESS(status)) {
                            break;
                        }

                        if (ix.Instruction == ND_INS_CALLNR)
                        {
                            injector::MakeCALLTrampoline(ip, vscanf_hook, true);
                            break;
                        }

                        ip += ix.Length;
                    }
                }
            }
        }
    }

    if (bImproveCameraPC)
    {
        uintptr_t CCameraProcess = 0;
        uintptr_t CCamProcess = 0;

        auto pGetRawInputData = utility::scan_ptr(utility::get_executable(), (uintptr_t)GetRawInputData);
        if (pGetRawInputData)
        {
            auto pGetRawInputDataRef = utility::scan_displacement_reference(utility::get_executable(), pGetRawInputData.value());
            if (pGetRawInputDataRef)
            {
                auto pWndProc = utility::find_function_start_with_call(pGetRawInputDataRef.value());
                if (pWndProc)
                {
                    shWndProc = safetyhook::create_inline(pWndProc.value(), CustomWndProc);
                }
            }
        }

        auto str = utility::scan_string(utility::get_executable(), "DATA\\GTA.DAT", true);
        if (str)
        {
            auto strref = utility::scan_reference(utility::get_executable(), str.value());

            if (strref)
            {
                INSTRUX ix{};
                auto ip = strref.value();
                bool bCheckHere = false;
                int callCount = 0;

                while (true)
                {
                    const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

                    if (!ND_SUCCESS(status)) {
                        break;
                    }

                    if (ix.Instruction == ND_INS_LEA && utility::resolve_displacement(ip).value_or(-1) == (uintptr_t)ScriptSpace)
                    {
                        bCheckHere = true;
                    }

                    if (bCheckHere && ix.Instruction == ND_INS_CALLNR)
                    {
                        if (callCount == 2)
                        {
                            auto disp = utility::resolve_displacement(ip);
                            if (disp)
                            {
                                CCameraProcess = disp.value();
                                break;
                            }
                        }

                        callCount++;
                    }

                    ip += ix.Length;
                }
            }
        }

        INSTRUX ix{};
        auto ip = CCameraProcess;
        auto nextCall = false;

        while (true)
        {
            const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

            if (!ND_SUCCESS(status)) {
                break;
            }

            if (!nextCall)
            {
                if (ix.Instruction == ND_INS_LEA && ix.DispLength == 4)
                {
                    if (injector::ReadMemory<uint32_t>(ip + ix.DispOffset, true) == 0x188)
                    {
                        nextCall = true;
                    }
                }
            }
            else if (ix.Instruction == ND_INS_CALLNR)
            {
                auto disp = utility::resolve_displacement(ip);
                if (disp)
                {
                    CCamProcess = disp.value();
                    break;
                }
            }

            ip += ix.Length;
        }

        static uintptr_t shProcess_FollowCar_SA = 0;
        static auto CCameraProcessHook = safetyhook::create_mid(CCameraProcess, [](SafetyHookContext& regs)
        {
            TheCamera = regs.rcx;
            LockCursor();
        });

        if (CCamProcess)
        {
            INSTRUX ix{};
            auto ip = CCamProcess;
            auto caseCount = 0;
            auto nextCall = false;

            while (true)
            {
                const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);
            
                if (!ND_SUCCESS(status)) {
                    break;
                }
            
                if (ix.Instruction == ND_INS_MOV && ix.Operands[0].Type == ND_OP_REG && ix.Operands[0].Info.Register.Reg == 1 && ix.Operands[1].Type == ND_OP_REG && ix.Operands[1].Info.Register.Reg == 7)
                {
                    caseCount++;
                    nextCall = true;
                }
                else if (nextCall && ix.Instruction == ND_INS_CALLNR)
                {
                    if (caseCount == 1) //cases 3,18,22 CCam::Process_FollowCar_SA
                    {
                        auto disp = utility::resolve_displacement(ip);
                        if (disp)
                            shProcess_FollowCar_SA = disp.value();
                    }
                    else if (caseCount == 2) //case 4 CCam::Process_FollowPed_SA
                    {
                        auto disp = utility::resolve_displacement(ip);
                        if (disp)
                            shProcess_FollowPed_SA = safetyhook::create_inline(disp.value(), Process_FollowPed_SA);
                    }
                    else if (caseCount == 6) //cases 53,55,65 CCam::Process_AimWeapon
                    {
                        auto disp = utility::resolve_displacement(ip);
                        if (disp)
                        {
                            shProcess_AimWeapon = safetyhook::create_inline(disp.value(), Process_AimWeapon);
                            break;
                        }
                    }
                    nextCall = false;
                }
            
                ip += ix.Length;
            }
        }

        static int* nPedZoom = utility::GetSymbolFromFuncStart<decltype(nPedZoom)>(shProcess_FollowPed_SA.target_address(), [](INSTRUX ix) { return ix.Instruction == ND_INS_CMP && ix.Operands[0].Type == ND_OP_MEM && ix.Operands[1].Type == ND_OP_IMM && ix.Operands[1].Info.Immediate.Imm == 3; });
        if (nPedZoom)
        {
            INSTRUX ix{};
            auto ip = shProcess_AimWeapon.target_address();
            auto movssCount = 0;

            while (true)
            {
                const auto status = NdDecodeEx(&ix, (ND_UINT8*)ip, 1000, ND_CODE_64, ND_DATA_64);

                if (!ND_SUCCESS(status)) {
                    break;
                }

                if (ix.Instruction == ND_INS_CMP && ix.HasDisp && ix.Operands[0].Type == ND_OP_MEM && ix.Operands[1].Type == ND_OP_REG && ix.Operands[1].Info.Register.Reg == 13)
                {
                    static auto AimCamSetFOVHook = safetyhook::create_mid(ip, [](SafetyHookContext& regs)
                    {
                        if (bIsFollowPed)
                        {
                            regs.r15 = 1;

                            switch (*nPedZoom)
                            {
                            case 0:
                                regs.xmm0.f32[0] = 60.0f;
                                break;
                            case 1:
                                regs.xmm0.f32[0] = 70.0f;
                                break;
                            case 2:
                                regs.xmm0.f32[0] = 80.0f;
                                break;
                            case 3:
                                regs.xmm0.f32[0] = 90.0f;
                                break;
                            default:
                                regs.xmm0.f32[0] = 70.0f;
                                break;
                            }
                        }
                    });
                }
                else if (ix.Instruction == ND_INS_MOVSS && ix.DispLength == 4)
                {
                    if (injector::ReadMemory<uint32_t>(ip + ix.DispOffset, true) == 0x788)
                    {
                        if (movssCount == 1)
                        {
                            injector::MakeNOP(ip, 8, true);
                            static auto Heading = safetyhook::create_mid(ip, [](SafetyHookContext& regs)
                            {
                                if (!bIsFollowPed)
                                    *(float*)(regs.rbx + 0x788) = regs.xmm0.f32[0];
                            });
                            break;
                        }
                        movssCount++;
                    }
                }

                ip += ix.Length;
            }
        }

        //Process_FollowCar_SA
        auto pattern = hook::pattern("F3 0F 11 4E ? 44 0F 28 C1");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 5, true);

        pattern = hook::pattern("F3 44 0F 11 46 ? EB");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 6, true);

        pattern = hook::pattern("44 89 76 ? 45 0F 28 C5");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 4, true);

        pattern = hook::pattern("F3 41 0F 59 BC 1F ? ? ? ? E8");
        if (!pattern.empty())
            injector::MakeNOP(pattern.get_first(), 10, true);

        constexpr float fThreshold = 0.00f; // originally 0.01
        static float fTimer = 0.0f;
        static float fAimWeaponUpDown = 0.0f;
        pattern = hook::pattern("F3 0F 59 3D ? ? ? ? 41 0F 2F FD");
        if (!pattern.empty())
        {
            static auto FollowCarHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                fAimWeaponUpDown = regs.xmm0.f32[0];
                constexpr float timeScale = 0.02f;

                float nextTimer = fabs(fAimWeaponUpDown) > fThreshold ? fCenteringDelay : fTimer;

                if (fTimer >= 0.0f)
                {
                    fTimer = fmaxf(nextTimer - *fTimeStep * timeScale, 0.0f);
                }
            });

            pattern = hook::pattern("F3 0F 11 46 ? 76 ? F3 44 0F 59 05");
            if (!pattern.empty())
            {
                injector::MakeNOP(pattern.get_first(), 5, true);
                static auto FollowCarHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    *(float*)(regs.rsi + 0x4) = fAimWeaponUpDown * 2.0f; // idk why 2.0, whatever
                });

                // x axis delay
                pattern = hook::pattern("44 0F 29 58 ? 45 0F 57 DB 44 0F 29 60 ? 48 89 58");
                static auto ComputeCarBetaAngleHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    regs.xmm10.f32[0] = 1.0f * fCenteringDelay;
                });

                // y axis delay
                pattern = hook::pattern("F3 0F 10 46 ? 0F 57 C9 F3 41 0F 10 CA");
                static auto Alpha = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (fTimer <= 0.0f || fabs(fAimWeaponUpDown) > fThreshold)
                    {

                    }
                    else
                    {
                        *(float*)regs.rsi = regs.xmm1.f32[0];
                        regs.xmm10.f32[0] = regs.xmm1.f32[0];
                    }
                });

                pattern = hook::pattern("F3 0F 11 36 F3 44 0F 58 F0");
                injector::MakeNOP(pattern.get_first(), 4, true);
                static auto targetAlphaBlendAmount = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (fTimer <= 0.0f || fabs(fAimWeaponUpDown) > fThreshold)
                        *(float*)regs.rsi = regs.xmm6.f32[0];
                    else
                        regs.xmm14.f32[0] = *(float*)regs.rsi;
                });
            }
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init);
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
