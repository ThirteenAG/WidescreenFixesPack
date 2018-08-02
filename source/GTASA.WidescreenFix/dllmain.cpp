#include "stdafx.h"
#include "GTA\common.h"
#include "GTA\global.h"

float** pfWideScreenWidthScaleDown;
float** pfWideScreenHeightScaleDown;
float fSubtitlesScaleX, fSubtitlesScaleY;
float fCustomRadarWidthScaleDown;

void InstallWSHPSFixes()
{
    //crash fixes
    injector::WriteMemory<uint8_t>(0x57F884 + 1, 0x08, true);
    injector::WriteMemory<uint8_t>(0x57FAE9, 0x14, true);
    injector::WriteMemory(0x57FAF3, 0x456A006A, true);
    injector::WriteMemory<uint8_t>(0x57FAF7, 0x8B, true);
    injector::WriteMemory<short>(0x57FD02, (short)0xFF68, true);

    //high speed bug
    injector::WriteMemory<uint8_t>(0x574252, 0x3B, true);
    injector::WriteMemory(0x574253, 0xD90875C7, true);

    //Frame limiter bug
    injector::WriteMemory<uint8_t>(0x58BB87, 0x0C, true);
    injector::WriteMemory<uint8_t>(0x58BB88, 0x8A, true);

    //Heat Haze bug
    injector::WriteMemory(0x0070149D, 0x00859520, true);
    injector::WriteMemory(0x007014C5, 0x00859524, true);

    //Stats + - bug
    static float f65 = 65.0f - 10.0f;
    injector::WriteMemory(0x0058C004, &f65, true);
}

// COMMAND_GET_HEIGHT_PERCENT_IN_WORLD_LEVEL
float getHeightPercentInWorldLevel(float fPosZ) {
    float fHeightPerc;

    if (fPosZ <= -100.0f) {
        fHeightPerc = 100.0f;
    }
    else if (fPosZ <= 0.0f) {
        fHeightPerc = -fPosZ;
    }
    else if (fPosZ <= 200.0f) {
        fHeightPerc = fPosZ * (100.0f / 200.0f);
    }
    else if (fPosZ <= 950.0f) {
        fHeightPerc = (fPosZ - 200.0f) * (100.0f / (950.0f - 200.0f));
    }
    else if (fPosZ < 1500.0f) {
        fHeightPerc = (fPosZ - 950.0f) * (100.0f / (1500.0f - 950.0f));
    }
    else {
        fHeightPerc = 100.0f;
    }

    return fHeightPerc;
}

void __declspec(naked) HOOK_PTR_0058A68F_drawMap() {
    static const float fPosX = 15.0f;
    static const float fPosY = 28.0f;
    static const float fWidth = 20.0f;
    static const float fHeight = 2.0f;
    static const float fScrollHeight = 74.0f;
    static const float fPercent = 0.01f;

    __asm {
        fild	ds : [0x00C17040 + 0x8]
        fld		st
        fmul	fWideScreenHeightScaleDown
        mov		ecx, [eax + 8]
        push	ecx
        call	getHeightPercentInWorldLevel
        add		esp, 4
        fmul	fPercent
        fmul	fScrollHeight
        fadd	fPosY
        fmul	st, st(1)
        fsubp	st(2), st
        fmul	fHeight
        fsubr	st, st(1)
        fstp	dword ptr[esp + 34h - 14h]
        fstp	dword ptr[esp + 34h - 0x1C]
        fild	ds : [0x00C17040 + 0x4]
        fmul	fCustomWideScreenWidthScaleDown
        fld		fPosX
        fadd	fWidth
        fmul	st, st(1)
        fxch	st(1)
        fmul	fPosX
        mov		edx, 0x58A73B
        jmp		edx
    }
}

void WINAPI InstallCustomHooks() {
    // Rampage (Kill Frenzy) counter.
    injector::WriteMemory(0x0043CF49, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0043CF59, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x0043D10B, &fCustomWideScreenWidthScaleDown, true);							// Time X position scale.
    injector::WriteMemory(0x0043D163, &fCustomWideScreenWidthScaleDown, true);							// Kills X position scale.
    // Collectable text.
    injector::WriteMemory(0x004477B9, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x004477CF, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x004477F9, &fCustomWideScreenWidthScaleDown, true);							// Text centre size scale.
    injector::WriteMemory(0x004478AE, &fCustomWideScreenHeightScaleDown, true);							// 2-numbered text Y position scale.
    injector::WriteMemory(0x00447918, &fCustomWideScreenHeightScaleDown, true);							// 1-numbered text Y position scale.
    injector::WriteMemory(0x0044795D, &fCustomWideScreenHeightScaleDown, true);							// 0-numbered text Y position scale.
    // Replay text.
    injector::WriteMemory(0x0045C241, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0045C257, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    // Radar map zoomed coordinates.
    injector::WriteMemory(0x005834BC, &fCustomRadarWidthScaleDown, true);	                			// Map width scale.
    //injector::WriteMemory(0x005834EE, &fCustomWideScreenHeightScaleDown, true);						// Map height scale.
    // Radar blip.
    injector::WriteMemory(0x0058410D, &fCustomWideScreenWidthScaleDown, true);							// Squared radar blip border width scale.
    injector::WriteMemory(0x0058412D, &fCustomWideScreenHeightScaleDown, true);							// Squared radar blip border height scale.
    injector::WriteMemory(0x00584192, &fCustomWideScreenWidthScaleDown, true);							// Squared radar blip width scale.
    injector::WriteMemory(0x005841B2, &fCustomWideScreenHeightScaleDown, true);							// Squared radar blip height scale.
    injector::WriteMemory(0x00584209, &fCustomWideScreenHeightScaleDown, true);							// Fipped triangular radar blip border height scale.
    injector::WriteMemory(0x0058424B, &fCustomWideScreenWidthScaleDown, true);							// Fipped triangular radar blip border width scale.
    injector::WriteMemory(0x005842C8, &fCustomWideScreenHeightScaleDown, true);							// Fipped triangular radar blip height scale.
    injector::WriteMemory(0x005842E8, &fCustomWideScreenWidthScaleDown, true);							// Fipped triangular radar blip width scale.
    injector::WriteMemory(0x00584348, &fCustomWideScreenHeightScaleDown, true);							// Triangular radar blip border height scale.
    injector::WriteMemory(0x0058439E, &fCustomWideScreenWidthScaleDown, true);							// Triangular radar blip border width scale.
    injector::WriteMemory(0x0058440E, &fCustomWideScreenHeightScaleDown, true);							// Triangular radar blip height scale.
    injector::WriteMemory(0x00584436, &fCustomWideScreenWidthScaleDown, true);							// Triangular radar blip width scale.
    // Radar position.
    injector::WriteMemory(0x005849FC, &fCustomWideScreenHeightScaleDown, true);							// Sprite height scale.
    injector::WriteMemory(0x00584A14, &fCustomWideScreenWidthScaleDown, true);							// Sprite width scale.
    // Radar sprite.
    injector::WriteMemory(0x00586041, &fCustomWideScreenWidthScaleDown, true);							// Sprite width scale.
    injector::WriteMemory(0x0058605A, &fCustomWideScreenHeightScaleDown, true);							// Sprite height scale.
    // Radar blip for entity.
    injector::WriteMemory(0x005876BE, &fCustomWideScreenHeightScaleDown, true);							// Radar centre sprite height scale.
    injector::WriteMemory(0x005876D6, &fCustomWideScreenWidthScaleDown, true);							// Radar centre sprite width scale.
    injector::WriteMemory(0x00587735, &fCustomWideScreenHeightScaleDown, true);							// Radar centre sprite height scale on camera locked behind the player.
    injector::WriteMemory(0x0058774D, &fCustomWideScreenWidthScaleDown, true);							// Radar centre sprite width scale on camera locked behind the player.
    injector::WriteMemory(0x0058780C, &fCustomWideScreenWidthScaleDown, true);							// Radar searchlight sprite X position scale.
    injector::WriteMemory(0x00587891, &fCustomWideScreenWidthScaleDown, true);							// Radar searchlight sprite width scale.
    injector::WriteMemory(0x00587918, &fCustomWideScreenHeightScaleDown, true);							// Radar moving light sprite height scale.
    injector::WriteMemory(0x00587930, &fCustomWideScreenWidthScaleDown, true);							// Radar moving light sprite width scale.
    injector::WriteMemory(0x00587A04, &fCustomWideScreenHeightScaleDown, true);							// Radar runway lights sprite height scale in menu.
    injector::WriteMemory(0x00587A1C, &fCustomWideScreenWidthScaleDown, true);							// Radar runway lights sprite width scale in menu.
    injector::WriteMemory(0x00587A94, &fCustomWideScreenHeightScaleDown, true);							// Radar runway lights sprite height scale.
    injector::WriteMemory(0x00587AAC, &fCustomWideScreenWidthScaleDown, true);							// Radar runway lights sprite width scale.
    // HUD component Y position computation.
    injector::WriteMemory(0x00588B9E, &fCustomWideScreenHeightScaleDown, true);							// Component relative Y position scale.
    // Armour meter.
    injector::WriteMemory(0x00589140, &fCustomWideScreenHeightScaleDown, true);							// Meter height scale.
    injector::WriteMemory(0x00589157, &fCustomWideScreenWidthScaleDown, true);							// Meter width scale.
    // Breath meter.
    injector::WriteMemory(0x00589218, &fCustomWideScreenHeightScaleDown, true);							// Meter height scale.
    injector::WriteMemory(0x0058922F, &fCustomWideScreenWidthScaleDown, true);							// Meter width scale.
    // Health meter.
    injector::WriteMemory(0x005892CC, &fCustomWideScreenWidthScaleDown, true);							// Meter width scale.
    injector::WriteMemory(0x00589348, &fCustomWideScreenHeightScaleDown, true);							// Meter height scale.
    injector::WriteMemory(0x00589380, &fCustomWideScreenWidthScaleDown, true);							// Meter X position scale.
    // Weapon ammo.
    injector::WriteMemory(0x005894B1, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x005894C7, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    // Statistics.
    injector::WriteMemory(0x00589705, &fCustomWideScreenWidthScaleDown, true);							// Texts and meters X position scale.
    injector::WriteMemory(0x00589737, &fCustomWideScreenHeightScaleDown, true);							// Respect meter relative Y position scale.
    injector::WriteMemory(0x0058978D, &fCustomWideScreenHeightScaleDown, true);							// 5-meters box height scale.
    injector::WriteMemory(0x005897C5, &fCustomWideScreenWidthScaleDown, true);							// 5-meters box width scale.
    injector::WriteMemory(0x00589815, &fCustomWideScreenHeightScaleDown, true);							// 5-meters box content relative Y position scale.
    injector::WriteMemory(0x00589841, &fCustomWideScreenHeightScaleDown, true);							// 6-meters box height scale.
    injector::WriteMemory(0x0058986F, &fCustomWideScreenWidthScaleDown, true);							// 6-meters box width scale.
    injector::WriteMemory(0x005898BF, &fCustomWideScreenHeightScaleDown, true);							// 6-meters box content relative Y position scale.
    injector::WriteMemory(0x005898F8, &fCustomWideScreenHeightScaleDown, true);							// Skill text height scale.
    injector::WriteMemory(0x0058990E, &fCustomWideScreenWidthScaleDown, true);							// Skill text width scale.
    injector::WriteMemory(0x00589A01, &fCustomWideScreenHeightScaleDown, true);							// Respect text and meter height scale.
    injector::WriteMemory(0x00589A18, &fCustomWideScreenWidthScaleDown, true);							// Respect text and meter width scale.
    injector::WriteMemory(0x00589A01, &fCustomWideScreenHeightScaleDown, true);							// Lung capacity/Weapon skill text and meter relative Y position scale.
    injector::WriteMemory(0x00589B18, &fCustomWideScreenHeightScaleDown, true);							// Lung capacity text and meter height scale.
    injector::WriteMemory(0x00589B2F, &fCustomWideScreenWidthScaleDown, true);							// Lung capacity text and meter width scale.
    injector::WriteMemory(0x00589C5E, &fCustomWideScreenHeightScaleDown, true);							// Weapon skill text and meter height scale.
    injector::WriteMemory(0x00589C75, &fCustomWideScreenWidthScaleDown, true);							// Weapon skill text and meter width scale.
    injector::WriteMemory(0x00589CAA, &fCustomWideScreenHeightScaleDown, true);							// Stamina text and meter relative Y position scale.
    injector::WriteMemory(0x00589D4C, &fCustomWideScreenHeightScaleDown, true);							// Stamina text and meter height scale.
    injector::WriteMemory(0x00589D63, &fCustomWideScreenWidthScaleDown, true);							// Stamina text and meter width scale.
    injector::WriteMemory(0x00589D94, &fCustomWideScreenHeightScaleDown, true);							// Muscle text and meter relative Y position scale.
    injector::WriteMemory(0x00589E34, &fCustomWideScreenHeightScaleDown, true);							// Muscle text and meter height scale.
    injector::WriteMemory(0x00589E4B, &fCustomWideScreenWidthScaleDown, true);							// Muscle text and meter width scale.
    injector::WriteMemory(0x00589E7C, &fCustomWideScreenHeightScaleDown, true);							// Fat text and meter relative Y position scale.
    injector::WriteMemory(0x00589F1C, &fCustomWideScreenHeightScaleDown, true);							// Fat text and meter height scale.
    injector::WriteMemory(0x00589F33, &fCustomWideScreenWidthScaleDown, true);							// Fat text and meter width scale.
    injector::WriteMemory(0x00589F64, &fCustomWideScreenHeightScaleDown, true);							// Sex appeal text and meter relative Y position scale.
    injector::WriteMemory(0x00589FFE, &fCustomWideScreenHeightScaleDown, true);							// Sex appeal text and meter height scale.
    injector::WriteMemory(0x0058A015, &fCustomWideScreenWidthScaleDown, true);							// Sex appeal text and meter width scale.
    injector::WriteMemory(0x0058A042, &fCustomWideScreenHeightScaleDown, true);							// Weekday text relative Y position scale.
    injector::WriteMemory(0x0058A07C, &fCustomWideScreenHeightScaleDown, true);							// Weekday text height scale.
    injector::WriteMemory(0x0058A092, &fCustomWideScreenWidthScaleDown, true);							// Weekday text width scale.
    injector::WriteMemory(0x0058A136, &fCustomWideScreenWidthScaleDown, true);							// Weekday text X position scale.
    // Map.
    injector::WriteMemory(0x0058A443, &fCustomRadarWidthScaleDown, true);						        // Radar ring plane sprite X position and width scale.
    //injector::WriteMemory(0x0058A475, &fCustomWideScreenHeightScaleDown, true);						// Radar ring plane sprite Y position and height scale.
    injector::WriteMemory(0x0058A5DA, &fCustomRadarWidthScaleDown, true);						        // Altimeter vertical bar X position and width scale.
    //injector::WriteMemory(0x0058A602, &fCustomRadarWidthScaleDown, true);						        // Altimeter vertical bar Y position and height scale.
    injector::MakeJMP(0x0058A68F, HOOK_PTR_0058A68F_drawMap, true);								        // Altimeter horizontal bar position and scale.
    injector::WriteMemory(0x0058A793, &fCustomRadarWidthScaleDown, true);						        // Upper-left radar disc X position and width scale.
    //injector::WriteMemory(0x0058A7BB, &fCustomWideScreenHeightScaleDown, true);						// Upper-left radar disc Y position and height scale.
    injector::WriteMemory(0x0058A830, &fCustomRadarWidthScaleDown, true);						        // Upper-right radar disc X position and width scale.
    //injector::WriteMemory(0x0058A85C, &fCustomWideScreenHeightScaleDown, true);						// Upper-right radar disc Y position and height scale.
    injector::WriteMemory(0x0058A8E1, &fCustomRadarWidthScaleDown, true);							    // Lower-left radar disc X position and width scale.
    //injector::WriteMemory(0x0058A90B, &fCustomWideScreenHeightScaleDown, true);						// Lower-left radar disc Y position and height scale.
    injector::WriteMemory(0x0058A984, &fCustomRadarWidthScaleDown, true);						        // Lower-right radar disc X position and width scale.
    //injector::WriteMemory(0x0058A9BF, &fCustomWideScreenHeightScaleDown, true);						// Lower-right radar disc Y position and height scale.
    // Zone name.
    injector::WriteMemory(0x0058AD26, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0058AD3C, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x0058AD67, &fCustomWideScreenWidthScaleDown, true);							// Text centre X wrap scale.
    injector::WriteMemory(0x0058AE0F, &fCustomWideScreenHeightScaleDown, true);							// Text Y position scale.
    injector::WriteMemory(0x0058AE4C, &fCustomWideScreenWidthScaleDown, true);							// Text X position scale.
    // Vehicle name.
    injector::WriteMemory(0x0058B08B, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0058B0A1, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x0058B12F, &fCustomWideScreenHeightScaleDown, true);							// Text Y position scale.
    injector::WriteMemory(0x0058B141, &fCustomWideScreenWidthScaleDown, true);							// Text X position scale.
    // On-screen displays.
    injector::WriteMemory(0x0058B265, &fCustomWideScreenHeightScaleDown, true);							// Texts height scale.
    injector::WriteMemory(0x0058B275, &fCustomWideScreenWidthScaleDown, true);							// Texts width scale.
    injector::WriteMemory(0x0058B3B1, &fCustomWideScreenWidthScaleDown, true);							// Timer text X position scale.
    injector::WriteMemory(0x0058B3FE, &fCustomWideScreenWidthScaleDown, true);							// Timer value text X position scale.
    injector::WriteMemory(0x0058B538, &fCustomWideScreenHeightScaleDown, true);							// Counter relative Y position.
    injector::WriteMemory(0x0058B56C, &fCustomWideScreenWidthScaleDown, true);							// Counter value text X position scale.
    injector::WriteMemory(0x0058B5E0, &fCustomWideScreenHeightScaleDown, true);							// Counter meter relative Y position scale.
    injector::WriteMemory(0x0058B5F0, &fCustomWideScreenWidthScaleDown, true);							// Counter meter X position scale.
    injector::WriteMemory(0x0058B680, &fCustomWideScreenWidthScaleDown, true);							// Counter text X position scale.
    // Message box.
    injector::WriteMemory(0x0058B771, &fCustomWideScreenWidthScaleDown, true);							// Text X wrap scale.
    injector::WriteMemory(0x0058B7BF, &fCustomWideScreenHeightScaleDown, true);							// Text Y position scale for the line count.
    injector::WriteMemory(0x0058B7D8, &fCustomWideScreenWidthScaleDown, true);							// Text X position scale for the line count.
    injector::WriteMemory(0x0058BA4E, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0058BA64, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x0058BCB2, &fCustomWideScreenWidthScaleDown, true);							// Text width scale for the line width.
    injector::WriteMemory(0x0058BD1B, &fCustomWideScreenHeightScaleDown, true);							// Statistic text Y position scale.
    injector::WriteMemory(0x0058BD5A, &fCustomWideScreenWidthScaleDown, true);							// Statistic text X position scale.
    injector::WriteMemory(0x0058BF1E, &fCustomWideScreenHeightScaleDown, true);							// Statistic increment/decrement foreground Y position and height scale.
    injector::WriteMemory(0x0058BF80, &fCustomWideScreenWidthScaleDown, true);							// Statistic increment/decrement foreground X position and width scale.
    //injector::WriteMemory(0x0058BFCD, &fCustomWideScreenHeightScaleDown, true);						// Statistic plus/minus text Y position scale.
    //injector::WriteMemory(0x0058BFFE, &fCustomWideScreenWidthScaleDown, true);							// Statistic plus/minus text X position scale.
    injector::WriteMemory(0x0058BAC8, &fCustomWideScreenWidthScaleDown, true);							// Message text X wrap scale.
    injector::WriteMemory(0x0058BBA9, &fCustomWideScreenHeightScaleDown, true);							// Message text Y position scale.
    injector::WriteMemory(0x0058BBDD, &fCustomWideScreenWidthScaleDown, true);							// Message text X position scale.
    // Subtitle text.
    injector::WriteMemory(0x0058C361, &fCustomWideScreenWidthScaleDown, true);							// Cutscene text centre size scale.
    injector::WriteMemory(0x0058C381, &fCustomWideScreenHeightScaleDown, true);							// Cutscene text height scale.
    injector::WriteMemory(0x0058C397, &fCustomWideScreenWidthScaleDown, true);							// Cutscene text width scale.
    injector::WriteMemory(0x0058C3B6, &fCustomWideScreenHeightScaleDown, true);							// Cutscene text Y position scale.
    injector::WriteMemory(0x0058C409, &fCustomWideScreenHeightScaleDown, true);							// Positioning/Normal text height scale.
    injector::WriteMemory(0x0058C41F, &fCustomWideScreenWidthScaleDown, true);							// Positioning/Normal text width scale.
    injector::WriteMemory(0x0058C451, &fCustomWideScreenWidthScaleDown, true);							// Positioning text centre size scale.
    injector::WriteMemory(0x0058C470, &fCustomWideScreenHeightScaleDown, true);							// Positioning text Y position scale.
    injector::WriteMemory(0x0058C4C8, &fCustomWideScreenHeightScaleDown, true);							// Stat box dependent text height scale.
    injector::WriteMemory(0x0058C4DE, &fCustomWideScreenWidthScaleDown, true);							// Stat box dependent text width scale.
    injector::WriteMemory(0x0058C501, &fCustomWideScreenWidthScaleDown, true);							// Stat box dependent text centre size scale.
    injector::WriteMemory(0x0058C546, &fCustomWideScreenHeightScaleDown, true);							// Stat box dependent text Y position scale.
    injector::WriteMemory(0x0058C5D4, &fCustomWideScreenWidthScaleDown, true);							// Normal text centre size scale.
    injector::WriteMemory(0x0058C613, &fCustomWideScreenHeightScaleDown, true);							// Normal text Y position scale.
    // Mission text.
    injector::WriteMemory(0x0058C6CE, &fCustomWideScreenHeightScaleDown, true);							// Text initial relative Y position scale.
    injector::WriteMemory(0x0058C76D, &fCustomWideScreenHeightScaleDown, true);							// Monoline text relative Y position scale.
    injector::WriteMemory(0x0058C7AB, &fCustomWideScreenHeightScaleDown, true);							// Multiline text relative Y position scale.
    injector::WriteMemory(0x0058C7F2, &fCustomWideScreenHeightScaleDown, true);							// Text relative Y position scale when the title text is printed.
    injector::WriteMemory(0x0058C813, &fCustomWideScreenHeightScaleDown, true);							// Text relative Y position scale when the odd-job and reward texts (A) are printed.
    injector::WriteMemory(0x0058C84F, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0058C865, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x0058C897, &fCustomWideScreenWidthScaleDown, true);							// Text centre size scale.
    // Title text.
    injector::WriteMemory(0x0058CA8B, &fCustomWideScreenHeightScaleDown, true);							// Text initial relative Y position scale.
    injector::WriteMemory(0x0058CB0E, &fCustomWideScreenHeightScaleDown, true);							// Text relative Y position scale.
    injector::WriteMemory(0x0058CB36, &fCustomWideScreenHeightScaleDown, true);							// Text relative Y position scale when the mission text is printed.
    injector::WriteMemory(0x0058CBAD, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0058CBC3, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    // Secondary mission text.
    injector::WriteMemory(0x0058CCC7, &fCustomWideScreenHeightScaleDown, true);							// Odd-job text (B) height scale.
    injector::WriteMemory(0x0058CCDD, &fCustomWideScreenWidthScaleDown, true);							// Odd-job text (B) width scale.
    injector::WriteMemory(0x0058CD08, &fCustomWideScreenWidthScaleDown, true);							// Odd-job text (B) centre size scale.
    injector::WriteMemory(0x0058CDD2, &fCustomWideScreenHeightScaleDown, true);							// Reward text (B) height scale.
    injector::WriteMemory(0x0058CDE8, &fCustomWideScreenWidthScaleDown, true);							// Reward text (B) width scale.
    injector::WriteMemory(0x0058CE0D, &fCustomWideScreenWidthScaleDown, true);							// Reward text (B) centre size scale.
    injector::WriteMemory(0x0058CECE, &fCustomWideScreenHeightScaleDown, true);							// Odd-job text (A) height scale.
    injector::WriteMemory(0x0058CEE4, &fCustomWideScreenWidthScaleDown, true);							// Odd-job text (A) width scale.
    injector::WriteMemory(0x0058CF0F, &fCustomWideScreenWidthScaleDown, true);							// Odd-job text (A) centre size scale.
    injector::WriteMemory(0x0058D148, &fCustomWideScreenHeightScaleDown, true);							// Reward text (A) height scale.
    injector::WriteMemory(0x0058D15E, &fCustomWideScreenWidthScaleDown, true);							// Reward text (A) width scale.
    injector::WriteMemory(0x0058D189, &fCustomWideScreenWidthScaleDown, true);							// Reward text (A) centre size scale.
    // Load mission text.
    injector::WriteMemory(0x0058D2C7, &fCustomWideScreenHeightScaleDown, true);							// Text height scale.
    injector::WriteMemory(0x0058D2DD, &fCustomWideScreenWidthScaleDown, true);							// Text width scale.
    injector::WriteMemory(0x0058D449, &fCustomWideScreenHeightScaleDown, true);							// Text Y position scale.
    injector::WriteMemory(0x0058D45B, &fCustomWideScreenWidthScaleDown, true);							// Text X position scale.
    // Weapon icon.
    injector::WriteMemory(0x0058D884, &fCustomWideScreenHeightScaleDown, true);							// Gun sprite height scale.
    injector::WriteMemory(0x0058D8C5, &fCustomWideScreenWidthScaleDown, true);							// Gun sprite width scale.
    injector::WriteMemory(0x0058D92F, &fCustomWideScreenWidthScaleDown, true);							// Fist sprite width scale.
    injector::WriteMemory(0x0058D947, &fCustomWideScreenHeightScaleDown, true);							// Fist sprite height scale.
    // Wanted level.
    injector::WriteMemory(0x0058DD02, &fCustomWideScreenWidthScaleDown, true);							// Rightmost star X position scale.
    injector::WriteMemory(0x0058DD6A, &fCustomWideScreenHeightScaleDown, true);							// Stars foreground text height scale.
    injector::WriteMemory(0x0058DD80, &fCustomWideScreenWidthScaleDown, true);							// Stars foreground text width scale.
    injector::WriteMemory(0x0058DF57, &fCustomWideScreenHeightScaleDown, true);							// Stars background text height scale.
    injector::WriteMemory(0x0058DF73, &fCustomWideScreenWidthScaleDown, true);							// Stars background text width scale.
    injector::WriteMemory(0x0058DF9D, &fCustomWideScreenHeightScaleDown, true);							// Stars background Y position scale.
    injector::WriteMemory(0x0058DFE7, &fCustomWideScreenWidthScaleDown, true);							// Stars relative X position scale.
    injector::WriteMemory(0x0058DEE6, &fCustomWideScreenHeightScaleDown, true);							// Stars foreground Y position scale.
    // HUD interface.
    injector::WriteMemory(0x0058EB2B, &fCustomWideScreenHeightScaleDown, true);							// Day time text height scale.
    injector::WriteMemory(0x0058EB41, &fCustomWideScreenWidthScaleDown, true);							// Day time text width scale.
    injector::WriteMemory(0x0058EBFB, &fCustomWideScreenHeightScaleDown, true);							// Day time text Y position scale.
    injector::WriteMemory(0x0058EC0E, &fCustomWideScreenWidthScaleDown, true);							// Day time text X position scale.
    injector::WriteMemory(0x0058EE62, &fCustomWideScreenHeightScaleDown, true);							// First player health meter Y position scale.
    injector::WriteMemory(0x0058EE80, &fCustomWideScreenWidthScaleDown, true);							// First player health meter X position scale.
    injector::WriteMemory(0x0058EECA, &fCustomWideScreenHeightScaleDown, true);							// Second player health meter Y position scale.
    injector::WriteMemory(0x0058EEF6, &fCustomWideScreenWidthScaleDown, true);							// Second player health meter X position scale.
    injector::WriteMemory(0x0058EF34, &fCustomWideScreenHeightScaleDown, true);							// First player armour meter Y position scale.
    injector::WriteMemory(0x0058EF52, &fCustomWideScreenWidthScaleDown, true);							// First player armour meter X position scale.
    injector::WriteMemory(0x0058EF9B, &fCustomWideScreenHeightScaleDown, true);							// Second player armour meter Y position scale.
    injector::WriteMemory(0x0058EFC7, &fCustomWideScreenWidthScaleDown, true);							// Second player armour meter X position scale.
    injector::WriteMemory(0x0058F0FA, &fCustomWideScreenHeightScaleDown, true);							// First player breath meter Y position scale.
    injector::WriteMemory(0x0058F118, &fCustomWideScreenWidthScaleDown, true);							// First player breath meter X position scale.
    injector::WriteMemory(0x0058F16A, &fCustomWideScreenHeightScaleDown, true);							// Second player breath meter Y position scale.
    injector::WriteMemory(0x0058F196, &fCustomWideScreenWidthScaleDown, true);							// Second player breath meter X position scale.
    injector::WriteMemory(0x0058F548, &fCustomWideScreenHeightScaleDown, true);							// Money text height scale.
    injector::WriteMemory(0x0058F55E, &fCustomWideScreenWidthScaleDown, true);							// Money text width scale.
    injector::WriteMemory(0x0058F5D0, &fCustomWideScreenHeightScaleDown, true);							// Money text Y position scale.
    injector::WriteMemory(0x0058F5F6, &fCustomWideScreenWidthScaleDown, true);							// Money text X position scale.
    injector::WriteMemory(0x0058F90D, &fCustomWideScreenHeightScaleDown, true);							// First player weapon icon Y position scale.
    injector::WriteMemory(0x0058F91E, &fCustomWideScreenWidthScaleDown, true);							// First player weapon icon X position scale.
    injector::WriteMemory(0x0058F974, &fCustomWideScreenHeightScaleDown, true);							// Second player weapon icon Y position scale.
    injector::WriteMemory(0x0058F995, &fCustomWideScreenWidthScaleDown, true);							// Second player weapon icon X position scale.
    injector::WriteMemory(0x0058F9C2, &fCustomWideScreenHeightScaleDown, true);							// First player weapon ammo Y position scale.
    injector::WriteMemory(0x0058F9D2, &fCustomWideScreenWidthScaleDown, true);							// First player weapon ammo X position scale.
    injector::WriteMemory(0x0058FA4C, &fCustomWideScreenHeightScaleDown, true);							// Second player weapon ammo Y position scale.
    injector::WriteMemory(0x0058FA5F, &fCustomWideScreenWidthScaleDown, true);							// Second player weapon ammo X position scale.
    // Meter.
    /*injector::WriteMemory(0x00728866, &fCustomWideScreenHeightScaleDown, true);						// Top border height scale.
    injector::WriteMemory(0x007288AB, &fCustomWideScreenHeightScaleDown, true);							// Bottom border height scale.
    injector::WriteMemory(0x007288F7, &fCustomWideScreenWidthScaleDown, true);							// Left border width scale.
    injector::WriteMemory(0x00728943, &fCustomWideScreenWidthScaleDown, true);							// Right border width scale.*/

    injector::WriteMemory(0x0058C397, &fSubtitlesScaleX, true);								            // Cutscene text height scale.
    injector::WriteMemory(0x0058C381, &fSubtitlesScaleY, true);								            // Cutscene text width scale.
    injector::WriteMemory(0x0058C409, &fSubtitlesScaleY, true);								            // Positioning/Normal text height scale.
    injector::WriteMemory(0x0058C41F, &fSubtitlesScaleX, true);								            // Positioning/Normal text width scale.

    // Unknown 2D stuff (0x00719B40).
    injector::WriteMemory(0x00719C0F, &fCustomWideScreenWidthScaleDown);								        // CFont::RenderString
    injector::WriteMemory(0x00719C29, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719C5A, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719C70, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719D2F, &fCustomWideScreenWidthScaleDown);								        // text outline
    injector::WriteMemory(0x00719D49, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719D7E, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719D96, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719DB7, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719DD3, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719DF4, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719E10, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719E31, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719E4D, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719E71, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719E99, &fCustomWideScreenWidthScaleDown);
    injector::WriteMemory(0x00719EC1, &fCustomWideScreenHeightScaleDown);
    injector::WriteMemory(0x00719EE5, &fCustomWideScreenHeightScaleDown);
}

int(__cdecl* FindPlayerVehicleSA)(int, char);
int FindPlayerVehicle2()
{
    return FindPlayerVehicleSA(-1, 0);
}

void GetMemoryAddresses()
{
    RsGlobal = (RsGlobalType *)0x00C17040;
    CDraw::pfScreenAspectRatio = (float*)0x00C3EFA4;
    CDraw::pfScreenFieldOfView = (float*)0x008D5038;
    CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x00727B60;
    bWideScreen = (bool*)0xBA6793; BordersVar1 = (uint32_t*)0xB6F0B8; BordersVar2 = (uint32_t*)0xB6F0CC;
    FindPlayerVehicleSA = (int(__cdecl *)(int playerNum, char a2)) 0x56E0D0;
    FindPlayerVehicle = &FindPlayerVehicle2;
    bIsInCutscene = (bool*)0xB6F065;
    pfWideScreenWidthScaleDown = (float**)0x00573F95;
    pfWideScreenHeightScaleDown = (float**)0x00573F7F;
}

void OverwriteResolution()
{
    // Unlocked widescreen resolutions by Silent
    injector::WriteMemory(0x745B71, 0x9090687D, true);
    injector::WriteMemory(0x74596C, 0x9090127D, true);
    injector::MakeNOP(0x745970, 2, true);
    injector::MakeNOP(0x745B75, 2, true);
    injector::MakeNOP(0x7459E1, 2, true);

    if (!ResX || !ResY)
        std::tie(ResX, ResY) = GetDesktopRes();
    else if (ResX == -1 || ResY == -1)
        return;

    injector::WriteMemory(0x746362 + 0x1, ResX, true);
    injector::WriteMemory(0x746367 + 0x1, ResY, true);
    injector::WriteMemory(0x74636C + 0x1, 32, true);
    injector::WriteMemory<unsigned short>(0x7462AB, 0xB2E9, true); //jz      loc_746362 > jmp      loc_746362
    injector::WriteMemory(0x7462AB + 2, 0x00, true);
    injector::WriteMemory<unsigned short>(0x7462B3, 0xAAE9, true);
    injector::WriteMemory(0x7462B3 + 2, 0x00, true);
}

DWORD _EAX;
void __declspec(naked) AllowMouseMovement()
{
    _asm
    {
        mov _EAX, eax
        mov eax, dword ptr ds : [0x8D621C]
        cmp eax, 0
        jne label1
        mov eax, _EAX
        ret

        label1 :
        {mov eax, _EAX}
        {mov _EAX, 0x7453F0}
        {jmp _EAX}
    }
}

template<uintptr_t addr>
void updateScreenAspectRatioWrapper()
{
    using func_hook = injector::function_hooker<addr, void()>;
    injector::make_static_hook<func_hook>([](func_hook::func_type updateScreenAspectRatio)
    {
        updateScreenAspectRatio();

        fWideScreenHeightScaleDown = **pfWideScreenHeightScaleDown;
        fCustomWideScreenWidthScaleDown = **pfWideScreenWidthScaleDown * fHudWidthScale;
        fCustomWideScreenHeightScaleDown = **pfWideScreenHeightScaleDown * fHudHeightScale;
        fCustomRadarWidthScaleDown = **pfWideScreenWidthScaleDown * fRadarWidthScale;
        fSubtitlesScaleX = **pfWideScreenWidthScaleDown * fHudWidthScale * fSubtitlesScale;
        fSubtitlesScaleY = **pfWideScreenHeightScaleDown * fHudHeightScale * fSubtitlesScale;
    });
}

uintptr_t SetEdgeAddr = 0x719590;
void __declspec(naked) SetDropShadowPosition()
{
    _asm {
        mov eax, [esp + 4]
        cmp ReplaceTextShadowWithOutline, 2
        jae label1
        test eax, eax
        jz label1
        dec eax
        mov[esp + 4], eax

        label1 :
        jmp SetEdgeAddr
    }
}

injector::hook_back<void(__cdecl*)(void)> hbWipeLocalVariableMemoryForMissionScript;
void __cdecl WipeLocalVariableMemoryForMissionScriptHook()
{
    uint8_t* ScriptSpace = *(uint8_t**)0x5D5380;
    size_t ScriptFileSize = *(size_t*)0x468E75;
    size_t ScriptMissionSize = *(size_t*)0x489A5B;
    auto pattern = hook::range_pattern(uintptr_t(ScriptSpace + ScriptFileSize), uintptr_t(ScriptSpace + ScriptFileSize + ScriptMissionSize), "3F 03 06 CD CC 4C 3F 06 66 66 E6 3F").count_hint(3);
    for (size_t i = 0; i < pattern.size(); i++)
    {
        float x = *pattern.get(i).get<float>(3) * (**pfWideScreenWidthScaleDown / (1.0f / 640.0f));
        float y = *pattern.get(i).get<float>(8) * (**pfWideScreenHeightScaleDown / (1.0f / 480.0f));
        injector::WriteMemory(pattern.get(i).get<float*>(3), x, true);
        injector::WriteMemory(pattern.get(i).get<float*>(8), y, true);
    }

    pattern = hook::range_pattern(uintptr_t(ScriptSpace + ScriptFileSize), uintptr_t(ScriptSpace + ScriptFileSize + ScriptMissionSize), "3F 03 06 9A 99 19 3F 06 CD CC CC 3F").count_hint(1);
    if (pattern.size() == 1)
    {
        float x = *pattern.get(0).get<float>(3) * (**pfWideScreenWidthScaleDown / (1.0f / 640.0f));
        float y = *pattern.get(0).get<float>(8) * (**pfWideScreenHeightScaleDown / (1.0f / 480.0f));
        injector::WriteMemory(pattern.get(0).get<float*>(3), x, true);
        injector::WriteMemory(pattern.get(0).get<float*>(8), y, true);
    }

    return hbWipeLocalVariableMemoryForMissionScript.fun();
}

DWORD WINAPI CompatHandler(LPVOID)
{
    size_t i = 0;
    while (GetModuleHandle(L"SilentPatchSA.asi") == NULL)
    {
        Sleep(0);
        ++i;

        if (i > 100000)
            return 0;
    }

    OverwriteResolution();
    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    if (!bDelay)
    {
        CIniReader iniReader("");
        ResX = iniReader.ReadInteger("MAIN", "ResX", -1);
        ResY = iniReader.ReadInteger("MAIN", "ResY", -1);
        fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.8f);
        fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.8f);
        fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.82f);
        fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);
        bRestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1) != 0;
        fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);
        bDontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0) != 0;
        bool DisableWhiteCrosshairDot = iniReader.ReadInteger("MAIN", "DisableWhiteCrosshairDot", 1) != 0;
        szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
        nHideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);
        bSmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1) != 0;
        ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);
        bool bAltTab = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;

        OverwriteResolution();
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&CompatHandler, NULL, 0, NULL);
        GetMemoryAddresses();

        if (bAltTab)
        {
            //Windowed mode fix (from MTA sources)
            if ((GetWindowLong((HWND)RsGlobal->ps, GWL_STYLE) & WS_POPUP) == 0)
            {
                // Disable MENU AFTER alt + tab
                //0053BC72   C605 7B67BA00 01 MOV BYTE PTR DS:[BA677B],1    
                injector::WriteMemory<uint8_t>(0x53BC78, 0x00);

                // ALLOW ALT+TABBING WITHOUT PAUSING
                injector::MakeNOP(0x748A8D, 6, true);
                injector::MakeJMP(0x6194A0, AllowMouseMovement, true);
            }
        }

        if (strncmp(szForceAspectRatio.c_str(), "auto", 4) != 0)
        {
            AspectRatioWidth = std::stoi(szForceAspectRatio.c_str());
            AspectRatioHeight = std::stoi(strchr(szForceAspectRatio.c_str(), ':') + 1);
        }

        bFOVControl = iniReader.ReadInteger("MAIN", "FOVControl", 1) != 0;
        FOVControl = (uint32_t*)0x6FF41B;
        injector::WriteMemory<float>(FOVControl, 1.0f, true);

        if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.8f; fHudHeightScale = 0.8f; }
        if (!fRadarWidthScale) { fRadarWidthScale = 0.82f; }
        if (!fSubtitlesScale) { fSubtitlesScale = 1.0f; }

        if (fCarSpeedDependantFOV)
        {
            struct FOVHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    _asm fstp dword ptr ds : [00BA8314h]
                        fRadarScaling = *(float*)0xBA8314 - 180.0f;
                }
            }; injector::MakeInline<FOVHook>(0x586C6A, 0x586C70);
        }

        if (DisableWhiteCrosshairDot)
        {
            injector::MakeNOP(0x58E2DD, 5, true);
        }

        if (ReplaceTextShadowWithOutline)
        {
            //CFont::SetDropShadowPosition -> CFont::SetEdge
            injector::MakeJMP(0x719570, SetDropShadowPosition, true);
        }
    }

    if (!bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (*(uint16_t*)0x0057B79C != 0x9090) { Sleep(10); };

    if (AspectRatioWidth && AspectRatioHeight)
    {
        auto TempPtr = injector::GetBranchDestination(0x0053D694, true);
        for (size_t i = 1; i < 20; i++)
        {
            if (*(DWORD*)(TempPtr.as_int() + i) == 0x00C17044)
            {
                injector::WriteMemory(TempPtr.as_int() + i, &AspectRatioWidth, true);
            }
            if (*(DWORD*)(TempPtr.as_int() + i) == 0x00C17048)
            {
                injector::WriteMemory(TempPtr.as_int() + 8 + 4, &AspectRatioHeight, true);
                break;
            }
        }
    }

    updateScreenAspectRatioWrapper<(0x0053D695 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053D7B2 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053D967 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053E771 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053EB1A - 0x1)>();

    injector::MakeCALL(0x00460306 - 1, CDraw::SetFOV, true);
    injector::MakeCALL(0x0052C977 - 1, CDraw::SetFOV, true);
    injector::MakeCALL(0x0053BD7B - 1, CDraw::SetFOV, true);
    injector::MakeCALL(0x005BA225 - 1, CDraw::SetFOV, true);

    if (nHideAABug)
    {
        injector::MakeJMP(0x53E90E, Hide1pxAABug, true);
    }

    injector::WriteMemory<uint8_t>(0x53E2AD, 0x74, true); //Reverse g_MenuManager.widescreenOn to make widescreen off equal to borders off
    injector::WriteMemory<uint8_t>(0x58BB90, 0x74, true); //for borders and text boxes.
    if (bSmartCutsceneBorders)
    {
        injector::MakeCALL(0x53E2B4, CCamera::DrawBordersForWideScreen, true);
        injector::MakeCALL(0x5AF8C0, CCamera::DrawBordersForWideScreen, true);
        injector::WriteMemory<uint8_t>(0x58BB93, 0x00, true); //text box offset in cutscenes
    }

    InstallCustomHooks();

    InstallWSHPSFixes();

    // intro text scaling
    hbWipeLocalVariableMemoryForMissionScript.fun = injector::MakeCALL(0x489A70, WipeLocalVariableMemoryForMissionScriptHook, true).get();
    injector::MakeCALL(0x4899F0, WipeLocalVariableMemoryForMissionScriptHook, true);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}