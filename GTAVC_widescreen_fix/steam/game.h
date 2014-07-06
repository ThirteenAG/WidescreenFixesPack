#include "stdafx.h"
#define _USE_MATH_DEFINES
#include "math.h"
#pragma warning(disable:4480)
#ifndef _GAME_H_
#define _GAME_H_

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle) * (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle) * 180.0f / (float)M_PI)

#define SCREEN_RESOLUTION_MINWIDTH	640
#define SCREEN_RESOLUTION_MINHEIGHT	448

#define SCREEN_RESOLUTION_WIDTH		640.0f
#define SCREEN_RESOLUTION_HEIGHT	448.0f

#define SCREEN_SCALE_WIDTH			(1.0f / SCREEN_RESOLUTION_WIDTH)
#define SCREEN_SCALE_HEIGHT			(1.0f / SCREEN_RESOLUTION_HEIGHT)

#define SCREEN_AR_ACADEMY			(11.0f / 8.0f)	// 660.0f / 480.0f
#define SCREEN_AR_NARROW			( 4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_AR_STRETCH			(10.0f / 7.0f)	// 640.0f / 448.0f
#define SCREEN_AR_CUTSCENE			( 5.0f / 4.0f)	// 560.0f / 448.0f

#define SCREEN_FOV_HORIZONTAL		70.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is ~55.0f.

#define RWFORMAT_A1R5G5B5			0x0100
#define RWFORMAT_R5G6B5				0x0200
#define RWFORMAT_A8R8G8B8			0x0500
#define RWFORMAT_X1R8G8B8			0x0600
#define RWFORMAT_X1R5G5B5			0x0A00

enum EFxQuality: int {
	FX_QUALITY_LOW,
	FX_QUALITY_VERY_HIGH						= 3
};

enum EWeaponModel: short {
	WEAPONMODEL_SNIPERRIFLE						= 358,
	WEAPONMODEL_CAMERA							= 367
};

enum ESoundEvent: int {
	SOUND_FRONTEND_UPDOWN						= 3
};

enum ECounterDisplay: short {
	COUNTER_DISPLAY_NUMBER,
	COUNTER_DISPLAY_BAR,
	COUNTER_DISPLAY_INTERVAL
};

enum EMenuRadarMode: int {
	MENU_RADAR_BOTH,
	MENU_RADAR_NONE								= 2
};

enum EMenuTexture: char {};
enum EMenuPage: char {};

enum EMenuAction : int {
	MENU_ACTION_GOBACK							= 3,
	MENU_ACTION_GONEXT
};

enum EMenuOptionDir: unsigned char {
	MENU_OPTION_NONE,
	MENU_OPTION_GONEXT,
	MENU_OPTION_GOBACK							= 0xFF
};

enum ERadarBlip: char {};
enum ERadarSprite: int {};

enum ETextAlign: char {
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_RIGHT
};

enum ERwSysDevRequest: int {
	RWSYSDEVREQUEST_GETNUMVIDEORATIOS			= 23,
	RWSYSDEVREQUEST_GETCURRENTVIDEOMODERATIO,
	RWSYSDEVREQUEST_GETCURRENTVIDEORATIO,
	RWSYSDEVREQUEST_GETVIDEORATIOINFO,
	RWSYSDEVREQUEST_SETVIDEORATIO
};

#pragma pack(push, 1)

struct SRwTexture;

struct SRwV2d {
	float m_fX;
	float m_fY;
};

struct SRwV3d {
	float m_fX;
	float m_fY;
	float m_fZ;
};

struct SRwRect {
	float m_fLeft;
	float m_fTop;
	float m_fRight;
	float m_fBottom;
};

struct SRwDevice {
	int m_iAlign0;
	bool (__cdecl *m_pcbSystem)(ERwSysDevRequest, void *, void *, int);
	char m_acAlign0[0x30];
};

struct SRwEngineInstance {
	char m_acAlign0[0x10];
	SRwDevice m_stOpenDevice;
};

struct SRwD3d9DisplayRatio {
	float m_fRatio;
	int m_iNumerator;
	int m_iDenominator;
	int m_iAdapter;
};

struct SRwVideoMode {
	int m_iWidth;
	int m_iHeight;
	int m_iDepth;
	int m_iAdapter;
	short m_sRefreshRate;
	short m_sRatioIndex;
	unsigned int m_uiFormat;
};

struct SRwVideoRatio : SRwD3d9DisplayRatio {
	bool m_bWidescreen;
	char m_acPadding0[3];
};

struct SRwRaster {
	char m_acAlign[0x0C];
	int m_iWidth;
	int m_iHeight;
};

struct SRwCamera {
	char m_acAlign[0x60];
	SRwRaster *m_pstFrameBuffer;
};

class CParticle {
  public:
	char m_acAlign0[0x54];
	EFxQuality m_eQuality;
};

class CAudioManager;

class CCam {
  public:
	char m_acAlign0[0x000C];
	bool m_bLocked;
	char m_acAlign1[0x00A7];
	float m_fZoomFactor;
	char m_acAlign2[0x0180];
};

class CCamera {
  public:
	char m_acAlign0[0x003D];
	bool m_bWidescreen;
	char m_acAlign1[0x001B];
	char m_cActiveCam;
	char m_acAlign2[0x011A];
	CCam m_aclCams[3];
};

class CPed;

class CPlayer {
  public:
	CPed *m_pclPed;
	char m_acAlign0[0x014B];
	char m_cMaxHealth;
	char m_acAlign1[0x0040];
};
/*
class COnScreenTimer {
  public:
	int m_iScriptVarValue;
	char m_szGxtEntry[8];
	short m_sAlign0;
	char m_szValue[6];
	char m_acAlign1[0x24];
	bool m_bShow;
	bool m_bEnable;
	short m_sAlign2;
	int m_iSecsToPlaySound;
};

class COnScreenCounter {
  public:
	int m_iScriptVarValue;
	int m_iScriptVarMaxValue;
	char m_szGxtEntry[8];
	short m_sAlign0;
	ECounterDisplay m_sType;
	char m_szValue[8];
	char m_acAlign1[0x22];
	bool m_bShow;
	bool m_bFlashOnShow;
	char m_cBaseColor;
	char m_acAlign2[0x03];
};
*/
class CMenuManager {
  public:
	char m_acAlign0[0x0024];
	EMenuRadarMode m_eRadarMode;
	char m_acAlign1[0x0014];
	int m_iBrightness;
	float m_fDrawDistance;
	char m_acAlign2[0x0007];
	char m_cVideoRatio;					// bool m_bWidescreen
	char m_acAlign3[0x0003];
	char m_cSfxVolume;
	char m_cRadioVolume;
	char m_acAlign4[0x003F];
	int m_iSelectedInput;
	char m_acAlign5[0x0023];
	bool m_bCarControls;
	bool m_bCursorShown;
	char m_acAlign6[0x0003];
	int m_iMousePosX;
	int m_iMousePosY;
	int m_iAlign7;
	int m_iMultiSamplingLevels;
	int m_iSelectedMultiSamplingLevels;
	bool m_bUsingJoypad;
	char m_acAlign8[0x0003];
	int m_iVideoMode;
	int m_iSelectedVideoMode;
	char m_acAlign9[0x001C];
	SRwTexture *m_apstTextures[25];
	char m_acAlign10[0x1996];
	bool m_bInputUp;
	bool m_bInputDown;
	char m_acAlign11[0x000C];
	EMenuAction m_eEntryAction;
	int m_iLastSelectedEntry;
	char m_cAlign12;
	bool m_bChangingInput;
	char m_acAlign13[0x0046];
	EMenuTexture m_eFrontend;
};

class CColor;
class CTexture;

class CPsGlobal {
	HWND m_hWindow;
};

class CRsGlobal {
  public:
	int m_iAlign0;
	int m_iScreenWidth;
	int m_iScreenHeight;
};

class CTextData {
  public:
	char *m_pacData;
	int m_iSize;
};

class CTextKey {
  public:
	char *m_pszData;
	unsigned int m_uiKey; // CRC32
};

class CTextKeys {
  public:
	CTextKey *m_paclKeys;
	short m_sCount;
	short m_sAlign0;
};

class CText {
  public:
	CTextKeys m_clMainKeys;
	CTextData m_clMainData;
};

#pragma pack(pop)

#define ADDR_drawRampageCounter								0x43CE30
#define ADDR_0043D095_drawRampageCounter					0x43D005
#define ADDR_drawCollectableText							0x4476A0
#define ADDR_drawPickupMoneyText							0x454EE0
#define ADDR_00455132_drawPickupMoneyText					0x455012
#define ADDR_drawReplayText									0x45C0F0
#define ADDR_drawRadioStationText							0x4E9D10
#define ADDR_CAudioManager__playFrontendSound				0x506D60
#define ADDR_getScreenFieldOfView							0x5095E0
#define ADDR_getScreenAspectRatio							0x5095F0
//#define ADDR_getCrc32FromString								0x53CDF0
#define ADDR_draw2dStuff									0x53E120
//#define ADDR_CMenuManager__scaleToScreenWidth				0x5732D0
//#define ADDR_CMenuManager__scaleToScreenHeight				0x573300
#define ADDR_CMenuManager__setPage							0x573570
#define ADDR_005736BC_CMenuManager__setPage					0x5735AC
#define ADDR_CMenuManager__resetPageSettings				0x5739D0
#define ADDR_00573BE5_CMenuManager__resetPageSettings		0x573AD5
#define ADDR_CMenuManager__drawWindow						0x573DD0
#define ADDR_CMenuManager__drawMessage						0x573F00
#define ADDR_00574043_CMenuManager__drawMessage				0x573F33
#define ADDR_005740D0_CMenuManager__drawMessage				0x573FC0
#define ADDR_0057414D_CMenuManager__drawMessage				0x57403D
#define ADDR_005741D0_CMenuManager__drawMessage				0x5740C0
#define ADDR_0057423F_CMenuManager__drawMessage				0x57412F
#define ADDR_0057426A_CMenuManager__drawMessage				0x57415A
#define ADDR_CMenuManager__drawRadioStationSprites			0x5745E0
#define ADDR_00574706_CMenuManager__drawRadioStationSprites	0x5745F6
#define ADDR_005747B2_CMenuManager__drawRadioStationSprites	0x5746A2
#define ADDR_005747EF_CMenuManager__drawRadioStationSprites	0x5746DF
#define ADDR_005748B6_CMenuManager__drawRadioStationSprites	0x5747A6
#define ADDR_CMenuManager__drawStats						0x5747F0
#define ADDR_005749B9_CMenuManager__drawStats				0x5748A9
#define ADDR_00574ED2_CMenuManager__drawStats				0x574DC2
#define ADDR_00575023_CMenuManager__drawStats				0x574F13
#define ADDR_CMenuManager__drawMap							0x575020
#define ADDR_00575BB3_CMenuManager__drawMap					0x575AA3
#define ADDR_00575EF3_CMenuManager__drawMap					0x575DE3
#define ADDR_0057614F_CMenuManager__drawMap					0x57603F
#define ADDR_CMenuManager__drawBrief						0x576210
#define ADDR_005763D4_CMenuManager__drawBriefs				0x5762C4
#define ADDR_0057668F_CMenuManager__drawBriefs				0x57657F
#define ADDR_005767FE_CMenuManager__drawBriefs				0x5766EE
#define ADDR_CMenuManager__drawEntryBars					0x576750
#define ADDR_005769EF_CMenuManager__drawEntryBars			0x5768DF
#define ADDR_CMenuManager__drawWindowText					0x578E40
#define ADDR_00579055_CMenuManager__drawWindowText			0x578F45
#define ADDR_005790B2_CMenuManager__drawWindowText			0x578FA2
#define ADDR_005790DC_CMenuManager__drawWindowText			0x578FCC
#define ADDR_005792A8_CMenuManager__drawWindowText			0x579198
#define ADDR_CMenuManager__drawEntries						0x579390
#define ADDR_005795E3_CMenuManager__drawEntries				0x5794D3
#define ADDR_0057972C_CMenuManager__drawEntries				0x57961C
#define ADDR_00579998_CMenuManager__drawEntries				0x579888
#define ADDR_00579A1E_CMenuManager__drawEntries				0x57990E
#define ADDR_00579CCA_CMenuManager__drawEntries				0x579BBA
#define ADDR_0057A385_CMenuManager__drawEntries				0x57A275
#define ADDR_0057A461_CMenuManager__drawEntries				0x57A351
#define ADDR_0057A5B6_CMenuManager__drawEntries				0x57A4A6
#define ADDR_0057B0FF_CMenuManager__drawEntries				0x57AFEF
#define ADDR_0057B180_CMenuManager__drawEntries				0x57B070
#define ADDR_0057B1F3_CMenuManager__drawEntries				0x57B0E3
#define ADDR_0057B222_CMenuManager__drawEntries				0x57B112
#define ADDR_CMenuManager__drawPage							0x57B640
#define ADDR_0057B9CC_CMenuManager__drawPage				0x57B8BC
#define ADDR_0057BA02_CMenuManager__drawPage				0x57B8F2
#define ADDR_0057BD46_CMenuManager__drawPage				0x57BC36
#define ADDR_0057BDF8_CMenuManager__drawPage				0x57BCE8
#define ADDR_0057C18F_CMenuManager__drawPage				0x57BFBF
#define ADDR_0057C1F5_CMenuManager__drawPage				0x57C025
#define ADDR_0057C274_CMenuManager__drawPage				0x57C0A4
#define ADDR_CMenuManager__updateEntrySetting				0x57CB80
#define ADDR_0057D438_CMenuManager__updateEntrySetting		0x57D268
#define ADDR_CMenuManager__drawDebugInputs					0x57D700
#define ADDR_0057D937_CMenuManager__drawDebugInputs			0x57D767
#define ADDR_0057DA8E_CMenuManager__drawDebugInputs			0x57D8BE
#define ADDR_CMenuManager__drawBottomMessage				0x57E070
#define ADDR_0057E277_CMenuManager__drawBottomMessage		0x57E0A7
#define ADDR_CMenuManager__drawInputs						0x57E510
#define ADDR_0057E764_CMenuManager__drawInputs				0x57E594
#define ADDR_0057E768_CMenuManager__drawInputs				0x57E598
#define ADDR_0057EAA8_CMenuManager__drawInputs				0x57E8D8
#define ADDR_0057EB2E_CMenuManager__drawInputs				0x57E95E
#define ADDR_0057EDE3_CMenuManager__drawInputs				0x57EC13
#define ADDR_CMenuManager__processController				0x57ED80
#define ADDR_0057EFF6_CMenuManager__processController		0x57EE26
#define ADDR_0057F23C_CMenuManager__processController		0x57F06C
#define ADDR_0057F2A3_CMenuManager__processController		0x57F0D3
#define ADDR_0057F2A9_CMenuManager__processController		0x57F0D9
#define ADDR_0057F2F7_CMenuManager__processController		0x57F127
#define ADDR_CMenuManager__drawControls						0x57F130
#define ADDR_0057F6B3_CMenuManager__drawControls			0x57F4E3
#define ADDR_0057F7E9_CMenuManager__drawControls			0x57F619
#define ADDR_0057F8D0_CMenuManager__drawControls			0x57F700
#define ADDR_0057F987_CMenuManager__drawControls			0x57F7B7
#define ADDR_0057F9AF_CMenuManager__drawControls			0x57F7DF
#define ADDR_0057F9C0_CMenuManager__drawControls			0x57F7F0
#define ADDR_0057F9E1_CMenuManager__drawControls			0x57F811
#define ADDR_0057FA38_CMenuManager__drawControls			0x57F868
#define ADDR_0057FAAD_CMenuManager__drawControls			0x57F8DD
#define ADDR_0057FAD9_CMenuManager__drawControls			0x57F909
#define ADDR_0057FCE9_CMenuManager__drawControls			0x57FB19
#define ADDR_0057FCC2_CMenuManager__drawControls			0x57FAF2
#define ADDR_CMenuManager__drawMenu							0x580C30
#define ADDR_CMenuManager__drawCarcolMenu					0x581510
#define ADDR_CMenuManager__drawMapLegend					0x5826D0
#define ADDR_00582E27_CMenuManager__drawMapLegend			0x582C57
#define ADDR_zoomMapCoords									0x5832B0
#define ADDR_drawRadarBlip									0x583EA0
#define ADDR_drawRotatingRadarSprite						0x584680
#define ADDR_drawRadarPosition								0x584790
#define ADDR_drawRadarSprite								0x585E20
#define ADDR_drawBlipForEntity								0x586E30
#define ADDR_drawBlips										0x587E80
#define ADDR_0058822D_drawBlips								0x58805D
#define ADDR_005886DA_drawBlips								0x58850A
#define ADDR_getHudComponentCoordY							0x588990
#define ADDR_drawArmourMeter								0x588ED0
#define ADDR_drawBreathMeter								0x588FC0
#define ADDR_drawHealthMeter								0x5890A0
#define ADDR_drawWeaponAmmo									0x5891E0
#define ADDR_drawStatistics									0x589480
#define ADDR_00589728_drawStatistics						0x589558
#define ADDR_drawTripSkip									0x589F90
#define ADDR_drawMap										0x58A160
#define ADDR_0058A73B_drawMap								0x58A56B
#define ADDR_drawZoneName									0x58A880
#define ADDR_drawVehicleName								0x58ACD0
#define ADDR_drawOnScreenDisplays							0x58AFB0
#define ADDR_0058B24C_drawOnScreenDisplays					0x58B07C
#define ADDR_drawMessageBox									0x58B510
#define ADDR_0058BFA2_drawMessageBox						0x58BDD2
#define ADDR_drawSubtitleText								0x58C080
#define ADDR_drawMissionText								0x58C4D0
#define ADDR_drawTitleText									0x58C880
#define ADDR_drawSecondaryMissionText						0x58CAB0
#define ADDR_drawLoadMissionText							0x58D070
#define ADDR_drawWeaponIcon									0x58D600
#define ADDR_drawWantedLevel								0x58D7D0
#define ADDR_drawWeaponCrosshair							0x58DE50
#define ADDR_drawHudInterface								0x58E920
#define ADDR_drawCreditText									0x5A8490
#define ADDR_005A86C0_drawCreditText						0x5A84F0
#define ADDR_005A8790_drawCreditText						0x5A85C0
#define ADDR_drawCredits									0x5A8620
#define ADDR_005A8824_drawCredits							0x5A8654
#define ADDR_005A8840_drawCredits							0x5A8670
#define ADDR_setScreenFieldOfView							0x6FE410
#define ADDR_updateScreenAspectRatio						0x6FE420
#define ADDR_precalc2dStuffCoords							0x700450
#define ADDR_updateCameraRaster								0x7033D0
#define ADDR_transform3DTo2DPointInClip						0x70BE30
#define ADDR_getTextCharWidth								0x717770
#define ADDR_drawTextChar									0x717A10
#define ADDR_00718C16_drawTextChar							0x717C16
#define ADDR_getTextRect									0x719620
#define ADDR_0071A641_getTextRect							0x719641
#define ADDR_drawTextTop									0x719700
#define ADDR_processText									0x719220
#define ADDR_transform3DTo2DPoint							0x71CA00
#define ADDR_drawRect										0x726B60
#define ADDR_drawMeter										0x727640
#define ADDR_psPrintVideoModeList							0x744920
#define ADDR_psAllocVideoModeList							0x744AF0
#define ADDR_psSelectDevice									0x745190
#define ADDR_007462C5_psSelectDevice						0x7452C5
#define ADDR_007462FA_psSelectDevice						0x7452FA
#define ADDR_007463D4_psSelectDevice						0x7453D4
//#define ADDR_fHeightLevel2									0x857A50
#define ADDR_fScreenFieldOfViewStd							0x857CE8
//#define ADDR_fHeightLevel3									0x857F54
//#define ADDR_fHeightLevel1									0x85801C
//#define ADDR_fHeightLevel4									0x859B08
#define ADDR_fScreenWidthStd								0x858524
#define ADDR_fScreenWidthScale								0x858528
#define ADDR_fScreenHeightScale								0x85852C
#define ADDR_fScreenHeightStd								0x8642B8
#define ADDR_aiFootCtrlIndices								0x8645A0
#define ADDR_aiCarCtrlIndices								0x864610
#define ADDR_iUTraxScanPosX									0x8CD008
#define ADDR_iDefaultDisplayMode							0x8D1E3C
#define ADDR_fScreenFieldOfView								0x695660
#define ADDR_iCurSelVideoMode								0x8D5228
/*#define ADDR_iRampageTime									0x9696F0
#define ADDR_iRampageKills									0x9696F4*/
/*#define ADDR_iMaxCollectables								0x96B014
#define ADDR_iCollectables									0x96B018
#define ADDR_szCollectableGxtEntry							0x96B01C*/
/*#define ADDR_sSubtitleTextCentreSize						0x00A44B60
#define ADDR_sSubtitleTextPosX								0x00A44B64
#define ADDR_bSubtitleTextChangedPos						0x00A44B66*/
//#define ADDR_acScriptBuffer									0x00A49960
#define ADDR_clParticle										0x00A9AE00
#define ADDR_clAudioManager									0x00B6BC90
#define ADDR_fMouseSensivity								0x00B6EC1C
#define ADDR_clCamera										0x00B6F028
#define ADDR_ucPlayer										0x00B7CD74
#define ADDR_aclPlayers										0x00B7CD98
/*#define ADDR_clOnScreenTimer								0x00BA1788
#define ADDR_clOnScreenCounters								0x00BA17C8
#define ADDR_bShowOnScreenDisplays							0x00BA18D8
#define ADDR_bFreezeTimer									0x00BA18D9*/
#define ADDR_clMenuManager									0x00BA6748
//#define ADDR_pszVehicleName									0x00BAA444
//#define ADDR_iMessageBoxShownTime							0x00BAA47C
//#define ADDR_szMissionText									0x00BAACC0
//#define ADDR_szLoadMissionText								0x00BAAD40
//#define ADDR_szTitleText									0x00BAADC0
/*#define ADDR_szOddJobTextA									0x00BAAE40
#define ADDR_szOddJobTextB									0x00BAAEC0
#define ADDR_szRewardTextA									0x00BAAF40
#define ADDR_szRewardTextB									0x00BAAFC0*/
//#define ADDR_pszZoneName									0x00BAB1D0
#define ADDR_pstGtaCamera									0x7E3690
#define ADDR_clRsGlobal										0x9B38E0
#define ADDR_clText											0x00C1B340
#define ADDR_fScreenAspectRatio								0x94CD40
#define ADDR_pstRwEngineInstance							0x00C97B24
#define ADDR_iRwCurrentDisplayMode							0x00C97C18
#define ADDR_iRwNumDisplayModes								0x00C97C40
#define ADDR_pastRwDisplayModeList							0x00C97C48
#define ADDR_iRwNumAdapterDisplayModes						0x00C9BEE0
#define ADDR_stRwAdapterDisplayMode							0x00C9BEE4

#define ADDR_pstGtaCamera_steam									0x7E2698
#define ADDR_clRsGlobal_steam										0x9B28E8
#define ADDR_fScreenAspectRatio_steam								0x94BD48
#define ADDR_fScreenFieldOfView_steam								0x694668

//float				  *const VAR_pfHeightLevel2							= (float						  *) ADDR_fHeightLevel2;
float				  *const VAR_pfScreenFieldOfViewStd					= (float						  *) ADDR_fScreenFieldOfViewStd;
//float				  *const VAR_pfHeightLevel3							= (float						  *) ADDR_fHeightLevel3;
//float				  *const VAR_pfHeightLevel1							= (float						  *) ADDR_fHeightLevel1;
//float				  *const VAR_pfHeightLevel4							= (float						  *) ADDR_fHeightLevel4;
float				  *const VAR_pfScreenWidthStd						= (float						  *) ADDR_fScreenWidthStd;
float				  *const VAR_pfScreenWidthScale						= (float						  *) ADDR_fScreenWidthScale;
float				  *const VAR_pfScreenHeightScale					= (float						  *) ADDR_fScreenHeightScale;
float				  *const VAR_pfScreenHeightStd						= (float						  *) ADDR_fScreenHeightStd;
int					( *const VAR_paiFootCtrlIndices)[28]				= (int						(*)[28]) ADDR_aiFootCtrlIndices;
int					( *const VAR_paiCarCtrlIndices)[25]					= (int						(*)[25]) ADDR_aiCarCtrlIndices;
int					  *const VAR_piUTraxScanPosX						= (int							  *) ADDR_iUTraxScanPosX;
int					  *const VAR_piDefaultDisplayMode					= (int							  *) ADDR_iDefaultDisplayMode;
float				  *const VAR_pfScreenFieldOfView					= (float						  *) ADDR_fScreenFieldOfView;
int					  *const VAR_piCurSelVideoMode						= (int							  *) ADDR_iCurSelVideoMode;
/*int					  *const VAR_piRampageTime							= (int							  *) ADDR_iRampageTime;
int					  *const VAR_piRampageKills							= (int							  *) ADDR_iRampageKills;*/
/*int					  *const VAR_piMaxCollectables						= (int							  *) ADDR_iMaxCollectables;
int					  *const VAR_piCollectables							= (int							  *) ADDR_iCollectables;
char				  *const VAR_pszCollectableGxtEntry					= (char							  *) ADDR_szCollectableGxtEntry;*/
/*short				  *const VAR_psSubtitleTextCentreSize				= (short						  *) ADDR_sSubtitleTextCentreSize;
short				  *const VAR_psSubtitleTextPosX						= (short						  *) ADDR_sSubtitleTextPosX;
bool				  *const VAR_pbSubtitleTextChangedPos				= (bool							  *) ADDR_bSubtitleTextChangedPos;*/
//char				( *const VAR_pacScriptBuffer)[200000]				= (char					(*)[200000]) ADDR_acScriptBuffer;
float				  *const VAR_pfMouseSensivity						= (float						  *) ADDR_fMouseSensivity;
unsigned char		  *const VAR_pucPlayer								= (unsigned char				  *) ADDR_ucPlayer;
/*bool				  *const VAR_pbShowOnScreenDisplays					= (bool							  *) ADDR_bShowOnScreenDisplays;
bool				  *const VAR_pbFreezeTimer							= (bool							  *) ADDR_bFreezeTimer;*/
//char				 **const VAR_ppszVehicleName						= (char							 **) ADDR_pszVehicleName;
//int					  *const VAR_piMessageBoxShownTime					= (int							  *) ADDR_iMessageBoxShownTime;
//char				  *const VAR_pszMissionText							= (char							  *) ADDR_szMissionText;
//char				  *const VAR_pszLoadMissionText						= (char							  *) ADDR_szLoadMissionText;
//char				  *const VAR_pszTitleText							= (char							  *) ADDR_szTitleText;
/*char				  *const VAR_pszOddJobTextA							= (char							  *) ADDR_szOddJobTextA;
char				  *const VAR_pszOddJobTextB							= (char							  *) ADDR_szOddJobTextB;
char				  *const VAR_pszRewardTextA							= (char							  *) ADDR_szRewardTextA;
char				  *const VAR_pszRewardTextB							= (char							  *) ADDR_szRewardTextB;*/
//char				 **const VAR_ppszZoneName							= (char							 **) ADDR_pszZoneName;
SRwCamera			  *const VAR_pstGtaCamera							= (SRwCamera					  *) ADDR_pstGtaCamera;
float				  *const VAR_pfScreenAspectRatio					= (float						  *) ADDR_fScreenAspectRatio;
SRwEngineInstance	 **const VAR_ppstRwEngineInstance					= (SRwEngineInstance			 **) ADDR_pstRwEngineInstance;
int					  *const VAR_piRwCurrentDisplayMode					= (int							  *) ADDR_iRwCurrentDisplayMode;
int					  *const VAR_piRwNumDisplayModes					= (int							  *) ADDR_iRwNumDisplayModes;

CParticle			  *const CLASS_pclParticle							= (CParticle					  *) ADDR_clParticle;
CAudioManager		  *const CLASS_pclAudioManager						= (CAudioManager				  *) ADDR_clAudioManager;
CCamera				  *const CLASS_pclCamera							= (CCamera						  *) ADDR_clCamera;
CPlayer				( *const CLASS_paclPlayers)[2]						= (CPlayer					 (*)[2]) ADDR_aclPlayers;
/*COnScreenTimer		  *const CLASS_pclOnScreenTimer						= (COnScreenTimer				  *) ADDR_clOnScreenTimer;
COnScreenCounter	( *const CLASS_pclOnScreenCounters)[4]				= (COnScreenCounter			 (*)[4]) ADDR_clOnScreenCounters;*/
CMenuManager		  *const CLASS_pclMenuManager						= (CMenuManager					  *) ADDR_clMenuManager;
CRsGlobal			  *const CLASS_pclRsGlobal							= (CRsGlobal					  *) ADDR_clRsGlobal;
CText				  *const CLASS_pclText								= (CText						  *) ADDR_clText;

SRwCamera			  *const VAR_pstGtaCamera_steam = (SRwCamera					  *)ADDR_pstGtaCamera_steam;
float				  *const VAR_pfScreenAspectRatio_steam = (float						  *)ADDR_fScreenAspectRatio_steam;
CRsGlobal			  *const CLASS_pclRsGlobal_steam = (CRsGlobal					  *)ADDR_clRsGlobal_steam;
float				  *const VAR_pfScreenFieldOfView_steam = (float						  *)ADDR_fScreenFieldOfView_steam;

void				   (__cdecl		*const PROC_drawRampageCounter					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawRampageCounter;
void				   (__cdecl		*const PROC_drawCollectableText					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawCollectableText;
void				   (__cdecl		*const PROC_drawPickupMoneyText					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawPickupMoneyText;
void				   (__cdecl		*const PROC_drawReplayText						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawReplayText;
void				   (__cdecl		*const PROC_drawRadioStationText				 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawRadioStationText;
void				   (__cdecl		*const PROC_CAudioManager__playFrontendSound	 ) (CAudioManager *pclAudioManager, ESoundEvent eSoundId, float fVolume, float fSpeed																								) = (void					  (__cdecl		*) (CAudioManager *, ESoundEvent, float, float												 )) ADDR_CAudioManager__playFrontendSound;
float				   (__cdecl		*const FUNC_getScreenFieldOfView				 ) (																																												) = (float					  (__cdecl		*) (																						 )) ADDR_getScreenFieldOfView;
float				   (__cdecl		*const FUNC_getScreenAspectRatio				 ) (																																												) = (float					  (__cdecl		*) (																						 )) ADDR_getScreenAspectRatio;
//unsigned int		   (__cdecl		*const FUNC_getCrc32FromString					 ) (char *pszKey																																									) = (unsigned int			  (__cdecl		*) (char *																					 )) ADDR_getCrc32FromString;
void				   (__cdecl		*const PROC_draw2dStuff							 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_draw2dStuff;
//float				   (__thiscall	*const FUNC_CMenuManager__scaleToScreenWidth	 ) (CMenuManager *pclMenuManager, float fCoordX																																		) = (float					  (__thiscall	*) (CMenuManager *, float																	 )) ADDR_CMenuManager__scaleToScreenWidth;
//float				   (__thiscall	*const FUNC_CMenuManager__scaleToScreenHeight	 ) (CMenuManager *pclMenuManager, float fCoordY																																		) = (float					  (__thiscall	*) (CMenuManager *, float																	 )) ADDR_CMenuManager__scaleToScreenHeight;
void				   (__thiscall	*const PROC_CMenuManager__setPage				 ) (CMenuManager *pclMenuManager, EMenuPage ePageId																																	) = (void					  (__thiscall	*) (CMenuManager *, EMenuPage																 )) ADDR_CMenuManager__setPage;
void				   (__thiscall	*const PROC_CMenuManager__resetPageSettings		 ) (CMenuManager *pclMenuManager, EMenuPage ePageId																																	) = (void					  (__thiscall	*) (CMenuManager *, EMenuPage																 )) ADDR_CMenuManager__resetPageSettings;
void				   (__thiscall	*const PROC_CMenuManager__drawWindow			 ) (CMenuManager *pclMenuManager, SRwRect *pstRect, char *pszGxtTitle, unsigned char ucTextChrome, CColor clRect, char cUnused, bool bDrawRect										) = (void					  (__thiscall	*) (CMenuManager *, SRwRect *, char *, unsigned char, CColor, char, bool					 )) ADDR_CMenuManager__drawWindow;
void				   (__thiscall	*const PROC_CMenuManager__drawMessage			 ) (CMenuManager *pclMenuManager, char *pszGxtEntry																																	) = (void					  (__thiscall	*) (CMenuManager *, char *																	 )) ADDR_CMenuManager__drawMessage;
void				   (__thiscall	*const PROC_CMenuManager__drawRadioStationSprites) (CMenuManager *pclMenuManager																																					) = (void					  (__thiscall	*) (CMenuManager *																			 )) ADDR_CMenuManager__drawRadioStationSprites;
void				   (__thiscall	*const PROC_CMenuManager__drawStats				 ) (CMenuManager *pclMenuManager																																					) = (void					  (__thiscall	*) (CMenuManager *																			 )) ADDR_CMenuManager__drawStats;
void				   (__thiscall	*const PROC_CMenuManager__drawMap				 ) (CMenuManager *pclMenuManager																																					) = (void					  (__thiscall	*) (CMenuManager *																			 )) ADDR_CMenuManager__drawMap;
void				   (__thiscall	*const PROC_CMenuManager__drawBriefs			 ) (CMenuManager *pclMenuManager																																					) = (void					  (__thiscall	*) (CMenuManager *																			 )) ADDR_CMenuManager__drawBrief;
void				   (__thiscall	*const PROC_CMenuManager__drawEntryBars			 ) (CMenuManager *pclMenuManager, float fPosX, float fPosY, float fStartHeight, float fEndHeight, float fTotalWidth, float fValue, float fWidth										) = (void					  (__thiscall	*) (CMenuManager *, float, float, float, float, float, float, float							 )) ADDR_CMenuManager__drawEntryBars;
void				   (__thiscall	*const PROC_CMenuManager__drawWindowText		 ) (CMenuManager *pclMenuManager, float fLeft, float fTop, float fWidth, char *pszGxtTitle, char *pszGxtText, ETextAlign eAlignment													) = (void					  (__thiscall	*) (CMenuManager *, float, float, float, char *, char *, ETextAlign							 )) ADDR_CMenuManager__drawWindowText;
void				   (__thiscall	*const PROC_CMenuManager__drawEntries			 ) (CMenuManager *pclMenuManager, bool bAudioPage																																	) = (void					  (__thiscall	*) (CMenuManager *, bool																	 )) ADDR_CMenuManager__drawEntries;
void				   (__thiscall	*const PROC_CMenuManager__drawPage				 ) (CMenuManager *pclMenuManager																																					) = (void					  (__thiscall	*) (CMenuManager *																			 )) ADDR_CMenuManager__drawPage;
bool				   (__thiscall	*const FUNC_CMenuManager__updateEntrySetting	 ) (CMenuManager *pclMenuManager, EMenuOptionDir eOptionDir, bool bNoReset																											) = (bool					  (__thiscall	*) (CMenuManager *, EMenuOptionDir, bool													 )) ADDR_CMenuManager__updateEntrySetting;
void				   (__thiscall	*const PROC_CMenuManager__drawDebugInputs		 ) (CMenuManager *pclMenuManager, int iPosY																																			) = (void					  (__thiscall	*) (CMenuManager *, int																		 )) ADDR_CMenuManager__drawDebugInputs;
void				   (__thiscall	*const PROC_CMenuManager__drawBottomMessage		 ) (CMenuManager *pclMenuManager, char *pszGxtEntry																																	) = (void					  (__thiscall	*) (CMenuManager *, char *																	 )) ADDR_CMenuManager__drawBottomMessage;
void				   (__thiscall	*const PROC_CMenuManager__drawInputs			 ) (CMenuManager *pclMenuManager, int iPosY, bool bHide																																) = (void					  (__thiscall	*) (CMenuManager *, int, bool																 )) ADDR_CMenuManager__drawInputs;
void				   (__thiscall	*const PROC_CMenuManager__processController		 ) (CMenuManager *pclMenuManager, bool *pbGoNext, bool *pbGoBack																													) = (void					  (__thiscall	*) (CMenuManager *, bool *, bool *															 )) ADDR_CMenuManager__processController;
void				   (__thiscall	*const PROC_CMenuManager__drawControls			 ) (CMenuManager *pclMenuManager																																					) = (void					  (__thiscall	*) (CMenuManager *																			 )) ADDR_CMenuManager__drawControls;
void				   (__thiscall	*const PROC_CMenuManager__drawMenu				 ) (CMenuManager *pclMenuManager, bool bInteractive																																	) = (void					  (__thiscall	*) (CMenuManager *, bool																	 )) ADDR_CMenuManager__drawMenu;
void				   (__thiscall	*const PROC_CMenuManager__drawCarcolMenu		 ) (CMenuManager *pclMenuManager, bool bInteractive																																	) = (void					  (__thiscall	*) (CMenuManager *, bool																	 )) ADDR_CMenuManager__drawCarcolMenu;
void				   (__thiscall	*const PROC_CMenuManager__drawMapLegend			 ) (CMenuManager *pclMenuManager, float fPosX, float fPosY, int iMarkerId																											) = (void					  (__thiscall	*) (CMenuManager *, float, float, int														 )) ADDR_CMenuManager__drawMapLegend;
void				   (__cdecl		*const PROC_zoomMapCoords						 ) (SRwV2d *pstMapZoomedPos, SRwV2d *pstMapPos																																		) = (void					  (__cdecl		*) (SRwV2d *, SRwV2d *																		 )) ADDR_zoomMapCoords;
void				   (__cdecl		*const PROC_drawRadarBlip						 ) (float fPosX, float fPosY, int iPad, unsigned char ucRed, unsigned char ucGreen, unsigned char ucBlue, unsigned char ucAlpha, ERadarBlip eType									) = (void					  (__cdecl		*) (float, float, int, unsigned char, unsigned char, unsigned char, unsigned char, ERadarBlip)) ADDR_drawRadarBlip;
void				   (__cdecl		*const PROC_drawRotatingRadarSprite				 ) (CTexture clTexture, float fPosX, float fPosY, float fAngle, float fWidth, float fHeight, CColor *pclColor																		) = (void					  (__cdecl		*) (CTexture, float, float, float, float, float, CColor *									 )) ADDR_drawRotatingRadarSprite;
void				   (__cdecl		*const PROC_drawRadarPosition					 ) (float fPosX, float fPosY																																						) = (void					  (__cdecl		*) (float, float																			 )) ADDR_drawRadarPosition;
void				   (__cdecl		*const PROC_drawRadarSprite						 ) (ERadarSprite eType, float fPosX, float fPosY, unsigned char ucAlpha																												) = (void					  (__cdecl		*) (ERadarSprite, float, float, unsigned char												 )) ADDR_drawRadarSprite;
void				   (__cdecl		*const PROC_drawBlipForEntity					 ) (ERadarSprite eType, bool bDrawLights																																			) = (void					  (__cdecl		*) (ERadarSprite, bool																		 )) ADDR_drawBlipForEntity;
void				   (__cdecl		*const PROC_drawBlips							 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawBlips;
float				   (__cdecl		*const FUNC_getHudComponentCoordY				 ) (int iPlayerId, float fCoordY, char cPadY																																		) = (float					  (__cdecl		*) (int, float, char																		 )) ADDR_getHudComponentCoordY;
void				   (__cdecl		*const PROC_drawArmourMeter						 ) (int iPlayerId, int iPosX, int iPosY																																				) = (void					  (__cdecl		*) (int, int, int																			 )) ADDR_drawArmourMeter;
void				   (__cdecl		*const PROC_drawBreathMeter						 ) (int iPlayerId, int iPosX, int iPosY																																				) = (void					  (__cdecl		*) (int, int, int																			 )) ADDR_drawBreathMeter;
void				   (__cdecl		*const PROC_drawHealthMeter						 ) (int iPlayerId, int iPosX, int iPosY																																				) = (void					  (__cdecl		*) (int, int, int																			 )) ADDR_drawHealthMeter;
void				   (__cdecl		*const PROC_drawWeaponAmmo						 ) (CPed *pclPed, int iPosX, int iPosY, float fAlpha																																) = (void					  (__cdecl		*) (CPed *, int, int, float																	 )) ADDR_drawWeaponAmmo;
void				   (__cdecl		*const PROC_drawStatistics						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawStatistics;
void				   (__cdecl		*const PROC_drawTripSkip						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawTripSkip;
void				   (__cdecl		*const PROC_drawMap								 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawMap;
void				   (__cdecl		*const PROC_drawZoneName						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawZoneName;
void				   (__cdecl		*const PROC_drawVehicleName						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawVehicleName;
void				   (__cdecl		*const PROC_drawOnScreenDisplays				 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawOnScreenDisplays;
void				   (__cdecl		*const PROC_drawMessageBox						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawMessageBox;
void				   (__cdecl		*const PROC_drawSubtitleText					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawSubtitleText;
void				   (__cdecl		*const PROC_drawMissionText						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawMissionText;
void				   (__cdecl		*const PROC_drawTitleText						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawTitleText;
void				   (__cdecl		*const PROC_drawSecondaryMissionText			 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawSecondaryMissionText;
void				   (__cdecl		*const PROC_drawLoadMissionText					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawLoadMissionText;
void				   (__cdecl		*const PROC_drawWeaponIcon						 ) (CPed *pclPed, int iPosX, int iPosY, float fAlpha																																) = (void					  (__cdecl		*) (CPed *, int, int, float																	 )) ADDR_drawWeaponIcon;
void				   (__cdecl		*const PROC_drawWantedLevel						 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawWantedLevel;
void				   (__cdecl		*const PROC_drawWeaponCrosshair					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawWeaponCrosshair;
void				   (__cdecl		*const PROC_drawHudInterface					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawHudInterface;
void				   (__cdecl		*const PROC_drawCreditText						 ) (float fWidth, float fHeight, char *pszText, float *pfFixedPosY, float fPosY, bool bTitle																						) = (void					  (__cdecl		*) (float, float, char *, float *, float, bool												 )) ADDR_drawCreditText;
void				   (__cdecl		*const PROC_drawCredits							 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_drawCredits;
void				   (__cdecl		*const PROC_setScreenFieldOfView				 ) (float fFactor																																									) = (void					  (__cdecl		*) (float																					 )) ADDR_setScreenFieldOfView;
void				   (__cdecl		*const PROC_updateScreenAspectRatio				 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_updateScreenAspectRatio;
void				   (__cdecl		*const PROC_precalc2dStuffCoords				 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_precalc2dStuffCoords;
void				   (__cdecl		*const PROC_updateCameraRaster					 ) (																																												) = (void					  (__cdecl		*) (																						 )) ADDR_updateCameraRaster;
bool				   (__cdecl		*const FUNC_transform3DTo2DPointInClip			 ) (SRwV3d *pstPoint, SRwV2d *pstScreen, float *pfMultX, float *pfMultY, bool bNearClip, bool bFarClip																				) = (bool					  (__cdecl		*) (SRwV3d *, SRwV2d *, float *, float *, bool, bool										 )) ADDR_transform3DTo2DPointInClip;
float				   (__cdecl		*const FUNC_getTextCharWidth					 ) (char *pszTextChar																																								) = (float					  (__cdecl		*) (char *																					 )) ADDR_getTextCharWidth;
void				   (__cdecl		*const PROC_drawTextChar						 ) (float fPosX, float fPosY, char *pszTextChar																																		) = (void					  (__cdecl		*) (float, float, char *																	 )) ADDR_drawTextChar;
int					   (__cdecl		*const FUNC_getTextRect							 ) (SRwRect *pstRect, float fPosX, float fPosY, char *pszText																														) = (int					  (__cdecl		*) (SRwRect *, float, float, char *															 )) ADDR_getTextRect;
void				   (__cdecl		*const PROC_drawTextTop							 ) (float fPosX, float fPosY, char *pszText																																			) = (void					  (__cdecl		*) (float, float, char *																	 )) ADDR_drawTextTop;
int					   (__cdecl		*const FUNC_processText							 ) (bool bDraw, float fPosX, float fPosY, char *pszText																																) = (int					  (__cdecl		*) (bool, float, float, char *																 )) ADDR_processText;
bool				   (__cdecl		*const FUNC_transform3DTo2DPoint				 ) (SRwV3d *pstPoint, SRwV2d *pstScreen, float *pfMultX, float *pfMultY																												) = (bool					  (__cdecl		*) (SRwV3d *, SRwV2d *, float *, float *													 )) ADDR_transform3DTo2DPoint;
void				   (__cdecl		*const PROC_drawRect							 ) (SRwRect *pstRect, CColor *pclColor																																				) = (void					  (__cdecl		*) (SRwRect *, CColor *																		 )) ADDR_drawRect;
void				   (__cdecl		*const PROC_drawMeter							 ) (float fPosX, float fPosY, short sWidth, char cHeight, float fPercentage, char cRightBorderWidth, bool bPercentage, bool bBorder, CColor clForeColor, CColor clRightBorderColor	) = (void					  (__cdecl		*) (float, float, short, char, float, char, bool, bool, CColor, CColor						 )) ADDR_drawMeter;
char				  *(__cdecl		*const FUNC_psPrintVideoModeList				 ) (/*CPsGlobal *EBX_pclGlobal*/																																					) = (char					 *(__cdecl		*) (/*CPsGlobal **/																			 )) ADDR_psPrintVideoModeList;
char				  *(__cdecl		*const FUNC_psAllocVideoModeList				 ) (																																												) = (char					 *(__cdecl		*) (																						 )) ADDR_psAllocVideoModeList;
bool				   (__cdecl		*const FUNC_psSelectDevice						 ) (																																												) = (bool					  (__cdecl		*) (																						 )) ADDR_psSelectDevice;
/*
#define DMA_SCRIPT_BUFFER(dwMemory) \
	((DWORD)dwMemory - (DWORD)VAR_pacScriptBuffer)
*/
#endif // ndef _GAME_H_
