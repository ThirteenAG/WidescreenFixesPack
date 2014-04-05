#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HMODULE h_e2mfc_dll = NULL;
HMODULE comics = NULL;

float res1 = 360.0f;
float half_res1 = 180.0f;
float door_fix = 0.0020833334f;
float height_multipl;
float mul2;
float div2;
float div_comics;
float div_comics2;

int width = 0;
int height = 0;
float aspect_ratio = 0.0f;
float FOV;
int FOV_ptr;
float shadows_fix;
DWORD jmpAddress;
float ini_FOV;



BYTE CullingImprove[6] = { 0x66, 0xB8, 0x01, 0x00, 0xC3, 0x90 };

BYTE Data[114] = { 0xB9, 0x00, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x04, 0x81, 0xFC, 0x94, 0xFC, 0x18, 0x00, 0x74, 0x43, 0x81, 0xFC,
0xD0, 0xFC, 0x08, 0x00, 0x74, 0x4E, 0x81, 0xFC, 0xCC, 0xFC, 0x08, 0x00, 0x74, 0x33, 0x81, 0xFC, 0x3C, 0xFC,
0x18, 0x00, 0x74, 0x2B, 0x81, 0xFC, 0x58, 0xFC, 0x18, 0x00, 0x74, 0x23, 0x81, 0xFC, 0xEC, 0xFC, 0x18, 0x00,
0x74, 0x1B, 0x81, 0xFC, 0x9C, 0xFC, 0x18, 0x00, 0x74, 0x13, 0xEB, 0x00, 0x8B, 0x14, 0x24, 0x89, 0x96, 0x0C,
0x02, 0x00, 0x00, 0x83, 0xEC, 0x04, 0xE9, 0xEE, 0x4F, 0x75, 0xFD, 0xD9, 0x41, 0x04, 0xD9, 0x41, 0x08, 0xDE,
0xC9, 0xD9, 0x04, 0x24, 0xDE, 0xC9, 0xD9, 0x19, 0x8B, 0x11, 0xEB, 0xDF, 0xD9, 0x04, 0x24, 0xD8, 0x49, 0x04,
0xD9, 0x19, 0x8B, 0x11, 0xEB, 0xD3 };


BYTE Data_nonsteam[56] = { 0x81, 0xFC, 0x98, 0xFC, 0x13, 0x00, 0x74, 0x43, 0x81, 0xFC, 0xD4, 0xFC, 0x03, 0x00, 0x74, 0x4E, 0x81, 0xFC, 0xD0, 0xFC, 0x13, 0x00,
0x74, 0x33, 0x81, 0xFC, 0x40, 0xFC, 0x13, 0x00, 0x74, 0x2B, 0x81, 0xFC, 0x5C, 0xFC, 0x13, 0x00, 0x74, 0x23, 0x81, 0xFC, 0xF0, 0xFC, 0x13, 0x00, 0x74,
0x1B, 0x81, 0xFC, 0xA0, 0xFC, 0x13, 0x00, 0x74, 0x13 };

BYTE Data2[4] = { 0x8C, 0x6F, 0x44, 0x3F };


char asm_patch[600];

int asm_patch_nonsteam = (DWORD)asm_patch + 126;
int asm_patch_nonsteam2 = (DWORD)asm_patch + 254;
int asm_patch_nonsteam3 = (DWORD)asm_patch + 383;


int __fastcall P_Driver__getWidth(int a1)
{
	int result; // eax@2

	if (*(DWORD *)(a1 + 48))
		result = *(DWORD *)(a1 + 52);
	else
		result = *(DWORD *)(a1 + 4);

	width = result;
	return result;
}

int __fastcall P_Driver__getHeight(int a1)
{
	int result; // eax@2

	if (*(DWORD *)(a1 + 48))
		result = *(DWORD *)(a1 + 56);
	else
		result = *(DWORD *)(a1 + 8);

	height = result;
	return result;
}


void setFOV()
{
	if (ini_FOV)
	{
		FOV = ini_FOV;
		return;
	}
		aspect_ratio = (float)width / (float)height;

		if (aspect_ratio >= 1.7777f /*&& aspect_ratio <= 1.78*/) //1.777777777777778 //16:9
		{
			FOV = FOV * 1.165f;
			return;
		}

		if (aspect_ratio > 1.5 && aspect_ratio < 1.7) //1.6 //16:10
		{
			FOV = FOV * 1.18f;
			return;
		}

		if (aspect_ratio > 1.49 && aspect_ratio < 1.51) //1.50 //3:2
		{
			FOV = FOV * 1.14f;
			return;
		}

		if (aspect_ratio >= 1.66 && aspect_ratio <= 1.67) //1.666666666666667 //5:3
		{
			FOV = FOV * 1.2f;
			return;
		}


		if (aspect_ratio >= 1.77083 && aspect_ratio <= 1.77085) //1.770833333333333
		{
			FOV = FOV * 1.27f;
			return;
		}

		if (aspect_ratio == 1.5625) //1.5625 //1600:1024
		{
			FOV = FOV * 1.14f;
			return;
		}

		if (aspect_ratio >= 1.700 && aspect_ratio <= 1.710) //1.706666666666667 //1024:600
		{
			FOV = FOV * 1.25f;
			return;
		}

		if (aspect_ratio >= 1.77864 && aspect_ratio <= 1.77869) //1.778645833333333 //1366:768
		{
			FOV = FOV * 1.27f;
			return;
		}

}



void __declspec(naked)patch_FOV()
{
	__asm mov edx, [ecx + 0x58]
	__asm mov FOV, edx
	__asm call dword ptr[setFOV]
	__asm mov edx, FOV
	__asm mov[ecx + 0x58], edx
	__asm fld dword ptr[ecx + 0x58]
	__asm ret
}


DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("");
	ini_FOV = iniReader.ReadFloat("MAIN", "FOV", 0.0f);

	CPatch::RedirectJump(0x50A17B, patch_FOV); //fov hack

	do
	{
		Sleep(100);
		h_e2mfc_dll = GetModuleHandle("e2mfc.dll");
	} while (h_e2mfc_dll == NULL);

	CPatch::RedirectCall((DWORD)h_e2mfc_dll + 0x176AF, P_Driver__getWidth);
	CPatch::RedirectCall((DWORD)h_e2mfc_dll + 0x176F4, P_Driver__getHeight);

	do
	{
		Sleep(0);
	} while (width == 0 || height == 0);

	aspect_ratio = (float)width / (float)height;

	res1 = 640.0f / aspect_ratio; // 640.0 original
	half_res1 = res1 / 2.0f; // 480.0 original

	div_comics = 1.5f; //1.0 / 480.0 / 2.0;
	div_comics2 = 2.0f; //1.0 / 480.0 / 2.0;

				if (aspect_ratio >= 1.4f) //WS
				{
					float Const133 = (4.0f / 3.0f);
					float WidthFactor = 1.0f;
					jmpAddress = (DWORD)h_e2mfc_dll + 0x15041;
					CPatch::Patch(asm_patch, &Data, 114);
					CPatch::SetUInt((DWORD)asm_patch + 0x1, (DWORD)asm_patch + 0x72);
					CPatch::SetFloat((DWORD)asm_patch + 0x72 + 0x8, Const133);
					CPatch::SetFloat((DWORD)asm_patch + 0x72 + 0x4, WidthFactor);
					CPatch::RedirectJump((DWORD)asm_patch + 0x4E, (void *)jmpAddress);
					CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x1503B, asm_patch);
					CPatch::Patch(asm_patch + 0x72, &Data2, 4);

					CPatch::SetChar((DWORD)asm_patch + 0x41, 0x44);

					CPatch::Patch((void *)asm_patch_nonsteam, &Data, 114);
					CPatch::Patch((void *)(asm_patch_nonsteam + 0x8), &Data_nonsteam, 56); //-4FFFC
					CPatch::SetUInt((DWORD)asm_patch_nonsteam + 0x1, (DWORD)asm_patch_nonsteam + 0x72);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam + 0x72 + 0x8, Const133);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam + 0x72 + 0x4, WidthFactor);
					CPatch::RedirectJump((DWORD)asm_patch_nonsteam + 0x4E, (void *)jmpAddress);
					CPatch::Patch((void *)(asm_patch_nonsteam + 0x72), &Data2, 4);
					//nop((DWORD)asm_patch_nonsteam+0x5, 3);
					//insertJmp((DWORD)asm_patch+0x4E, asm_patch_nonsteam);


					CPatch::SetChar((DWORD)asm_patch_nonsteam + 0x41, 0x46);


					CPatch::Patch((void *)asm_patch_nonsteam2, &Data, 114);
					//patch((DWORD)asm_patch_nonsteam2+0x8, &Data_nonsteam, 56); //-4FFFC
					CPatch::SetUInt((DWORD)asm_patch_nonsteam2 + 0x1, (DWORD)asm_patch_nonsteam2 + 0x72);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam2 + 0x72 + 0x8, Const133);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam2 + 0x72 + 0x4, WidthFactor);
					CPatch::RedirectJump((DWORD)asm_patch_nonsteam2 + 0x4E, (void *)jmpAddress);
					CPatch::Patch((void *)(asm_patch_nonsteam2 + 0x72), &Data2, 4);
					//nop((DWORD)asm_patch_nonsteam+0x5, 3);
					//insertJmp((DWORD)asm_patch+0x4E, asm_patch_nonsteam);


					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x41, 0x47);


					CPatch::Patch((void *)asm_patch_nonsteam3, &Data, 114);
					CPatch::Patch((void *)(asm_patch_nonsteam3 + 0x8), &Data_nonsteam, 56); //-4FFFC
					CPatch::SetUInt((DWORD)asm_patch_nonsteam3 + 0x1, (DWORD)asm_patch_nonsteam3 + 0x72);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam3 + 0x72 + 0x8, Const133);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam3 + 0x72 + 0x4, WidthFactor);
					CPatch::RedirectJump((DWORD)asm_patch_nonsteam3 + 0x4E, (void *)jmpAddress);
					CPatch::Patch((void *)(asm_patch_nonsteam3 + 0x72), Data2, 4);
					//nop((DWORD)asm_patch_nonsteam+0x5, 3);
					//insertJmp((DWORD)asm_patch+0x4E, asm_patch_nonsteam);


					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x41, 0x00);



					CPatch::Patch(h_e2mfc_dll + 0x14C80, &CullingImprove, sizeof(CullingImprove));

					//if ((DWORD)*(DWORD*)0x400040 == 0x0EBA1F0E) {
					//patch((DWORD)asm_patch_nonsteam+0x8, &Data_nonsteam, 56); //-4FFFC
					//}
					/*else
					{
					}*/


					//if() {
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x8 + 0x2, 0x90u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x10 + 0x2, 0xCCu);
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x18 + 0x2, 0xC8u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x20 + 0x2, 0x38u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x28 + 0x2, 0x54u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x30 + 0x2, 0xE8u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam2 + 0x38 + 0x2, 0x98u);

					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x8 + 0x2, 0x94u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x10 + 0x2, 0xD0u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x18 + 0x2, 0xCCu);
					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x20 + 0x2, 0x3Cu);
					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x28 + 0x2, 0x58u);
					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x30 + 0x2, 0xECu);
					CPatch::SetChar((DWORD)asm_patch_nonsteam3 + 0x38 + 0x2, 0x9Cu);


					WidthFactor = aspect_ratio / (16.0f / 9.0f);
					CPatch::SetFloat((DWORD)asm_patch + 0x72 + 0x4, WidthFactor);
					CPatch::SetFloat((DWORD)asm_patch_nonsteam + 0x72 + 0x4, WidthFactor);

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