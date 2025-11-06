 /*=============================================================================

 Class:         EMenuHUD

 Description:   Menu HUD

 Reference:     -

=============================================================================*/

class EMenuHUD extends Actor native;

#exec OBJ LOAD FILE=..\Sounds\Interface.uax

/*-----------------------------------------------------------------------------
                        T Y P E   D E F I N I T I O N S 
-----------------------------------------------------------------------------*/

const NB_NOISE_LINE = 3;
const NB_LANGUAGE = 6;                  // Make this value a multiple of 3 to avoid problem //

enum MSGBOX_RETURN{IDYES, IDNO, IDOK, IDEASY, IDNORMAL, IDHARD, IDNONE, IDCONTINUE, IDBACKTOGAME, IDBACK};
enum MSGBOX_TYPE{ID_YESNO, ID_OK, ID_DIFFICULTY, ID_LOAD, ID_CONTINUE, ID_NOTHING, ID_OVERWRITE};

enum XBOX_BUTTON{BUT_NONE, BUT_A, BUT_B, BUT_X, BUT_Y, but_start};
enum BKG_COLOR {BKG_NONE, BKG_REG, BKG_PALE, BKG_DARK};

var int UCONST_NB_MAINMENU_ITEM;

/*-----------------------------------------------------------------------------
                        M E M B E R   V A R I A B L E S 
-----------------------------------------------------------------------------*/
var	EchelonGameInfo	    eGame;
var EchelonLevelInfo    eLevel;
var EPlayerController	Epc;
var ETextureManager     TMANAGER_MENU;
var ETMENU              TMENU;
var ETextureManager     TMANAGER_GAME;
var ETGAME              TGAME;
var ETICON              TICON;
var Material            PdaBorder;
var Texture             EchelonLogo;
var Texture             HUDBackground;
var Texture             ETPixel;
var Texture             PdaBackGround;
var Texture             MenuBar;

var int navAlpha;
var float navAlphaTimer;
var bool bNavAlphaIncrease;

var int depth;
var int Row;
var int Column;
var int Cursor;
var int topDepth;
var int oldScheme;

var	sound	BeepSound1;
var	sound	BeepSound2;
var	sound	BeepSound3;
var Sound   MenuMusic;

var bool bNavLeft, bNavUp, bNavRight, bNavDown;

var string szTempPlayerName;
var int curAlpha;
var float curAlphaTimer;
var bool bCurAlphaIncrease;

var float fNoiseLine[NB_NOISE_LINE];
var string szLanguage[NB_LANGUAGE];

// CONTROLLER MENU //
var int RegularControllerOffSet, controllerOffset;
var bool bControllerLeftTransition, bControllerRightTransition;
var float controllerTimer;
var int holdController;


// Message box //
var bool bMessageBox;
var int MsgType;
var int msgPos;

var color borderColor;

// (Yanick Mimee) June-13-2002
var name nLastMenuPage; // Keep track of the last menu page consulted

var bool bBlink;
var float blinkTimer;


/*-----------------------------------------------------------------------------
                                M E T H O D S
-----------------------------------------------------------------------------*/

event bool KeyEvent(string Key, EInputAction Action, FLOAT Delta);
event PostRender(ECanvas Canvas);

/*-----------------------------------------------------------------------------
 Function:      PostBeginPlay

 Description:   -
-----------------------------------------------------------------------------*/
function PostBeginPlay()
{
    Super.PostBeginPlay();

    eGame  = EchelonGameInfo(Level.Game);
    eLevel = EchelonLevelInfo(Level);

    if(EPlayerController(Owner.Owner) != None)
    {
        Epc = EPlayerController(Owner.Owner);
	    if( Epc == None )
		    Log("ERROR: Getting PlayerController in EMenuHUD");
    }

    navAlpha            = 255 * 0.80f;
    navAlphaTimer       = 0.0f;
    bNavAlphaIncrease   = false;

    curAlpha            = 255 * 0.30f;
    curAlphaTimer       = 0.0f;
    bCurAlphaIncrease   = false;

    szLanguage[0] = "ENGLISH";
    szLanguage[1] = "FRANCAIS";
    szLanguage[2] = "DEUTSH";
    szLanguage[3] = "ESPANOL";
    szLanguage[4] = "ITALIANO";

    TMENU         = eLevel.TMENU;
    TMANAGER_MENU = eLevel.TMENU;
    TGAME         = eLevel.TGAME;
    TMANAGER_GAME = eLevel.TGAME;
    TICON         = eLevel.TICON;
}

/*-----------------------------------------------------------------------------
 Function:      Tick

 Description:   -
-----------------------------------------------------------------------------*/
function Tick(float DeltaTime)
{
    TickMenuHUD(DeltaTime);
}

/*-----------------------------------------------------------------------------
 Function:      GotoStateSafe

 Description:   To be able to switch state in c++
-----------------------------------------------------------------------------*/
event GotoStateSafe( name State )
{
	GotoState(State);
}

/*-----------------------------------------------------------------------------
 Function:      GetStateNameSafe

 Description:   To be able to get state name in c++
-----------------------------------------------------------------------------*/
event name GetStateNameSafe()
{
	return GetStateName();
}

/*-----------------------------------------------------------------------------
 Function:      SendConsoleCommand

 Description:   To be able to send console command in c++
-----------------------------------------------------------------------------*/
event string SendConsoleCommand(string command)
{
	return ConsoleCommand(command);
}

native(1397) final function TickMenuHUD(float DeltaTime);

native(1337) final function DrawLoading(ECanvas Canvas);

defaultproperties
{
    PdaBorder=Shader'HUD.HUD.ETMENU_specu'
    EchelonLogo=Texture'HUD.HUD.logo'
    HUDBackground=Texture'HUD.HUD.ETMenuBar'
    ETPixel=Texture'HUD.HUD.ETPixel'
    PdaBackGround=Texture'HUD.HUD.palm_bkg'
    MenuBar=Texture'HUD.HUD.ETMenuBar'
    BeepSound1=Sound'Interface.Play_ActionChoice'
    BeepSound2=Sound'Interface.Play_ActionChoice'
    BeepSound3=Sound'Interface.Play_Validate'
    MenuMusic=Sound'CommonMusic.Play_theme_Menu'
    bHidden=true
    bAlwaysTick=true
}