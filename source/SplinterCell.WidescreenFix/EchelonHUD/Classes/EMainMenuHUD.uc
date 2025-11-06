/******************************************************************************
 Class:         EMainMenuHUD

 Description:   Main Menu HUD
******************************************************************************/

class EMainMenuHUD extends EMenuHUD native;

/*-----------------------------------------------------------------------------
                        M E M B E R   V A R I A B L E S 
-----------------------------------------------------------------------------*/
var bool bEdit;

// MAIN MENU //
var int MainMenuDepth;
var bool bSelectedBadProfile;
var bool bInactVideoPlaying;
var bool bStopInactVideo;
var bool bInputCheck;
var bool bLoadPrefBad;
var bool bSavePref;
var int  iLoadPredBadSkip;


// PLAYERS MENU //
var int PlayerMenuDepth;
var int topPlayerMenuDepth;
var bool bStartTimer;
var float fDisplayTime;		


// SAVEGAME MENU
var int SaveGameMenuDepth;
var int iErrorCode;
var bool bNotEnoughMemory;
var bool bMemoryCheckDone;
var int iBlockMissing;
var bool bSavingGame;
var bool bSavingGameSkipFrame;
var bool bSavingGameBad;
var bool bWantToExitGame;
var bool bDeleteGame;
var bool bDeleteGameBad;
var bool bDeleteGameSkipFrame;
var bool bFromStartMenu;
var EPlayerInfo tempPlayerInfo;
var bool bConfirmContinue;
var bool bLoadingProfile;
var bool bLoadingProfileBad;
var bool bMaxSaveGameReach;
var bool bInvalidPlayerName;
var transient bool bCheckLoading;
var bool bCreatingNewProfile;
var bool bSaveWindowIsUp;
var bool bConfirmBack;
var bool bMusicStarted;
var bool bDiskFull;
var bool bWantToFreeFile;

// Profile menu
var bool bBadProfileCreation;
var bool bNotEnoughMemProfile;

// LOADGAME MENU //
var int LoadGameMenuDepth;
var int LoadGameMenuScroll;
var bool bIsOnLevel;
var bool bLoadingGame;
var bool bLoadingGameBad;
var bool bLoadingGameSkipFrame;
var int iStartPos; 
var int iOldStartPos;
var int iEndPos;
var int iOldEndPos;	
var bool bScrollUp;
var bool bScrollDown;
var string SelectedDownloadMap;
var int CursorPos;
var bool bCheckOnline;
var bool bCheckOnlineSkipFrame;


// LOADING PLAYERS INFO MENU //
var int tempPercentage;

// SETTINGS MENU //
var int oldOffsetX;
var int oldOffsetY;
var int slidderPos;
var int oldSlidderPos;
var bool bBrightness;       // Set if we change brightness or contrast
var int ContrastSlidderPos;
var int BrightnessSlidderPos;
var int MusicSlidderPos;
var int VoiceSlidderPos;
var int SfxSlidderPos;
var int AmbientSlidderPos;
var int CurrentSoundSlot;
var name nextState;

// DEMO MODE//
var bool bOpenVideo;
var bool bCloseVideo;
var float DemoTimer;
var name prevState;

// EXTRA FEATURES
var int iMenuDepth;       
var int iMenuSceneDepth;
var int iMenuRdmFactDepth;
var int iMenuUbiGameDepth;
var int iMenuSamFisherDepth;

var string tExFeature[5];
var string tExBhdTheScene[9];
var string tExUbiGame[4];
var string tExSamFisher[2];

var bool bStopBhdTheSceneVideo;			
var bool bBhdTheSceneVideoPlaying;

var bool bStopUbiGameVideo;		
var bool bUbiGameVideoPlaying;

var bool bStopSamFisherVideo;
var bool bSamFisherVideoPlaying;

var EchelonMainHUD myEchelonMainHUDOwner;
var EPlayerInfo spawner;

var string VideoName;
var int iVideoIndex;

// Credits
var bool bCreditVideoPlaying;
var bool bStopVideo;

var bool bFirstPage;
var bool bSkipFrame;
var float fCurrentTime;

// Download
var bool bDownloadAvailable;
var bool bEthernetOK;
var bool bShowEthernetMsgON;
var bool bShowEthernetMsgOFF;
var bool bCheckDownload;

var bool bShowLoadingScreen;

enum eMapExistCode
{
	eMAP_NO,
	eMAP_YES,
	eMAP_YES_DOWNLOAD	
};


/*-----------------------------------------------------------------------------
                                M E T H O D S
-----------------------------------------------------------------------------*/

native(1346) final function TickMainMenuHUD(float DeltaTime);

native(1347) final function BeginState_s_StartGame();

native(1399) final function BeginState_s_MainMenu();
native(1777) final function EndState_s_MainMenu();
native(1349) final function PostRender_s_MainMenu(ECanvas Canvas);
native(1350) final function bool KeyEvent_s_MainMenu(string Key, EInputAction Action, FLOAT Delta);

native(1351) final function BeginState_s_KeyBoard();
native(1352) final function Tick_s_KeyBoard(float DeltaTime);
native(1353) final function PostRender_s_KeyBoard(ECanvas Canvas);
native(1354) final function bool KeyEvent_s_KeyBoard(string Key, EInputAction Action, FLOAT Delta);

native(1363) final function BeginState_s_SavePlayerInfo();
native(1364) final function Tick_s_SavePlayerInfo(float DeltaTime);
native(1365) final function PostRender_s_SavePlayerInfo(ECanvas Canvas);
native(1366) final function bool KeyEvent_s_SavePlayerInfo(string Key, EInputAction Action, FLOAT Delta);

native(1720) final function BeginState_s_Credit();
native(1721) final function PostRender_s_Credit(ECanvas Canvas);
native(1722) final function bool KeyEvent_s_Credit(string Key, EInputAction Action, FLOAT Delta);

native(1723) final function BeginState_s_Download();
native(1724) final function PostRender_s_Download(ECanvas Canvas);
native(1725) final function bool KeyEvent_s_Download(string Key, EInputAction Action, FLOAT Delta);

native(1726) final function BeginState_s_RestoreSaveGame();
native(1727) final function PostRender_s_RestoreSaveGame(ECanvas Canvas);
native(1728) final function bool KeyEvent_s_RestoreSaveGame(string Key, EInputAction Action, FLOAT Delta);

native(1715) final function Tick_s_PlayerMenu(float DeltaTime);
native(1367) final function BeginState_s_PlayerMenu();
native(1368) final function PostRender_s_PlayerMenu(ECanvas Canvas);
native(1369) final function bool KeyEvent_s_PlayerMenu(string Key, EInputAction Action, FLOAT Delta);

native(1370) final function BeginState_s_PlayerSettings();
native(1371) final function PostRender_s_PlayerSettings(ECanvas Canvas);
native(1372) final function bool KeyEvent_s_PlayerSettings(string Key, EInputAction Action, FLOAT Delta);
native(1719) final function EndState_s_PlayerSettings();

native(1373) final function BeginState_s_Controller();
native(1374) final function Tick_s_Controller(float DeltaTime);
native(1375) final function PostRender_s_Controller(ECanvas Canvas);
native(1376) final function bool KeyEvent_s_Controller(string Key, EInputAction Action, FLOAT Delta);

native(1701) final function BeginState_s_SaveGame();
native(1702) final function PostRender_s_SaveGame(ECanvas Canvas);
native(1703) final function bool KeyEvent_s_SaveGame(string Key, EInputAction Action, FLOAT Delta);
native(1704) final function EndState_s_SaveGame();
native(1705) final function Tick_s_SaveGame(float DeltaTime);

native(1770) final function bool UpdateProfile();
native(1771) final function bool LoadProfile(String PlayerName);
native(1772) final function bool SaveTInfo(int tab);
native(1773) final function bool LoadTInfo(int tab);

native(1377) final function BeginState_s_LoadGame();
native(1378) final function PostRender_s_LoadGame(ECanvas Canvas);
native(1379) final function bool KeyEvent_s_LoadGame(string Key, EInputAction Action, FLOAT Delta);
native(1706) final function Tick_s_LoadGame(float DeltaTime);


native(1746) final function Tick_s_SettingsMenu(float DeltaTime);
native(1380) final function BeginState_s_SettingsMenu();
native(1381) final function PostRender_s_SettingsMenu(ECanvas Canvas);
native(1382) final function bool KeyEvent_s_SettingsMenu(string Key, EInputAction Action, FLOAT Delta);

native(1383) final function BeginState_s_SetHUDPosition();
native(1384) final function EndState_s_SetHUDPosition();
native(1385) final function PostRender_s_SetHUDPosition(ECanvas Canvas);
native(1386) final function bool KeyEvent_s_SetHUDPosition(string Key, EInputAction Action, FLOAT Delta);

native(1387) final function BeginState_s_SetBrightness();
native(1388) final function EndState_s_SetBrightness();
native(1389) final function PostRender_s_SetBrightness(ECanvas Canvas);
native(1390) final function bool KeyEvent_s_SetBrightness(string Key, EInputAction Action, FLOAT Delta);

native(1391) final function BeginState_s_PlayDemo();
native(1392) final function PostRender_s_PlayDemo(ECanvas Canvas);
native(1393) final function bool KeyEvent_s_PlayDemo(string Key, EInputAction Action, FLOAT Delta);

native(1785) final function BeginState_s_Inactivity();
native(1786) final function PostRender_s_Inactivity(ECanvas Canvas);
native(1787) final function bool KeyEvent_s_Inactivity(string Key, EInputAction Action, FLOAT Delta);

native(1731) final function BeginState_s_BehindTheScene();
native(1732) final function PostRender_s_BehindTheScene(ECanvas Canvas);
native(1733) final function bool KeyEvent_s_BehindTheScene(string Key, EInputAction Action, FLOAT Delta);

native(1734) final function BeginState_s_UbiGame();
native(1735) final function PostRender_s_UbiGame(ECanvas Canvas);
native(1736) final function bool KeyEvent_s_UbiGame(string Key, EInputAction Action, FLOAT Delta);

native(1737) final function BeginState_s_RandomFact();
native(1738) final function PostRender_s_RandomFact(ECanvas Canvas);
native(1739) final function bool KeyEvent_s_RandomFact(string Key, EInputAction Action, FLOAT Delta);
/*-----------------------------------------------------------------------------
 Function:      PostBeginPlay
-----------------------------------------------------------------------------*/
function PostBeginPlay()
{
    Super.PostBeginPlay();
    
    if(EchelonMainHUD(owner) != None)
        myEchelonMainHUDOwner = EchelonMainHUD(owner);

    spawner = spawn(class'EPlayerInfo', self);
}

/*-----------------------------------------------------------------------------
 Function:      Tick
-----------------------------------------------------------------------------*/
function Tick(float DeltaTime)
{
    Super.Tick(DeltaTime);
    TickMainMenuHUD(DeltaTime);
}

/*-----------------------------------------------------------------------------
 Function:      OwnerGotoStateSafe

 Description:   Be able to switch owner state in c++
-----------------------------------------------------------------------------*/
event OwnerGotoStateSafe(name newState)
{
	if( newState == '' )
		newState = EchelonMainHUD(owner).RestoreState();
    myEchelonMainHUDOwner.GotoState(newState);
}

/*-----------------------------------------------------------------------------
 Function:      OwnerGotoStateSafe

 Description:   Be able to switch owner state in c++
-----------------------------------------------------------------------------*/
event OwnerDrawHUD(bool bDraw)
{
	Epc.bStopRenderWorld = !bDraw;
    myEchelonMainHUDOwner.bDrawMainHUD = bDraw;
}

event StopRender(bool bSet)
{	
	Epc.bStopRenderWorld = bSet;
}

/*-----------------------------------------------------------------------------
                                 S T A T E S
-----------------------------------------------------------------------------*/


/*=============================================================================
 State :        s_StartGame
=============================================================================*/
state s_StartGame
{
    function BeginState() {BeginState_s_StartGame();}
}


/*=============================================================================
 State :        s_RestoreSaveGame
=============================================================================*/
state s_RestoreSaveGame
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_RestoreSaveGame(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_RestoreSaveGame(Canvas);}

    function BeginState() {BeginState_s_RestoreSaveGame();}
}

/*=============================================================================
 State :        s_MainMenuState
=============================================================================*/
state s_MainMenu
{		
	function PostRender(ECanvas Canvas) {PostRender_s_MainMenu(Canvas);}

    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_MainMenu(Key, Action, Delta);}    

    function BeginState() {BeginState_s_MainMenu();}	

	function EndState() {EndState_s_MainMenu();}
}

/*=============================================================================
 State :        s_KeyBoard
=============================================================================*/
state s_KeyBoard
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_KeyBoard(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_KeyBoard(Canvas);}

    function BeginState() {BeginState_s_KeyBoard();}

    function Tick(float DeltaTime)
    {
        Global.Tick(DeltaTime);
        Tick_s_KeyBoard(DeltaTime);
    }
}

/*=============================================================================
 State :        s_LoadPlayersInfo
=============================================================================*/
//state s_LoadPlayersInfo
//{
//    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_LoadPlayersInfo(Key, Action, Delta);}

//    function PostRender(ECanvas Canvas) {PostRender_s_LoadPlayersInfo(Canvas);}

//    function Tick(float DeltaTime)
//    {
//        Global.Tick(DeltaTime);
//        Tick_s_LoadPlayersInfo(DeltaTime);
//    }

//    function BeginState() {BeginState_s_LoadPlayersInfo();}
//}

/*=============================================================================
 State :        s_SavePlayerInfo
=============================================================================*/
state s_SavePlayerInfo
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SavePlayerInfo(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_SavePlayerInfo(Canvas);}

    function Tick(float DeltaTime)
    {
        Global.Tick(DeltaTime);
        Tick_s_SavePlayerInfo(DeltaTime);
    }

    function BeginState() {BeginState_s_SavePlayerInfo();}
}


/*=============================================================================
 State :        s_Download
=============================================================================*/
state s_Download
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_Download(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_Download(Canvas);}

    function BeginState() {BeginState_s_Download();}
}

/*=============================================================================
 State :        s_Credit
=============================================================================*/
state s_Credit
{
	function PostRender(ECanvas Canvas) {PostRender_s_Credit(Canvas);}

    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_Credit(Key, Action, Delta);}       

    function BeginState() {BeginState_s_Credit();}
}

/*=============================================================================
 State :        s_PlayerMenu
=============================================================================*/
state s_PlayerMenu
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_PlayerMenu(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_PlayerMenu(Canvas);}

    function BeginState() {BeginState_s_PlayerMenu();}

	function Tick(float DeltaTime) 
	{ 
		Global.Tick(DeltaTime);
		Tick_s_PlayerMenu(DeltaTime);
	}
}

/*=============================================================================
 State :        s_PlayerSettings
=============================================================================*/
state s_PlayerSettings
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_PlayerSettings(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_PlayerSettings(Canvas);}

    function BeginState() {BeginState_s_PlayerSettings();}

	function EndState() {EndState_s_PlayerSettings();}	
}

/*=============================================================================
 State :        s_Controller
=============================================================================*/
state s_Controller
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_Controller(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_Controller(Canvas);}

    function BeginState() {BeginState_s_Controller();}

    function Tick(float DeltaTime)
    {
        Global.Tick(DeltaTime);
        Tick_s_Controller(DeltaTime);
    }
}

/*=============================================================================
 State :        s_SaveGame
=============================================================================*/
state s_SaveGame
{	
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SaveGame(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_SaveGame(Canvas);}

    function BeginState() {BeginState_s_SaveGame();}

	function EndState() {EndState_s_SaveGame();}

	function Tick(float DeltaTime)
    {
        Global.Tick(DeltaTime);
        Tick_s_SaveGame(DeltaTime);
    }
}

/*=============================================================================
 State :        s_LoadGame
=============================================================================*/
state s_LoadGame
{		
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_LoadGame(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_LoadGame(Canvas);}

    function BeginState() {BeginState_s_LoadGame();}

	function Tick(float DeltaTime)
    {
        Global.Tick(DeltaTime);
        Tick_s_LoadGame(DeltaTime);
    }
}

/*=============================================================================
 State :        s_SettingsMenu
=============================================================================*/
state s_SettingsMenu
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SettingsMenu(Key, Action, Delta);}

    function PostRender(ECanvas Canvas){ PostRender_s_SettingsMenu(Canvas);}

    function BeginState() {BeginState_s_SettingsMenu();}

	function Tick(float DeltaTime) 
	{ 
		Global.Tick(DeltaTime);
		Tick_s_SettingsMenu(DeltaTime);
	}
}

/*=============================================================================
 State :        s_SetHUDPosition
=============================================================================*/
state s_SetHUDPosition
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SetHUDPosition(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_SetHUDPosition(Canvas);}

    function BeginState() {BeginState_s_SetHUDPosition();}

    function EndState() {EndState_s_SetHUDPosition();}
    
}

/*=============================================================================
 State :        s_SetBrightness
=============================================================================*/
state s_SetBrightness
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SetBrightness(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_SetBrightness(Canvas);}

    function BeginState() {BeginState_s_SetBrightness();}

    function EndState() {EndState_s_SetBrightness();}
}

/*=============================================================================
 State :        s_PlayDemo
=============================================================================*/
state s_PlayDemo
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_PlayDemo(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_PlayDemo(Canvas);}

    function BeginState() {BeginState_s_PlayDemo();}
}

/*=============================================================================
 State :        s_Inactivity
=============================================================================*/
state s_Inactivity
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_Inactivity(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_Inactivity(Canvas);}

    function BeginState() {BeginState_s_Inactivity();}
}

/*=============================================================================
 State :        s_BehindTheScene
=============================================================================*/
state s_BehindTheScene
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_BehindTheScene(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_BehindTheScene(Canvas);}

    function BeginState() {BeginState_s_BehindTheScene();}
}


/*=============================================================================
 State :        s_UbiGame
=============================================================================*/
state s_UbiGame
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_UbiGame(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_UbiGame(Canvas);}

    function BeginState() {BeginState_s_UbiGame();}
}

/*=============================================================================
 State :        s_RandomFact
=============================================================================*/
state s_RandomFact
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_RandomFact(Key, Action, Delta);}
    
    function PostRender(ECanvas Canvas) {PostRender_s_RandomFact(Canvas);}

    function BeginState() {BeginState_s_RandomFact();}
}

