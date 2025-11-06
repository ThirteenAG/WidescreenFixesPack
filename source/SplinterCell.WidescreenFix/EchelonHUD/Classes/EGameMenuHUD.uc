 /******************************************************************************

 Class:         EGameMenuHUD

 Description:   Game Menu HUD

******************************************************************************/
class EGameMenuHUD extends EMenuHUD native;

/*-----------------------------------------------------------------------------
                        T Y P E   D E F I N I T I O N S 
-----------------------------------------------------------------------------*/
const MAX_SECTION_TITLES = 4;
const NB_MISSION_NOISE_LINE = 6;

var int UCONST_NB_GAMEMENU_MAINMENU_SECTION;

/*-----------------------------------------------------------------------------
                        M E M B E R   V A R I A B L E S 
-----------------------------------------------------------------------------*/
var int MenuSection;        // 0 = Inventory, 1 = Goals And Notes, 2 = MainMenu
var int TitleSection;       
var int SubTitleSection;


// Main Menu //
var int MainMenuDepth;
var bool bGameMainMenu;
var bool bFinalMap;
var EGameMainMenuHUD GameMainMenuHUD;
var int TitleSectionMainMenu;

// GameInfo Menu //
var int GoalCurScroll;
var int GoalNbScroll;
var int NoteCurScroll;
var int NoteNbScroll;
var bool bComputeScroll;
var int TitleSectionGameInfo;

var string szSectionTitles[MAX_SECTION_TITLES];
var string szSubSectionTitles[MAX_SECTION_TITLES];

// Inventory Menu //
var	EInventory	 EpcInventory;
var eInvCategory CurrentCategory;
var int ItemPos;
var int InvMenuDepth;
var int TitleSectionInv;    // (Yanick Mimee) June-13-2002 
var bool bCheckVideo;
var bool bErrorIsCatch;
var bool bShowDesc;
var int  HowToUseScrollPos;
var int  HowToUseScrollMaxPos;

// Mission Failed//
var int missionFilterAlpha;
var float missionFilterTimer;
var float missionNoiseLineX[NB_MISSION_NOISE_LINE];
var float missionNoiseLineY[NB_MISSION_NOISE_LINE];
var int missionLinesAlpha;
var float missionTextScale;
var float missionTextScaleX;
var float missionTextScaleY;

// Training Failed and Complete//
var int blackAlpha;
var float blackAlphaTimer;
var bool  bIncrease;
var bool bDrawSaveMission;
var bool bStartTimer;
var float fDisplayTime;
var bool bSkipFrame;

// training //
var int TrainingDepth;

// (Yanick Mimee) June-17-2002
var bool bGadgetVideoIsPlaying; // flag to know that a video is playing
var string sVideoName; // Name of the video currently playing
var EInventoryItem CurrentItem;

var int  iNbrOfRecon;          // Nbr of recon in the list
var int  iNbrOfReconSpot;      // How many recon name we can put in the box
var bool bNbrOfSpotIsFound;    // Flag (see line above)
var bool bNeedToDrawScrollBar; // Flag to determine if we draw the SB or not
var int  iIndexReconScroll;    // To display the cursor.
var int  iIndexRecon;          // To know where we are in the list of recon?
var bool bScrollUp;            // Scrolling up
var bool bScrollDown;          // Scrolling down
var int iStartPos;
var int iOldStartPos;
var int iEndPos;
var int iOldEndPos;
var bool bReconSelected;       // Player has selected a RECON from the list

var ERecon tCurrentRecon;   // To save the current Recon pic
var int iIndexCurRoom;
var int ReconDescScrollPos;
var int ReconDescMaxPos;

// Error messages
var int iErrorType;

// Mission Failed and Succeed
var bool bMissionFailedToMenu;
var int  iNbrOfLetter;
var bool bGameIsFinished;
var string MissionFailedMsg;

// GameInfo
var bool bInsideSubMenu;



/*-----------------------------------------------------------------------------
                                M E T H O D S
-----------------------------------------------------------------------------*/
native(1179) final function CheckFinalMap();

// s_QuickSave //
native(1751) final function BeginState_s_QuickSave();
native(1752) final function bool KeyEvent_s_QuickSave(string Key, EInputAction Action, FLOAT Delta);
native(1753) final function PostRender_s_QuickSave(ECanvas Canvas);

// s_QuickLoad //
native(1754) final function BeginState_s_QuickLoad();
native(1755) final function bool KeyEvent_s_QuickLoad(string Key, EInputAction Action, FLOAT Delta);
native(1756) final function PostRender_s_QuickLoad(ECanvas Canvas);

// s_Inventory //
native(2310) final function BeginState_s_Inventory();
native(2311) final function bool KeyEvent_s_Inventory(string Key, EInputAction Action, FLOAT Delta);
native(2312) final function PostRender_s_Inventory(ECanvas Canvas);
native(2400) final function EndState_s_Inventory();

// s_GameInfo //
native(2313) final function BeginState_s_GameInfo();
native(2314) final function bool KeyEvent_s_GameInfo(string Key, EInputAction Action, FLOAT Delta);
native(2315) final function PostRender_s_GameInfo(ECanvas Canvas);

// s_MainMenu //
native(2319) final function BeginState_s_MainMenu();
native(2320) final function bool KeyEvent_s_MainMenu(string Key, EInputAction Action, FLOAT Delta);
native(2321) final function PostRender_s_MainMenu(ECanvas Canvas);

// s_Training //
native(2337) final function BeginState_s_Training();
native(2336) final function bool KeyEvent_s_Training(string Key, EInputAction Action, FLOAT Delta);
native(2329) final function PostRender_s_Training(ECanvas Canvas);

// s_MissionFailed //
native(2330) final function BeginState_s_MissionFailed();
native(1780) final function bool KeyEvent_s_MissionFailed(string Key, EInputAction Action, FLOAT Delta);
native(2331) final function PostRender_s_MissionFailed(ECanvas Canvas);
native(2332) final function Tick_s_MissionFailed(float DeltaTime);

// s_MissionComplete //
native(2333) final function BeginState_s_MissionComplete();
native(1781) final function bool KeyEvent_s_MissionComplete(string Key, EInputAction Action, FLOAT Delta);
native(2334) final function PostRender_s_MissionComplete(ECanvas Canvas);
native(2335) final function Tick_s_MissionComplete(float DeltaTime);


/*-----------------------------------------------------------------------------
     Function:      PostBeginPlay

     Description:   
-----------------------------------------------------------------------------*/
function PostBeginPlay()
{
    Super.PostBeginPlay();
    GameMainMenuHUD = spawn(class'EGameMainMenuHUD',self);

    EpcInventory = Epc.ePawn.FullInventory;
	if( EpcInventory == None )
		Log("Problem to get player inventory in Game Menu HUD.");
}

/*-----------------------------------------------------------------------------
 Function:      OwnerGotoStateSafe

 Description:   Be able to switch owner state in c++
-----------------------------------------------------------------------------*/
event OwnerGotoStateSafe( name newState )
{
	if( newState == '' )
		newState = EchelonMainHUD(owner).RestoreState();
    EchelonMainHUD(owner).GotoState(newState);
}


/*=============================================================================
 State :        s_QuickSave
=============================================================================*/

state s_QuickSave
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_QuickSave(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_QuickSave(Canvas);}
    
    function BeginState() {BeginState_s_QuickSave();}
}

/*=============================================================================
 State :        s_QuickLoad
=============================================================================*/

state s_QuickLoad
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_QuickLoad(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_QuickLoad(Canvas);}
    
    function BeginState() {BeginState_s_QuickLoad();}
}


/*=============================================================================
 State :        s_Inventory
=============================================================================*/
auto state s_Inventory
{
/*
	function PostRender(ECanvas Canvas)	{PostRender_s_Inventory(Canvas);}

    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_Inventory(Key, Action, Delta);}	
    
    function BeginState() {BeginState_s_Inventory();}

	function EndState() {EndState_s_Inventory();}
*/
}

/*=============================================================================
 State :        s_GameInfo
=============================================================================*/
state s_GameInfo
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_GameInfo(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_GameInfo(Canvas);}
    
    function BeginState() {BeginState_s_GameInfo();}
}

/*=============================================================================
 State :        s_MainMenu
=============================================================================*/
state s_MainMenu
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_MainMenu(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_MainMenu(Canvas);}
    
    function BeginState() {BeginState_s_MainMenu();}
}

/*=============================================================================
 State :        s_Training
=============================================================================*/
state s_Training
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_Training(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_Training(Canvas);}

    function BeginState() {BeginState_s_Training();}
}

/*=============================================================================
 State :        s_MissionFailed
=============================================================================*/
state s_MissionFailed
{
	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_MissionFailed(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_MissionFailed(Canvas);}
    
    function BeginState() {BeginState_s_MissionFailed();}

    function Tick(float DeltaTime) { Tick_s_MissionFailed(DeltaTime);}
}

/*=============================================================================
 State :        s_MissionComplete
=============================================================================*/
state s_MissionComplete
{
	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_MissionComplete(Key, Action, Delta);}

	function PostRender(ECanvas Canvas)	{PostRender_s_MissionComplete(Canvas);}
    
    function BeginState() {BeginState_s_MissionComplete();}

    function Tick(float DeltaTime) { Tick_s_MissionComplete(DeltaTime);}
}

/*=============================================================================
 State :        s_TrainingFailed
=============================================================================*/
state s_TrainingFailed
{
	function PostRender(ECanvas Canvas)
    {
        Canvas.DrawLine(0, 0, 640, 480, Canvas.black, blackAlpha, TGAME);
    }
    
    function BeginState()
    {
        bIncrease = true;
        blackAlpha = 0;
        blackAlphaTimer = 0.0f;
    }

    function Tick(float DeltaTime)
    {
        blackAlphaTimer += DeltaTime;

        if(bIncrease)
        {
            if(blackAlphaTimer < 1.5f)
            {
                blackAlpha = 255.0f * (blackAlphaTimer / 1.5f);
            }
            else if(blackAlphaTimer > 2.5f)
            {
                bIncrease = false;
                blackAlphaTimer = 0.0f;
            }
            else
            {
                blackAlpha = 255.0f;
            }
        }
        else
        {
            if(blackAlphaTimer < 1.5)
            {
                blackAlpha = 255.0f * (1.0f - (blackAlphaTimer / 1.5f));
            }
            else
            {
                blackAlpha = 0.0f;
                OwnerGotoStateSafe('MainHUD');
            }
        }
    }    
}


