/******************************************************************************

 Class:         EchelonMainHUD

 Description:   EMainHUD interface implementation
                Separates the rendering stuff from Echelon Package


 Reference:     -

******************************************************************************/
class EchelonMainHUD extends EMainHUD native;

/*-----------------------------------------------------------------------------
                                 E X E C ' S
-----------------------------------------------------------------------------*/
#exec OBJ LOAD FILE=..\textures\HUD.utx PACKAGE=HUD
#exec OBJ LOAD FILE=..\textures\MAPS.utx PACKAGE=MAPS
#exec OBJ LOAD FILE=..\Sounds\CommonMusic.uax

/*-----------------------------------------------------------------------------
                      T Y P E   D E F I N I T I O N S 
-----------------------------------------------------------------------------*/

// LifeBar //
// (Yanick Mimee) June-27-2002 
const   LIFEBAR_WIDTH        = 17;
const   LIFEBAR_HEIGHT       = 194;
const   INTER_OFFSET_X       = 50;
const   INTER_OFFSET_Y       = 10;
const   INTER_OPT_BOX_OFFSET_Y = 35;
const   MAX_INTER_NAME_LENGHT = 20;


//////////////////////////////
// Computer interface 
//////////////////////////////
const SCREEN_WIDTH    = 640;
const SCREEN_HEIGHT   = 480;
const COMPUTER_WIDTH  = 510;
const COMPUTER_HEIGHT = 410;

const COMPUTER_X       = 65;
const COMPUTER_Y       = 35;

var float TimeElapsed;
var bool bIncreaseColor;
var int tmpColor;
const MAX_COLOR = 90;

// Interact icons //
const SPACE_BETWEEN_ICONS     = 5;

// Error msg box
const ERROR_BOX_WIDTH = 250;
const SPACING_ERROR_BOX = 50;

/*-----------------------------------------------------------------------------
                          M E M B E R   V A R I A B L E S 
-----------------------------------------------------------------------------*/
// Menu //
var EQInvHUD			QuickInvAndCurrentItems;
var EMainMenuHUD        MainMenuHUD;
var EGameMenuHUD		GameMenuHUD;

var int					TimerCounter;
var int					OldTimeCounter;

var EPlayerController	Epc;

var	EchelonGameInfo	    eGame;
var EchelonLevelInfo    eLevel;

var bool bDrawMainHUD;          // Used in configuration menu to draw HUD //
var bool bErrorFound;
var bool bRestoreState;

var bool bStockInMemory;

//var bool bSubMap;
//var bool bDisplaySplash;

native(1757) final function CheckError(ECanvas Canvas, bool bGameWasPause);

//native(1740) final function BeginState_s_LoadingScreen();
//native(1741) final function PostRender_s_LoadingScreen(ECanvas Canvas);
//native(1742) final function bool KeyEvent_s_LoadingScreen(string Key, EInputAction Action, FLOAT Delta);

/*-----------------------------------------------------------------------------
                                M E T H O D S
-----------------------------------------------------------------------------*/
event UpdateProfile()
{
	//MainMenuHUD.UpdateProfile();
}

event LoadProfile(String PlayerName)
{
	//MainMenuHUD.LoadProfile(PlayerName);
}

event SaveTInfo(int tab)
{
	//MainMenuHUD.SaveTInfo(tab);
}

event LoadTInfo(int tab)
{
	//MainMenuHUD.LoadTInfo(tab);
}

event SetPause(bool bSet)
{
	Epc.SetPause(bSet);
}

event StopRender(bool bSet)
{	
	Epc.bStopRenderWorld = bSet;
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
 Function:      PostBeginPlay

 Description:   -
-----------------------------------------------------------------------------*/
function PostBeginPlay()
{
	Epc = EPlayerController(Owner);
	if( Epc == None )
		Log("ERROR: invalid PlayerController for EchelonMainHud");

    QuickInvAndCurrentItems = spawn(class'EQInvHUD',self);
    GameMenuHUD             = spawn(class'EGameMenuHUD',self);
    
/*    
    MainMenuHUD             = spawn(class'EMainMenuHUD', self);

	// (Yanick Mimee) June-13-2002
	// Var initialization

	GameMenuHUD.TitleSectionInv = -1;
	GameMenuHUD.TitleSectionMainMenu = -1;
	GameMenuHUD.InvMenuDepth    = 0;
    GameMenuHUD.ItemPos         = -1;    
    GameMenuHUD.CurrentCategory = CAT_NONE;

		
	if (!EchelonGameInfo(Level.game).bDemoMode)
	{
		GameMenuHUD.nLastMenuPage = 's_Inventory';		
	}
	else
	{
		GameMenuHUD.nLastMenuPage = 's_GameInfo';
	}

	// (Yanick Mimee) June-13-2002
	GameMenuHUD.TitleSectionGameInfo  = -1; 

	// (Yanick Mimee) June-13-2002	
	GameMenuHUD.bGadgetVideoIsPlaying = false;
	GameMenuHUD.ReconDescScrollPos = 0;	
		
	GameMenuHUD.iIndexReconScroll = 0;
	GameMenuHUD.iIndexCurRoom = 0;	
	GameMenuHUD.iIndexRecon = 0;

	GameMenuHUD.bScrollUp   = true; // update the text the first time through
	GameMenuHUD.bScrollDown = false; // update the text the first time through

	GameMenuHUD.iStartPos = 0;
	GameMenuHUD.iOldStartPos = 0;
	GameMenuHUD.iEndPos = 0;
	GameMenuHUD.iOldEndPos = 0;  
	GameMenuHUD.bReconSelected = false;

	GameMenuHUD.bGameIsFinished = false;

	MainMenuHUD.bInactVideoPlaying = false;
*/    
	// Testing recons system		
	/*	
	Epc.AddRecon(class'EReconMapMinistry');
	Epc.AddRecon(class'EReconMapKalinatek');
	Epc.AddRecon(class'EReconMapTibilisi');
	Epc.AddRecon(class'EReconMapCIA1');
	Epc.AddRecon(class'EReconMapCIA2');
	Epc.AddRecon(class'EReconMap4_1');
	Epc.AddRecon(class'EReconMap3_2Cooling');	
	

	Epc.AddRecon(class'EReconPicGrinko');
	Epc.AddRecon(class'EReconPicMasse');
	Epc.AddRecon(class'EReconPicJackBaxter');
	Epc.AddRecon(class'EReconPicMitDoughert');	
	Epc.AddRecon(class'EReconPicAlekseevich');
	Epc.AddRecon(class'EReconPicCritavi');		

	Epc.AddRecon(class'EReconFullText3_2AF_A');
	Epc.AddRecon(class'EReconFullText3_2AF_B');
	Epc.AddRecon(class'EReconFullText5_1AF_A');	
	Epc.AddRecon(class'EReconFullText5_1AF_B');	
	Epc.AddRecon(class'EReconFullTextGugen');
	Epc.AddRecon(class'EReconFullTextBlaust');
	Epc.AddRecon(class'EReconFullTextMasse');
	Epc.AddRecon(class'EReconFullTextMadison');
	Epc.AddRecon(class'EReconFullTextJackBaxter');					   
	Epc.AddRecon(class'EReconFullTextMitDoughert');
	Epc.AddRecon(class'EReconFullTextCall911');
	Epc.AddRecon(class'EReconFullTextGrinko');	
	Epc.AddRecon(class'EReconFullTextUSAF');
	Epc.AddRecon(class'EReconFullTextAlek');
	Epc.AddRecon(class'EReconFullTextMissileDesc');
	Epc.AddRecon(class'EReconFullProgDesc');		

	Epc.AddRecon(class'EReconInfWaterValve');
	Epc.AddRecon(class'EReconInfCrudeOilCircuit');
	Epc.AddRecon(class'EReconInfCrudeOilRegulator');
	Epc.AddRecon(class'EReconInfPetrolRes');
	*/

	bErrorFound = false;

												
    eGame  = EchelonGameInfo(Level.Game);
    eLevel = EchelonLevelInfo(Level);

    Super.PostBeginPlay();

    /*
	if( Level.bIsStartMenu )
	{			
		if ( Epc.iErrorMsg == -3 )
		{			
			Epc.iErrorMsg = -2;		
		}
        GotoState('s_MainMenu');
	}
	else
	{				
		Epc.iErrorMsg = 0;
    */
        GotoState('MainHUD');
		//GotoState('s_LoadingScreen');		
	//}	
}

//------------------------------------------------------------------------
// Description		
//		
//------------------------------------------------------------------------
function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta );

//clauzon 9/11/2002 To change key mapping in Menus
function RealKeyEvent( string RealKeyValue, EInputAction Action, FLOAT Delta)
{
	//log("RealKeyEvent received in EchelonMainHUD " $ RealKeyValue);
	
	//To receive the message in the various Huds just pass the message
	//to the MainMenuHUD or	GameMenuHUD classes as necessary.
	//GameMenuHUD.RealKeyEvent(RealKeyValue, Action, Delta);
}

function DrawSaveLoadBox(ECanvas Canvas)
{
	if ( Epc.bSavingTraining )
	{
		if(Epc.bAutoSaveLoad)
			DrawErrorMsgBox( Canvas, Localize("HUD", "AUTOSAVING", "Localization\\HUD") );
		else
			DrawErrorMsgBox( Canvas, Localize("HUD", "QUICKSAVING", "Localization\\HUD") );
	}

	if ( Epc.bLoadingTraining )
	{
		if(Epc.bAutoSaveLoad)
			DrawErrorMsgBox( Canvas, Localize("HUD", "AUTOLOADING", "Localization\\HUD") );
		else
			DrawErrorMsgBox( Canvas, Localize("HUD", "QUICKLOADING", "Localization\\HUD") );
	}		
}

/*-----------------------------------------------------------------------------
 Function:      DrawDebugInfo

 Description:   -
-----------------------------------------------------------------------------*/
function DrawDebugInfo(ECanvas Canvas)
{    
	if	(Epc.bDebugInput)
	{
		//Canvas.Font = font'SmallFont';
		Canvas.SetPos(0,0);

		Epc.ShowDebugInput(Canvas);
	}

	if (Epc.bDebugStealth)
	{
		if(Epc.ePawn!=None)
		{
			Epc.Epawn.show_lighting_debug_info(Canvas);	
		}
	}
}

/*-----------------------------------------------------------------------------
 Function:      DrawLifeBar

 Description:   -
-----------------------------------------------------------------------------*/
function DrawLifeBar(ECanvas Canvas)
{
	local int LifeBarSize, xPos, yPos;
	local color Green;
	
	Green.R = 98;
	Green.G = 113;
	Green.B = 79;
	Green.A = 255;

    xPos = 640 - eGame.HUD_OFFSET_X - LIFEBAR_WIDTH;
    yPos = eGame.HUD_OFFSET_Y;

    Canvas.SetDrawColor(128,128,128);

	Canvas.Style=ERenderStyle.STY_Alpha;
    // TOP LEFT CORNER //
    Canvas.SetPos(xPos, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 8, 4, -8, -4);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(xPos, yPos+ LIFEBAR_HEIGHT - 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 0, 0, 8, 7);

    // TOP RIGHT CORNER //
    Canvas.SetPos(xPos + LIFEBAR_WIDTH - 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 0, 4, 8, -4);
    
    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(xPos + LIFEBAR_WIDTH - 8, yPos + LIFEBAR_HEIGHT - 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 8, 0, -8, 7);

    // LEFT BORDER //
    Canvas.SetPos(xPos, yPos + 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v,5, LIFEBAR_HEIGHT - 11, 0, 0, 5, 1);
    
    // RIGHT //    
    Canvas.SetPos(xPos + LIFEBAR_WIDTH - 5, yPos + 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v,5, LIFEBAR_HEIGHT - 11, 5, 0, -5, 1);
    
    // TOP //
    Canvas.SetPos(xPos + 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h2, LIFEBAR_WIDTH - 16, 4, 0, 0, 1, 4);

    // BOTTOM//
    Canvas.SetPos(xPos + 8, yPos + LIFEBAR_HEIGHT - 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h, LIFEBAR_WIDTH - 16, 6, 0, 0, 1, 6);
	Canvas.Style=ERenderStyle.STY_Normal;

    // BACKGROUND //		    	
	Canvas.SetPos(xPos + 5,yPos + 4);
	Canvas.DrawColor=Canvas.black;
	Canvas.DrawColor.A=40;
	Canvas.Style=ERenderStyle.STY_Alpha;	
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.lf_niv_plein, 8,LIFEBAR_HEIGHT - 10, 0, 0, 7, 1);
	

    // LIFE //
    if(Epc.Pawn != None && Epc.ePawn.Health > 0.0f)
    {
		// (Yanick Mimee) June-27-2002
		Canvas.SetDrawColor(128,128 + (127 * (1 - (float(Epc.ePawn.Health) / Epc.ePawn.InitialHealth))) ,128,255);	
		Canvas.Style=ERenderStyle.STY_Normal;	
		LifeBarSize = (LIFEBAR_HEIGHT - 10) * (float(Epc.ePawn.Health) / Epc.ePawn.InitialHealth);
        Canvas.SetPos(xPos + 4, yPos + LIFEBAR_HEIGHT - LifeBarSize - 7);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.lf_niv_plein, 8,LifeBarSize, 0, 0, 7, 1);		
	}	
}

/*-----------------------------------------------------------------------------
 Function:      DisplayInteractBox

 Description:   -
-----------------------------------------------------------------------------*/
function DisplayInteractBox(ECanvas Canvas, int iNbrOfInter, int iCurrentInter)
{
    local int xPos, yPos , INTER_BOX_WIDTH, INTER_BOX_HEIGHT, INTER_OPT_BOX_HEIGHT, INTER_OPT_BOX_WIDTH;
	local float xLen, yLen; 
    local EInteractObject	IO;


	Canvas.Font = Canvas.ETextFont;
    Canvas.SetDrawColor(128,128,128);	
    Canvas.TextSize("T", xLen, yLen);

	// Find box width with the max number of caracter in a description
	// Find the height of the option box with the number of interaction and the FONT height
	if ( Epc.egi.bInteracting )
	{
		INTER_OPT_BOX_HEIGHT = (iNbrOfInter - 1) * yLen + 3 + 3;
	}
	else
	{
		INTER_OPT_BOX_HEIGHT = iNbrOfInter * yLen + 3 + 3;
	}

	INTER_OPT_BOX_WIDTH  = xLen * MAX_INTER_NAME_LENGHT + 3 + 3;
	INTER_BOX_WIDTH = INTER_OPT_BOX_WIDTH;
	INTER_BOX_HEIGHT = yLen + 5 + 5 + 12;
		
	
	xPos = 640 - eGame.HUD_OFFSET_X - LIFEBAR_WIDTH - INTER_BOX_WIDTH;
    yPos = eGame.HUD_OFFSET_Y + INTER_BOX_HEIGHT + 1;
		

	IO = Epc.IManager.Interactions[iCurrentInter];
	if( IO != None )
	{	
		if( iCurrentInter == Epc.IManager.SelectedInteractions )
		{	
			// Draw selector		
			// When X is hold selector can go on BACK TO MAIN MENU
			if ( Epc.egi.bInteracting && (iCurrentInter == 0) )
			{
				Canvas.SetPos( 640 - eGame.HUD_OFFSET_X - LIFEBAR_WIDTH - INTER_BOX_WIDTH + 4 ,eGame.HUD_OFFSET_Y + (INTER_BOX_HEIGHT/2) - (yLen/2) );	
			}
			else
			{
				Canvas.SetPos( xPos + 2, yPos + 2 + ((iNbrOfInter - 1) * yLen ) - ( iCurrentInter * yLen ) );		
			}
			Canvas.Style=ERenderStyle.STY_Alpha;			
			eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_selecteur, INTER_BOX_WIDTH - 8, yLen, 0, 0, 1, 1);
			Canvas.Style=ERenderStyle.STY_Normal;

			// Color given by the interface artist (Veronique)
			Canvas.SetDrawColor(92,109,76);		
		}            
		else
		{		
			// Color given by the interface artist (Veronique)
			Canvas.SetDrawColor(49,56,40);
		}

		if ( Epc.egi.bInteracting && (iCurrentInter == 0) )
		{
			//Canvas.SetDrawColor(92,109,76);		
			Canvas.SetPos( 640 - eGame.HUD_OFFSET_X - LIFEBAR_WIDTH - INTER_BOX_WIDTH + 5 ,eGame.HUD_OFFSET_Y + (INTER_BOX_HEIGHT/2) - (yLen/2) );				
			Canvas.DrawTextAligned( Caps(IO.GetDescription() ) );		
		}
		else
		{
			Canvas.SetPos( xPos + 8, yPos + 2 + ((iNbrOfInter - 1) * yLen ) - ( iCurrentInter * yLen ) );		
			Canvas.DrawTextAligned( Caps(IO.GetDescription() ) );		
		}
	}		
}



/*-----------------------------------------------------------------------------
 Function:      DisplayInteractIcons

 Description:   -
-----------------------------------------------------------------------------*/
function DisplayInteractIcons(ECanvas Canvas, bool bSetPos)
{
    local int i, xPos, yPos, icon, iNbrOfInter;
    local EInteractObject	IO;

	local int INTER_OPT_BOX_HEIGHT, INTER_OPT_BOX_WIDTH, INTER_BOX_WIDTH, INTER_BOX_HEIGHT;
	local float xLen, yLen; 		

    Canvas.SetDrawColor(128,128,128);
    Canvas.Font = Canvas.EHUDFont;

    yPos  = SCREEN_HEIGHT - eGame.HUD_OFFSET_Y;
    iNbrOfInter = Epc.IManager.GetNbInteractions();

	// Display the interaction boxes
	if ( ( iNbrOfInter > 0 ) || ( bSetPos ) )
	{		
		Canvas.Font = Canvas.ETextFont;
		Canvas.SetDrawColor(128,128,128);	
		Canvas.TextSize("T", xLen, yLen);

		// Find box width with the max number of caracter in a description
		// Find the height of the option box with the number of interaction and the FONT height
		if ( Epc.egi.bInteracting )
		{
			INTER_OPT_BOX_HEIGHT = (iNbrOfInter - 1) * yLen + 3 + 3;
		}
		else
		{
			INTER_OPT_BOX_HEIGHT = iNbrOfInter * yLen + 3 + 3;
		}		

		INTER_OPT_BOX_WIDTH  = xLen * MAX_INTER_NAME_LENGHT + 3 + 3;
		INTER_BOX_WIDTH = INTER_OPT_BOX_WIDTH;
		INTER_BOX_HEIGHT = yLen + 5 + 5 + 12;

		xPos = 640 - INTER_BOX_WIDTH - eGame.HUD_OFFSET_X - LIFEBAR_WIDTH;
		yPos = eGame.HUD_OFFSET_Y;

		Canvas.Style=ERenderStyle.STY_Alpha;
		// Draw the top box with "INTERACT..." title
		// TOP LEFT CORNER 
		Canvas.SetPos(xPos, yPos);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 8, 4, -8, -4);

		// BOTTOM LEFT CORNER 
		Canvas.SetPos(xPos, yPos+ INTER_BOX_HEIGHT - 7);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 0, 0, 8, 7);

		// TOP RIGHT CORNER 
		Canvas.SetPos(xPos + INTER_BOX_WIDTH - 8, yPos);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 0, 4, 8, -4);
    
		// BOTTOM RIGHT CORNER
		Canvas.SetPos(xPos + INTER_BOX_WIDTH - 8, yPos + INTER_BOX_HEIGHT - 7);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 8, 0, -8, 7);

		// LEFT BORDER
		Canvas.SetPos(xPos, yPos + 4);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v,5, INTER_BOX_HEIGHT - 11, 0, 0, 5, 1);
    
		// RIGHT
		Canvas.SetPos(xPos + INTER_BOX_WIDTH - 5, yPos + 4);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v,5, INTER_BOX_HEIGHT - 11, 5, 0, -5, 1);
    
		// TOP
		Canvas.SetPos(xPos + 8, yPos);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h2, INTER_BOX_WIDTH - 16, 4, 0, 0, 1, 4);

		// BOTTOM
		Canvas.SetPos(xPos + 8, yPos + INTER_BOX_HEIGHT - 6);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h, INTER_BOX_WIDTH - 16, 6, 0, 0, 1, 6);
		
		// Fill the background
		Canvas.DrawLine(xPos + 4, yPos + 4, INTER_BOX_WIDTH - 8, INTER_BOX_HEIGHT - 8, Canvas.black, 200, eLevel.TGAME);				
				
		// Reset color
		// (Yanick Mimee) June-27-2002
		Canvas.SetDrawColor(128,128,128,255);	
		Canvas.Style=ERenderStyle.STY_Normal;	
		
		if ( !Epc.egi.bInteracting )
		{
			// Display "INTERACT"
			// Color given by the interface artist (Veronique)
			Canvas.SetDrawColor(49,56,40);
			Canvas.SetPos( xPos + 5 ,yPos + (INTER_BOX_HEIGHT/2) - (yLen/2) );
			Canvas.DrawTextAligned(Localize("HUD", "INTERACT", "Localization\\HUD"));

			
		
			// Draw the X button 						
			// Color given by the interface artist (Veronique)
			//Canvas.SetDrawColor(92,109,76);
			//Canvas.SetPos( 640 - eGame.HUD_OFFSET_X - LIFEBAR_WIDTH - 30 ,eGame.HUD_OFFSET_Y + (INTER_BOX_HEIGHT/2) - 11 );
			//eLevel.Tmenu.DrawTileFromManager(Canvas, eLevel.TMENU.but_s_a, 22, 22, 0, 0, 22, 22);		
		}										
				
		// Reset color
		Canvas.SetDrawColor(128,128,128,255);

		xPos = 640 - eGame.HUD_OFFSET_X - INTER_BOX_WIDTH - LIFEBAR_WIDTH;
		yPos = eGame.HUD_OFFSET_Y + INTER_BOX_HEIGHT + 1;

		Canvas.Style=ERenderStyle.STY_Alpha;

		// Draw the second box with the interaction messages	
		// TOP LEFT CORNER 
		Canvas.SetPos(xPos, yPos);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 0, 3, 3, -3);				

		// BOTTOM LEFT CORNER 		
		Canvas.SetPos(xPos, yPos+ INTER_OPT_BOX_HEIGHT - 3);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 0, 0, 3, 3);				

		// TOP RIGHT CORNER 
		Canvas.SetPos(xPos + INTER_OPT_BOX_WIDTH - 3, yPos);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 3, 3, -3, -3);		
    
		// BOTTOM RIGHT CORNER
		Canvas.SetPos(xPos + INTER_OPT_BOX_WIDTH - 3, yPos + INTER_OPT_BOX_HEIGHT - 3);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 3, 0, -3, 3);

		// LEFT BORDER
		Canvas.SetPos(xPos, yPos + 3);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_v,2, INTER_OPT_BOX_HEIGHT - 3, 0, 0, 2, 1);		
    
		// RIGHT
		Canvas.SetPos(xPos + INTER_OPT_BOX_WIDTH - 2, yPos + 3);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_v,2, INTER_OPT_BOX_HEIGHT - 3, 2, 1, -2, -1);		
    
		// TOP
		Canvas.SetPos(xPos + 3, yPos);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_h, INTER_OPT_BOX_WIDTH - 6, 2, 0, 0, 1, 2);		

		// BOTTOM
		Canvas.SetPos(xPos + 3, yPos + INTER_OPT_BOX_HEIGHT - 2);
		eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_h, INTER_OPT_BOX_WIDTH - 6, 2, 0, 0, 1, 2);		
		Canvas.Style=ERenderStyle.STY_Normal;

		// Fill the background
		Canvas.DrawLine(xPos + 2, yPos + 2, INTER_OPT_BOX_WIDTH - 4, INTER_OPT_BOX_HEIGHT - 4, Canvas.black, 200, eLevel.TMENU);
	}

	if ( !bSetPos )
	{
		for(i = 0; i < iNbrOfInter; i++)
		{		        
			DisplayInteractBox( Canvas, iNbrOfInter, i);        
		}
	}
}


/*-----------------------------------------------------------------------------
 Function:      DrawMainHUD

 Description:   -
-----------------------------------------------------------------------------*/
function DrawMainHUD(ECanvas Canvas)
{
	Canvas.Style     = ERenderStyle.STY_Normal;

	// Transmissions
	CommunicationBox.Draw(Canvas);

	// Life display
	DrawLifeBar(Canvas);

	// Quick Inventory	
	QuickInvAndCurrentItems.PostRender(Canvas);	

	// Interaction manager
	DisplayInteractIcons(Canvas, false);

	// Debug information
	DrawDebugInfo(Canvas);

    // Timer //
    if( TimerView != None )
        TimerView.PostRender(Canvas);
}

/*-----------------------------------------------------------------------------
 Function:      DrawInventoryItemInfo
    
 Description:   Draw any additionnal HUd info requested by InventoryItem
-----------------------------------------------------------------------------*/
function DrawInventoryItemInfo( ECanvas Canvas )
{
	// if item needs to add anything to the HUD, draw here
	if( EInventoryItem(Epc.ePawn.HandItem) != None && hud_master == None && Epc.GetStateName() != 's_Pickup' )
		EInventoryItem(Epc.ePawn.HandItem).DrawAdditionalInfo(self, Canvas);
}

/*-----------------------------------------------------------------------------
 Function:      DrawConfigMainHUD

 Description:   -
-----------------------------------------------------------------------------*/
function DrawConfigMainHUD(ECanvas Canvas)
{
	// Communication Box
	CommunicationBox.DrawConfig(Canvas);

	// Life Bar
	DrawLifeBar(Canvas);

    // Quick Inventory
	QuickInvAndCurrentItems.PostRender(Canvas);

	// Interactions    
	DisplayInteractIcons(Canvas, true);
}

event DrawErrorMsgBox( ECanvas Canvas, String sErrorMsg )
{
	local int xPos, yPos, iNbrOfLine, test, iErrorBoxHeight;    	
	local float xLen, yLen; 		

	// Compute usefull infos on text
	Canvas.Font = Canvas.ETextFont;
	Canvas.SetClip( ERROR_BOX_WIDTH - 100, yLen);
	Canvas.SetPos(0,0);
	Canvas.TextSize(sErrorMsg, xLen, yLen);
	iNbrOfLine = Canvas.GetNbStringLines(sErrorMsg, 1.0f);
	Canvas.SetClip(640, 480);

	iErrorBoxHeight = (iNbrOfLine * yLen) + SPACING_ERROR_BOX;

	xPos = SCREEN_WIDTH/2 - ERROR_BOX_WIDTH/2;
	yPos = SCREEN_HEIGHT/2 - iErrorBoxHeight/2;
	
	Canvas.Style=ERenderStyle.STY_Alpha;

    // FILL BACKGROUND //
    Canvas.DrawLine( (SCREEN_WIDTH/2 - ERROR_BOX_WIDTH/2) + 2,
		             (SCREEN_HEIGHT/2 - iErrorBoxHeight/2) + 2,
					 ERROR_BOX_WIDTH - 4,
					 iErrorBoxHeight - 4,
					 Canvas.white, -1, eLevel.TGAME);

    Canvas.SetDrawColor(128,128,128);
	
    // CORNERS //
    // TOP LEFT CORNER //
    Canvas.SetPos(xPos, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 0, 7, 8, -7);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(xPos, yPos + iErrorBoxHeight - 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 0, 0, 8, 7);

    // TOP RIGHT CORNER //
    Canvas.SetPos(xPos + ERROR_BOX_WIDTH - 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 8, 7, -8, -7);

    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(xPos + ERROR_BOX_WIDTH - 8, yPos + iErrorBoxHeight - 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 8, 0, -8, 7);

    // OUTSIDE BORDERS //

    // TOP BORDER //
    Canvas.SetPos(xPos + 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h, ERROR_BOX_WIDTH - 16, 6, 0, 6, 1, -6);

    // BOTTOM BORDER //
    Canvas.SetPos(xPos + 8, yPos + iErrorBoxHeight - 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h, ERROR_BOX_WIDTH - 16, 6, 0, 0, 1, 6);

    // LEFT BORDER //
    Canvas.SetPos(xPos, yPos + 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v, 5, iErrorBoxHeight - 14, 0, 0, 5, 1);

    // RIGHT BORDER //
    Canvas.SetPos(xPos + ERROR_BOX_WIDTH - 5, yPos + 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v, 5, iErrorBoxHeight - 14, 5, 0, -5, 1);
	
    // INSIDE BORDERS //	
    Canvas.DrawRectangle(xPos + 5, yPos + 6, ERROR_BOX_WIDTH - 10, iErrorBoxHeight - 12, 1, Canvas.black, 77, eLevel.TGAME);	

	Canvas.SetDrawColor(64,64,64,255);
    Canvas.Style = ERenderStyle.STY_Modulated;

    Canvas.SetPos(xPos + 5, yPos + 6);
    Canvas.DrawTile(Texture'HUD.HUD.ETMenuBar', ERROR_BOX_WIDTH - 10, iErrorBoxHeight - 12, 0, 0, 128, 2);	


	// Draw error message text			
	
	
	Canvas.SetDrawColor(128,128,128,255);
	Canvas.DrawColor = Canvas.TextBlack;
	Canvas.SetClip( ERROR_BOX_WIDTH - 100, yLen);
	Canvas.SetPos( xPos + (ERROR_BOX_WIDTH/2), yPos + (iErrorBoxHeight/2) - ((yLen * iNbrOfLine)/2) );			
	Canvas.DrawTextAligned( sErrorMsg, TXT_CENTER );		        

		
	Canvas.Style = ERenderStyle.STY_Normal;
}


/*=============================================================================
 State:         MainHUD
=============================================================================*/
state MainHUD
{
	function BeginState()
	{
		// Start rendering world when level loaded
		Epc.bStopRenderWorld = false;
	}

	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{		

		if( Action == IST_Press || Action == IST_Hold )
		{
			switch( Key )
			{
			case "QuickInventory" :
				if( Level.Pauser == None && Epc.CanAccessQuick() && !Epc.bStopInput )
				{
					SaveState();
					GotoState('QuickInventory');
					return true;
				}
				break;
			}
		}

		return false; // continue input processing
	} 

	function PostRender( Canvas C )
	{
		local ECanvas Canvas;
		
		Canvas = ECanvas(C);
    			
				
    					
		DrawMainHUD(Canvas);				

		// Draw item selected
		if( hud_master != None )
			hud_Master.DrawView(self,Canvas);

			DrawInventoryItemInfo(Canvas);

		DrawSaveLoadBox(Canvas);
		
		Super.PostRender(Canvas);
		
	}
}

/*=============================================================================
 State:         s_Slavery
=============================================================================*/
state s_Slavery
{
	function BeginState()
	{
		hud_master.HudView(true);
	}

	function EndState()
	{
		hud_master.HudView(false);
		hud_master = None;
	}

	function Slave( EGameplayObject NewMaster )
	{
		hud_master = NewMaster;
		BeginState();
	}

	function PostRender( Canvas C )
	{
		local ECanvas Canvas;
		Canvas = ECanvas(C);		

		if( hud_master != None )
			hud_master.DrawView(self, Canvas);

		if( !Epc.bLockedCamera )
		{
			if( Epc.GetStateName() == 's_FirstPersonTargeting'	|| 
				Epc.GetStateName() == 's_CameraJammerTargeting'	||
				Epc.GetStateName() == 's_RappellingTargeting'	||
				Epc.GetStateName() == 's_PlayerBTWTargeting'	||
				Epc.GetStateName() == 's_HOHTargeting'			||
				Epc.GetStateName() == 's_SplitTargeting'		||
				Epc.GetStateName() == 's_GrabTargeting'			||
				Epc.GetStateName() == 's_PlayerSniping' )
			{
				// Transmissions
				CommunicationBox.Draw(Canvas);

				// Life Bar
				DrawLifeBar(Canvas);

				if( Epc.GetStateName() != 's_PlayerSniping' )
				{
					// Quick Inventory
					QuickInvAndCurrentItems.PostRender(Canvas);					
				}
			}

			//Interactions
			if( Epc.GetStateName() == 's_FirstPersonTargeting' )
				DisplayInteractIcons(Canvas, false);
		}

			// Timer //
		if( TimerView != None )
			TimerView.PostRender(Canvas);
		

		DrawSaveLoadBox(Canvas);

        DrawDebugInfo(Canvas);

		CheckError(Canvas, Epc.GetPause());

		Super.PostRender(Canvas);
	}

	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{
		if( Action == IST_Press || 
			Action == IST_Hold )
		{
			switch( Key )
			{
			case "QuickInventory" :
				if( Epc.CanAccessQuick() &&
					!Epc.bStopInput && 
					Level.Pauser == None && 
					(Epc.GetStateName() == 's_FirstPersonTargeting'	||
					 Epc.GetStateName() == 's_RappellingTargeting'	||
					 Epc.GetStateName() == 's_SplitTargeting') )
				{
					SaveState();
					GotoState('QuickInventory');
					return true;
				}
				break;
			}
		}

		return false; // continue input processing
	} 
}

/*=============================================================================
 State:         s_MainMenu

 Description:   -
=============================================================================*/
state s_MainMenu
{
	Ignores FullInventory;

	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{				
        MainMenuHUD.DemoTimer = 0.0f;
		return MainMenuHUD.KeyEvent(Key,Action, Delta);
	}

	function PostRender( Canvas C )	
	{
		Local ECanvas Canvas;
		Canvas=ECanvas(C);

		Canvas.Style = ERenderStyle.STY_Normal;

        if(bDrawMainHUD)
            DrawConfigMainHUD(Canvas);

		MainMenuHUD.PostRender(Canvas);         		
        
        DrawDebugInfo(Canvas);

		CheckError(Canvas, Epc.GetPause());

		Super.PostRender(Canvas);
	}

	function BeginState()
	{
		StopAllSounds();
        Epc.SetPause(true);
		Epc.bStopRenderWorld = true;

		if ( GameMenuHUD.bGameIsFinished )
		{			
			GameMenuHUD.bGameIsFinished = false;
			MainMenuHUD.GotoState('s_StartGame');
		}
		else
		{
			if ( !EPC.bQuickLoad )
			{				
				MainMenuHUD.GotoState('s_StartGame');
			}
			else
			{
				MainMenuHUD.bFromStartMenu = true;
				MainMenuHUD.GotoState('s_LoadGame');
			}
		}
	}

    function EndState()
    {
        Epc.SetPause(false);
    }
}

/*=============================================================================
 State:         s_GameMenu

 Description:   -
=============================================================================*/
state s_GameMenu
{
	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{
        return GameMenuHUD.KeyEvent(Key, Action, Delta);
	}

	function PostRender( Canvas C )	
	{
		Local ECanvas Canvas;
		Canvas=ECanvas(C);
		Canvas.Style = ERenderStyle.STY_Normal;

        if(bDrawMainHUD)
            DrawConfigMainHUD(Canvas);

        GameMenuHUD.PostRender(Canvas);

        // Transmissions //
	    CommunicationBox.DrawMenuSpeech(Canvas);		

        DrawDebugInfo(Canvas);

		CheckError( Canvas, Epc.GetPause());

		Super.PostRender(Canvas);
	}

	function BeginState()
	{
		// Player just poped up game menu
        Epc.SetPause(true);

		Epc.EPawn.playsound(Sound'Interface.Play_ActionChoice', SLOT_Interface);
		
		// (Yanick Mimee) June-17-2002
		// Go back to the last menu page access.
		if ( Epc.bNewGoal || Epc.bNewNote || Epc.bNewRecon)
		{
			GameMenuHUD.bNbrOfSpotIsFound = false;
			GameMenuHUD.bScrollUp   = true;  // update the text the first time through
			GameMenuHUD.bScrollDown = false; // update the text the first time through
			GameMenuHUD.iStartPos    = 0; 
			GameMenuHUD.iOldStartPos = 0;
			GameMenuHUD.iEndPos      = 0;
			GameMenuHUD.iOldEndPos   = 0;
			GameMenuHUD.iIndexRecon  = 0;
			GameMenuHUD.ReconDescScrollPos = 0;			
			GameMenuHUD.iIndexReconScroll = 0;
			GameMenuHUD.iIndexCurRoom = 0;		
			GameMenuHUD.bReconSelected = false;  
			GameMenuHUD.bInsideSubMenu = false;  
			GameMenuHUD.TitleSectionInv = -1;
			GameMenuHUD.InvMenuDepth = 0;
			GameMenuHUD.ItemPos = 0;


			GameMenuHUD.nLastMenuPage = 's_gameInfo';
			if ( Epc.bNewGoal )
			{
				GameMenuHUD.TitleSectionGameInfo = 0;
			}
			else
			{
				if ( Epc.bNewRecon )
				{
					GameMenuHUD.TitleSectionGameInfo = 2;
				}			
				else
				{
					GameMenuHUD.TitleSectionGameInfo = 1;
				}
			}
		}
				
		GameMenuHUD.GoToState(GameMenuHUD.nLastMenuPage);		
	}

    function EndState()
    {
		// Player leaving game menu
        Epc.SetPause(false);

        GameMenuHUD.GoToState('');
    }

	function FullInventory()
	{
		GameMenuHUD.bNbrOfSpotIsFound = false;		
		ResumeSound();
		Epc.bStopRenderWorld = false;
		GotoState(RestoreState());
	}
}

/*=============================================================================
 State:         s_QuickSaveMenu

 Description:   -
=============================================================================*/
state s_QuickSaveMenu
{	
	Ignores FullInventory;

	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{

		if (!EchelonGameInfo(Level.game).bDemoMode)
			return MainMenuHUD.KeyEvent(Key, Action, Delta);
		else
			return GameMenuHUD.KeyEvent(Key, Action, Delta);
	}

	function PostRender( Canvas C )	
	{
		Local ECanvas Canvas;
		Canvas=ECanvas(C);
		Canvas.Style = ERenderStyle.STY_Normal;

		if (!EchelonGameInfo(Level.game).bDemoMode)
			MainMenuHUD.PostRender(Canvas);
		else
			GameMenuHUD.PostRender(Canvas);

		CheckError( Canvas, Epc.GetPause());

		Super.PostRender(Canvas);
	}

	function BeginState()
	{		
        Epc.SetPause(true);
		Epc.EPawn.playsound(Sound'Interface.Play_ActionChoice', SLOT_Interface);

		if (!EchelonGameInfo(Level.game).bDemoMode)
			MainMenuHUD.GoToState('s_SaveGame');		
		else
			GameMenuHUD.GoToState('s_QuickSave');		
	}

    function EndState()
    {
        Epc.SetPause(false);

		if (!EchelonGameInfo(Level.game).bDemoMode)
			MainMenuHUD.GoToState('');
		else
			GameMenuHUD.GoToState('');
    }
}

/*=============================================================================
 State:         s_QuickLoadMenu

 Description:   -
=============================================================================*/
state s_QuickLoadMenu
{
	Ignores FullInventory;

	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{       
		if (!EchelonGameInfo(Level.game).bDemoMode)
			return MainMenuHUD.KeyEvent(Key, Action, Delta);
		else
			return GameMenuHUD.KeyEvent(Key, Action, Delta);
	}

	function PostRender( Canvas C )	
	{
		Local ECanvas Canvas;
		Canvas=ECanvas(C);
		Canvas.Style = ERenderStyle.STY_Normal;

		if (!EchelonGameInfo(Level.game).bDemoMode)
			MainMenuHUD.PostRender(Canvas);
		else
			GameMenuHUD.PostRender(Canvas);

		CheckError( Canvas, Epc.GetPause());

		Super.PostRender(Canvas);
	}

	function BeginState()
	{
        Epc.SetPause(true);
		Epc.EPawn.playsound(Sound'Interface.Play_ActionChoice', SLOT_Interface);				

		if (!EchelonGameInfo(Level.game).bDemoMode)
			MainMenuHUD.GoToState('s_LoadGame');
		else
			GameMenuHUD.GoToState('s_QuickLoad');		
	}

    function EndState()
    {
        Epc.SetPause(false);

		if (!EchelonGameInfo(Level.game).bDemoMode)
			MainMenuHUD.GoToState('');
		else
			GameMenuHUD.GoToState('');
    }
}



/*=============================================================================
 State:         QuickInventory
=============================================================================*/
state QuickInventory
{
	Ignores SaveState;

	function BeginState()
	{
		QuickInvAndCurrentItems.GotoState('s_QDisplay');
	}

	function EndState()
	{
		if( QuickInvAndCurrentItems.GetStateName() == 's_QDisplay' )
			QuickInvAndCurrentItems.GotoState('');
	}

	function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{
		return QuickInvAndCurrentItems.KeyEvent(Key, Action, Delta);
	}

	function PostRender( Canvas C )
	{
		DrawMainHUD(ECanvas(C));
		
		CheckError( ECanvas(C), Epc.GetPause());
		Super.PostRender(C);
	}
}

/*=============================================================================
 State:         s_Training

 Description:   -
=============================================================================*/
state s_Training
{
    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{
        return GameMenuHUD.KeyEvent(Key, Action, Delta);
	}

	function PostRender( Canvas C )	
	{
		Local ECanvas Canvas;
		Canvas=ECanvas(C);
		Canvas.Style = ERenderStyle.STY_Normal;

		
			GameMenuHUD.PostRender(Canvas);


		CheckError( Canvas, Epc.GetPause());

        DrawDebugInfo(Canvas);

		Super.PostRender(Canvas);
	}

	function BeginState()
	{
        Epc.SetPause(true);
        GameMenuHUD.GoToState('s_Training');
    }

    function EndState()
    {
        Epc.SetPause(false);
        GameMenuHUD.GoToState('');
    }
}

// TEMP TEMP TEMP //
/*=============================================================================
 State:         s_Loading
=============================================================================*/
state s_Loading
{	
	Ignores FullInventory;

	function PostRender( Canvas C )	
	{
		local ECanvas Canvas;
		Canvas=ECanvas(C);

		Canvas.Style     = ERenderStyle.STY_Normal;
        
        MainMenuHUD.DrawLoading(Canvas);

		Super.PostRender(Canvas);
	}
}
// TEMP TEMP TEMP //

/*=============================================================================
 State:         s_Mission/Training Completed/Failed
=============================================================================*/
state s_Mission
{
	Ignores FullInventory, Slave, NormalView;

    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{
        return GameMenuHUD.KeyEvent(Key, Action, Delta);        
	}

	function bool IsPlayerGameOver()
	{
		return GameMenuHUD.GetStateName() == 's_MissionFailed';
	}

    function PostRender( Canvas C )	
	{
		local ECanvas Canvas;
		Canvas=ECanvas(C);

		Canvas.Style     = ERenderStyle.STY_Normal;
        
        GameMenuHUD.PostRender(Canvas);

		CheckError( Canvas, Epc.GetPause());

        DrawDebugInfo(Canvas);

		Super.PostRender(Canvas);
	}

Failed:
	StartFadeOut(40.0f);
	PlaySound(Sound'CommonMusic.Play_theme_Missionfailure', SLOT_Fisher);		
	StopRender(false);
    GameMenuHUD.GotoState('s_MissionFailed');
	Stop;

Complete:
	GameMenuHUD.CheckFinalMap();
	StartFadeOut(40.0f);
	StopRender(false);
	if(!GameMenuHUD.bFinalMap)
		PlaySound(Sound'CommonMusic.Play_theme_MissionSuccess', SLOT_Fisher);	
	GameMenuHUD.GotoState('s_MissionComplete');
	Sleep(5);
	Stop;
}

state s_TrainingFailed extends s_Mission
{
	Ignores KeyEvent;

    function EndState()
    {
        GameMenuHUD.GotoState('');
    }

Begin:
	GameMenuHUD.GotoState('s_TrainingFailed');
}

/*=============================================================================
 State:         Cinematic
=============================================================================*/
state s_Cinematic
{
	Ignores FullInventory;

   	function PostRender( Canvas C )
	{
		local ECanvas Canvas;

		Canvas = ECanvas(C);

        // Draw Black Line //
        Canvas.DrawLine(0, 0, 640, 60, Canvas.black, -1, eLevel.TMENU);
        Canvas.DrawLine(0, 480-60, 640, 60, Canvas.black, -1, eLevel.TMENU);

		DrawDebugInfo(Canvas);
        
        // Transmissions //
	    CommunicationBox.Draw(Canvas);

		CheckError( Canvas, Epc.GetPause());

		Super.PostRender(Canvas);
	}

    function BeginState()
    {		
        CommunicationBox.GotoState('s_Cinematic');        
    }

    function EndState()
    {
        CommunicationBox.GotoState('Idle');
    }

	// If Player is drawing weapon while cinematic starts, it will pop here ..
	// saving as if he was in slavery before cinemtic so that everything gets 
	// restored properly when coming back form cinematic
	function Slave( EGameplayObject NewMaster )
	{
		in_game_state_name	= 's_Slavery';
		in_game_hud_master	= NewMaster;
	}
}

defaultproperties
{
    bAlwaysTick=true
}