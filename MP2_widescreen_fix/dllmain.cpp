#include "stdafx.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HMODULE h_e2mfc_dll = NULL;
HMODULE comics = NULL;

float iniFOV, FOV;
float fAspectRatio, stdViewPortSize, fWidthFactor, fDiffFactor;
float fVisibilityFactor1, fVisibilityFactor2;
unsigned int ScreenWidth, ScreenHeight;
DWORD jmpAddress, jmpFOV;
DWORD _EAX, _EBX, _nEDX; float _EDX;
bool bComicsMode;

int __fastcall PDriverGetWidth(int a1)
{
	int result; // eax@2

	if (*(DWORD *)(a1 + 48))
		result = *(DWORD *)(a1 + 52);
	else
		result = *(DWORD *)(a1 + 4);

	ScreenWidth = result;
	return result;
}

int __fastcall PDriverGetHeight(int a1)
{
	int result; // eax@2

	if (*(DWORD *)(a1 + 48))
		result = *(DWORD *)(a1 + 56);
	else
		result = *(DWORD *)(a1 + 8);

	ScreenHeight = result;
	return result;
}

void __declspec(naked)PatchFOV()
{
	__asm mov ebx, FOV
	__asm push    ebx //FOV value
	__asm mov     ecx, esi
	__asm call    ebp
	__asm jmp jmpFOV
}

void __fastcall P_CameraSetFOV(DWORD P_Camera, float FOVvalue)
{
	FOVvalue *= iniFOV;

	if (*(float *)&FOVvalue != *(float *)(P_Camera + 284))
	{
		if (*(float *)&FOVvalue >= 0.0)
		{
		
			if (FOVvalue <= 3.1241393f)
			{
				*(BYTE *)(P_Camera + 236) |= 0x40u;
				*(float *)(P_Camera + 284) = FOVvalue;
			}
		}
		else
		{
			*(BYTE *)(P_Camera + 236) |= 0x40u;
			*(DWORD *)(P_Camera + 284) = 0;
		}
	}
}


void __declspec(naked)ForceEntireViewport()
{
	__asm mov ax, 1
	__asm ret
}

void __declspec(naked)DisableSubViewport()
{
	__asm ret 10h
}

void __declspec(naked)PCameraValidateHook()
{
    __asm mov _EAX, eax
    __asm mov _EBX, ebx
    __asm mov     edx, [esp + 4]
    __asm mov _EDX, edx
    __asm mov _nEDX, edx

    if (!fDiffFactor)
    {
        fDiffFactor = fWidthFactor / _EDX;
    }
    else
    {
        if (_EDX >= 0.7f && _EDX <= 0.8f)
        {
            if ((_EBX == 1 && _EAX == 8) || (_EBX == 0 && _EAX != 8))
            {
                _EDX *= fDiffFactor;
                __asm mov edx, _EDX
            }
        }
        else
        {
            if (_EAX == 8 && _EDX < 0.7f)
            {
                _EDX *= fDiffFactor;
                if (_EBX == 1 && _EAX == 8 && _nEDX != 0x3ED413CD) //0.414213568, it's to avoid comics stretching
                    __asm mov edx, _EDX
            }
            else
            {
                if ((_EDX > 0.8f && _EDX <= 1.5f) && _nEDX != 0x3F800000)
                {
                    _EDX *= fDiffFactor;
                    if ((_EBX == 1 && _EAX == 8) || (_EBX == 0 && _EAX != 8))
                        __asm mov edx, _EDX
                }
            }
        }
    }
    __asm mov[esi + 20Ch], edx
    __asm jmp jmpAddress
}

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("");
	iniFOV = iniReader.ReadFloat("MAIN", "FOV", 0.0f);

	do
	{
		Sleep(100);
		h_e2mfc_dll = GetModuleHandle("e2mfc.dll");
	} while (h_e2mfc_dll == NULL);

	CPatch::RedirectCall((DWORD)h_e2mfc_dll + 0x176AF, PDriverGetWidth);
	CPatch::RedirectCall((DWORD)h_e2mfc_dll + 0x176F4, PDriverGetHeight);

	CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x156A0, DisableSubViewport);
	CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x14C80, ForceEntireViewport);

	do
	{
		Sleep(0);
	} while (ScreenWidth == 0 || ScreenHeight == 0);

	/*if (iniFOV)
	{
		//FOV = iniFOV;
		//jmpFOV = 0x428F86;
		//CPatch::RedirectJump(0x428F81, PatchFOV); //fov hack
		//CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x14E70, P_CameraSetFOV);
		//CPatch::SetPointer(0x428F7B, P_CameraSetFOV);
	}*/

	fAspectRatio = static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight);

	//fWidthFactor = fAspectRatio / (16.0f / 9.0f);
    fWidthFactor = 0.7673270702f / ((4.0f / 3.0f) / fAspectRatio);

	if (fAspectRatio >= 1.4f)
	{
		jmpAddress = (DWORD)h_e2mfc_dll + 0x15041;
		CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x15037, PCameraValidateHook);

		//object disappearance fix
		CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x15B87 + 0x6, 0.0f);

		stdViewPortSize = 640.0f;
		CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x176D4 + 0x2, &stdViewPortSize);

		DWORD nComicsCheck = (DWORD)h_e2mfc_dll + 0x61183;

		//doors graphics fix
		fVisibilityFactor1 = 0.5f;
		fVisibilityFactor2 = 1.5f;
		//CPatch::SetPointer((DWORD)GetModuleHandle("X_GameObjectsMFC.dll") + 0xA8D0 + 0x4F5 + 0x2, &fVisibilityFactor4);
		CPatch::SetPointer((DWORD)GetModuleHandle("X_GameObjectsMFC.dll") + 0xA8D0 + 0x4CE + 0x2, &fVisibilityFactor1);
		//CPatch::SetPointer((DWORD)GetModuleHandle("X_GameObjectsMFC.dll") + 0xA8D0 + 0x4AC + 0x2, &fVisibilityFactor3);
		CPatch::SetPointer((DWORD)GetModuleHandle("X_GameObjectsMFC.dll") + 0xA8D0 + 0x485 + 0x2, &fVisibilityFactor2);

		//CPatch::SetPointer((DWORD)GetModuleHandle("X_GameObjectsMFC.dll") + 0x101F39 + 0x2, &fVisibilityFactor1);
		//CPatch::SetPointer((DWORD)GetModuleHandle("X_GameObjectsMFC.dll") + 0x101F67 + 0x2, &fVisibilityFactor2);

		if (iniReader.ReadInteger("MAIN", "COMICS_MODE", 0))
			bComicsMode = !bComicsMode;

		while (true)
		{
			Sleep(0);

			if ((GetAsyncKeyState(VK_F2) & 1) && ((unsigned char)*(DWORD*)nComicsCheck == 0xAE))
			{
				bComicsMode = !bComicsMode;
				iniReader.WriteInteger("MAIN", "COMICS_MODE", bComicsMode);
				while ((GetAsyncKeyState(VK_F2) & 0x8000) > 0) { Sleep(0); }
			}

			if (bComicsMode)
			{
				Sleep(1);
				if ((unsigned char)*(DWORD*)nComicsCheck == 0xAE)
				{
					stdViewPortSize = 480.0f * (fAspectRatio);
					CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x434F4, 480.0f); //480.0f
				}
				else
				{
					stdViewPortSize = 640.0f;
					CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x434F4, 480.0f); //480.0f
				}
			}
			else
			{
				Sleep(1);
				if ((unsigned char)*(DWORD*)nComicsCheck == 0xAE)
				{
					stdViewPortSize = (480.0f * (fAspectRatio) / 1.17936117936f);
					CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x434F4, 480.0f / 1.17936117936f); //480.0f
				}
				else
				{
					stdViewPortSize = 640.0f;
					CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x434F4, 480.0f); //480.0f
				}
			}
		}

	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}