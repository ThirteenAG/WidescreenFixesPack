/******************************************************************************
 Class:         EGameMainMenuHUD

 Description:   In Game Main Menu HUD
******************************************************************************/
class EGameMainMenuHUD extends EMainMenuHUD native;

var EGameMenuHUD EGameMenuHUDOwner;
var bool bInsideSubMenu;

native(1789) final function TickGameMainMenuHUD(float DeltaTime);

native(2300) final function bool KeyEvent_s_LoadGame_MainMenu(string Key, EInputAction Action, FLOAT Delta);
native(2301) final function bool KeyEvent_s_SettingsMenu_MainMenu(string Key, EInputAction Action, FLOAT Delta);
native(2302) final function bool KeyEvent_s_PlayerSettings_MainMenu(string Key, EInputAction Action, FLOAT Delta);
native(2303) final function bool KeyEvent_s_SavePlayerInfo_MainMenu(string Key, EInputAction Action, FLOAT Delta);
native(2304) final function Tick_s_SavePlayerInfo_MainMenu(float DeltaTime);

native(1766) final function BeginState_s_ControllerInGame();
native(1767) final function Tick_s_ControllerInGame(float DeltaTime);
native(1768) final function PostRender_s_ControllerInGame(ECanvas Canvas);
native(1769) final function bool KeyEvent_s_ControllerInGame(string Key, EInputAction Action, FLOAT Delta);


/*-----------------------------------------------------------------------------
 Function:      PostBeginPlay

 Description:   -
-----------------------------------------------------------------------------*/
function PostBeginPlay()
{
    Super.PostBeginPlay();

    if(EGameMenuHUD(Owner) != None)
    {
        EGameMenuHUDOwner = EGameMenuHUD(Owner);

        Epc = EGameMenuHUD(Owner).Epc;
	    if( Epc == None )
		    Log("ERROR: Getting PlayerController in EGameMainMenuHUD");
    }

    if(EchelonMainHUD(owner.owner) != None)
        myEchelonMainHUDOwner = EchelonMainHUD(owner.owner);
}

/*-----------------------------------------------------------------------------
 Function:      Tick
-----------------------------------------------------------------------------*/
function Tick(float DeltaTime)
{
    Super(EMenuHUD).Tick(DeltaTime);
    TickGameMainMenuHUD(DeltaTime);
}

/*=============================================================================
 State :        s_LoadGame
=============================================================================*/
state s_LoadGame
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_LoadGame_MainMenu(Key, Action, Delta);}}

/*=============================================================================
 State :        s_SettingsMenu
=============================================================================*/
state s_SettingsMenu
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SettingsMenu_MainMenu(Key, Action, Delta);}
}

/*=============================================================================
 State :        s_PlayerSettings
=============================================================================*/
state s_PlayerSettings
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_PlayerSettings_MainMenu(Key, Action, Delta);}	
}

/*=============================================================================
 State :        s_SavePlayerInfo
=============================================================================*/
state s_SavePlayerInfo
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_SavePlayerInfo_MainMenu(Key, Action, Delta);}

    function Tick(float DeltaTime)
    {		
        Global.Tick(DeltaTime);
        Tick_s_SavePlayerInfo_MainMenu(DeltaTime);
    }
}

/*=============================================================================
 State :        s_Controller
=============================================================================*/
state s_Controller
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta ) {return KeyEvent_s_ControllerInGame(Key, Action, Delta);}

    function PostRender(ECanvas Canvas) {PostRender_s_ControllerInGame(Canvas);}

    function BeginState() {BeginState_s_ControllerInGame();}

    function Tick(float DeltaTime)
    {		
        Global.Tick(DeltaTime);
        Tick_s_ControllerInGame(DeltaTime);
    }
}
