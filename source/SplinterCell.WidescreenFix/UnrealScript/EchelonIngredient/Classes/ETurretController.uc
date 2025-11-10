class ETurretController extends EGameplayObject;

#exec OBJ LOAD FILE=..\textures\HUD.utx PACKAGE=HUD
#exec new TrueTypeFontFactory Name=Verdana9 FontName="Verdana" Height=9 AntiAlias=0 CharactersPerPage=64

/*-----------------------------------------------------------------------------
                      T Y P E   D E F I N I T I O N S 
-----------------------------------------------------------------------------*/
// Turret general //
const SCREEN_END_X              = 640;

const LBAR_WIDTH                = 17; // Width of lifebar 

const TURRET_WIDTH              = 208;
const TURRET_HEIGHT             = 144;
const TURRET_BORDER_WIDTH       = 5;

// Middle Box //
const MIDDLEBOX_OFFSET_X        = 5;
const MIDDLEBOX_OFFSET_Y        = 34;
const MIDDLEBOX_WIDTH           = 187;
const MIDDLEBOX_HEIGHT          = 96;

// Inside black borders //
const INSIDE_BOX_OFFSET_X       = 9;
const INSIDE_BOX_OFFSET_Y1      = 8;
const INSIDE_BOX_OFFSET_Y2      = 72;
const INSIDE_BOX_HEIGHT1        = 61;
const INSIDE_BOX_HEIGHT2        = 17;

// Text //
const TEXT_POS_X        = 27;
const TEXT_DEACTIVATE_Y = 21;
const TEXT_DISABLE_Y    = 45;
const TEXT_EXIT         = 73;

const TEXT_ROTATION_TIME        = 0.1f;     // Set the speed of animated text

const DEACTIVATE  = 0;
const DISABLE_IFF = 1;
const EXIT        = 2;
const OUTSIDE     = 3;

/*-----------------------------------------------------------------------------
                   I N S T A N C E   V A R I A B L E S
-----------------------------------------------------------------------------*/
var() ETurret			LinkedTurret;

var string				szHackingText, szTopBynaryText, szBottomBynaryText;
var int					iHackingIndex, CursorPos;
var float				fLastRotationTime;

var EchelonLevelInfo	eLevel;
var EchelonGameInfo		eGame;

var int					TURRET_OFFSET_X;     // Offset from right side of the screen
var int					TURRET_OFFSET_Y;     // Offset from the top of the screen

var bool				bCheckMouse;

function PostBeginPlay()
{
	Super.PostBeginPlay();

    eLevel = EchelonLevelInfo(Level);
    eGame  = EchelonGameInfo(Level.Game);

    TURRET_OFFSET_X = eGame.HUD_OFFSET_X;
    TURRET_OFFSET_Y = eGame.HUD_OFFSET_Y + 25;

    szHackingText = "";
    iHackingIndex = 0;
    szTopBynaryText    = "100111001011010010101110010";
    szBottomBynaryText = "001101011011010001101010101";

    CursorPos = EXIT;
}

function BeginEvent()
{
    bCheckMouse = true;
}
function EndEvent()
{
    bCheckMouse = false;
}

function bool CoordinateWithin(EPlayerController Epc, float x, float y, int w, int h)
{
	return Epc.m_FakeMouseX > x && Epc.m_FakeMouseX < x + w && 
		   Epc.m_FakeMouseY > y && Epc.m_FakeMouseY < y + h;
}

function Tick( float DeltaTime )
{
	local int OldCurPos;
	local EPlayerController Epc;
	Epc = EPlayerController(ETurretInteraction(Interaction).User);

	OldCurPos = CursorPos;

	if( Epc == None || !bCheckMouse )
		return;

	//
	// Crappy button selection
	//
	if( CoordinateWithin(Epc, 420, 95, 130, 20) )
		CursorPos = DEACTIVATE;
	else if( CoordinateWithin(Epc, 420, 120, 130, 20) )
		CursorPos = DISABLE_IFF;
	else if( CoordinateWithin(Epc, 420, 150, 130, 17) )
		CursorPos = EXIT;
	else if( !CoordinateWithin(Epc, 390, 40, 208, 140) )
		CursorPos = OUTSIDE;
	else
		CursorPos = -1;


	// Change selection
	if( OldCurPos != CursorPos )
		PlaySound(Sound'Interface.Play_ActionChoice', SLOT_Interface);

	//
	// Manage Mouse click
	//
	if( Epc.m_FakeMouseClicked )
	{
		switch( CursorPos )
		{
			case DEACTIVATE :
    			if( LinkedTurret.GetStateName() == 's_Deactivated' )
				{
					LinkedTurret.GotoState('s_Patrol');
					LinkedTurret.PlaySound(LinkedTurret.AmbientPlaySound, SLOT_SFX);
					LinkedTurret.bInAmbientRange = true;
				}
				else
					LinkedTurret.GotoState('s_Deactivated');
				break;

			case DISABLE_IFF :
				if( LinkedTurret.SensorDetectionType != SCAN_AllChangedActors )
					LinkedTurret.SensorDetectionType = SCAN_AllChangedActors;
				else
					LinkedTurret.SensorDetectionType = SCAN_AllPawnsAndChangedActors;
				break;

			case EXIT :
			case OUTSIDE :
				Interaction.PostInteract(ETurretInteraction(Interaction).User);
				break;
			default:
				break;
		}
	}
	Epc.m_FakeMouseClicked = false;
}

/*-----------------------------------------------------------------------------
 Function:      KeyEvent

 Description:   -
-----------------------------------------------------------------------------*/
function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
{
	if(Action == IST_Press)
	{
		switch( Key )
		{
			case "DPadUp":
			case "AnalogUp":
			case "MoveForward" :
				PlaySound(Sound'Interface.Play_ActionChoice', SLOT_Interface);
				if( CursorPos > DEACTIVATE )
					CursorPos--;
				break;

			case "DPadDown":
			case "AnalogDown":
			case "MoveBackward" :
				PlaySound(Sound'Interface.Play_ActionChoice', SLOT_Interface);
				if( CursorPos < EXIT )
					CursorPos++;
				break;

            case "Interaction" :
				PlaySound(Sound'Interface.Play_Validate', SLOT_Interface);
			    switch( CursorPos )
			    {
			        case DEACTIVATE :
    				    if( LinkedTurret.GetStateName() == 's_Deactivated' )
						{
					        LinkedTurret.GotoState('s_Patrol');
							LinkedTurret.PlaySound(LinkedTurret.AmbientPlaySound, SLOT_SFX);
							LinkedTurret.bInAmbientRange = true;
						}
				        else
					        LinkedTurret.GotoState('s_Deactivated');
				        break;

			        case DISABLE_IFF :
				        if( LinkedTurret.SensorDetectionType != SCAN_AllChangedActors )
							LinkedTurret.SensorDetectionType = SCAN_AllChangedActors;
						else
							LinkedTurret.SensorDetectionType = SCAN_AllPawnsAndChangedActors;
				        break;

			        case EXIT :
				        Interaction.PostInteract(ETurretInteraction(Interaction).User);
				        break;
			    }
			    break;
		}
	}
    
    return false;
}

//---------------------------------------[David Kalina - 13 Nov 2001]-----
// 
// Description
//		Turn on Friend-or-Foe detection (triggered by NPCs)
//
//------------------------------------------------------------------------
function EnableFOF()
{
	LinkedTurret.SensorDetectionType = SCAN_AllChangedActors;
	LinkedTurret.GotoState('s_Patrol');
}

/*-----------------------------------------------------------------------------
 Function:      DrawView

 Description:   -
-----------------------------------------------------------------------------*/
function DrawView( HUD hud, ECanvas Canvas )
{
    DrawBackGround(Canvas);
    DrawBox(Canvas);
    DrawAnimText(Canvas);
}

/*-----------------------------------------------------------------------------
    Function :      DrawBackGround 

    Description:    -
-----------------------------------------------------------------------------*/
function DrawBackGround(ECanvas Canvas)
{
	local int xPos, yPos;

    Canvas.SetDrawColor(128, 128, 128);	

	xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - LBAR_WIDTH;
    yPos = eGame.HUD_OFFSET_Y;

	// TOP LEFT CORNER //
    Canvas.SetPos(xPos - TURRET_WIDTH, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_coin, 6, 6, 0, 6, 6, -6);

    // TOP RIGHT CORNER //
    Canvas.SetPos(xPos - 6, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_coin, 6, 6, 6, 6, -6, -6);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(xPos - TURRET_WIDTH, yPos + TURRET_HEIGHT - 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_coin, 6, 6, 0, 0, 6, 6);

    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(xPos - 6, yPos + TURRET_HEIGHT - 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_coin, 6, 6, 6, 0, -6, 6);

    // TOP BORDER //
    Canvas.SetPos(xPos - TURRET_WIDTH + 6, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_border_h, TURRET_WIDTH - 2*6, 5, 0, 5, 1, -5);

    // BOTTOM BORDER //
    Canvas.SetPos(xPos - TURRET_WIDTH + 6, yPos + TURRET_HEIGHT - 5);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_border_h, TURRET_WIDTH - 2*6, 5, 0, 0, 1, 5);

    // LEFT BORDER //
    Canvas.SetPos(xPos - TURRET_WIDTH, yPos + 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_border_v, 5, TURRET_HEIGHT - 2*6, 0, 0, 5, 1);

    // RIGHT BORDER //
    Canvas.SetPos(xPos - 5, yPos + 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sw_border_v, 5, TURRET_HEIGHT - 2*6, 5, 0, -5, 1);

    // FILL BACKGROUND IN BLACK //
    Canvas.DrawLine(xPos - TURRET_WIDTH + TURRET_BORDER_WIDTH, yPos + TURRET_BORDER_WIDTH,
                     TURRET_WIDTH - TURRET_BORDER_WIDTH*2, TURRET_HEIGHT - TURRET_BORDER_WIDTH*2, Canvas.black, -1, eLevel.TGAME);	
}

/*-----------------------------------------------------------------------------
 Function:      DrawBox

 Description:   -
-----------------------------------------------------------------------------*/
function DrawBox(ECanvas Canvas)
{
    local int xPos, yPos;

	xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - TURRET_WIDTH - LBAR_WIDTH + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_X;
    yPos = eGame.HUD_OFFSET_Y + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_Y;

    // Background //
	Canvas.Style = ERenderStyle.STY_Alpha;
	Canvas.SetDrawColor(128,128,128);
    // FILL BACKGROUND IN GREEN //
    Canvas.DrawLine(xPos + 5, yPos + 5, MIDDLEBOX_WIDTH - 2*5, MIDDLEBOX_HEIGHT - 9, Canvas.white, -1, eLevel.TGAME);
    Canvas.DrawLine(xPos + 5, yPos + 5, MIDDLEBOX_WIDTH - 2*5, MIDDLEBOX_HEIGHT - 9, Canvas.black, 255 * 0.30f, eLevel.TGAME);

    // Outside Borders //

    Canvas.SetDrawColor(128,128,128);

    // TOP LEFT CORNER //
    Canvas.SetPos(xPos, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 6, 6, 0, 6, 6, -6);

    // TOP RIGHT CORNER //
    Canvas.SetPos(xPos + MIDDLEBOX_WIDTH - 6, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 6, 6, 6, 6, -6, -6);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(xPos, yPos + MIDDLEBOX_HEIGHT - 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 8, 0, -8, 4);

    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(xPos + MIDDLEBOX_WIDTH - 8, yPos + MIDDLEBOX_HEIGHT - 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 0, 0, 8, 4);

    // TOP BORDER //
    Canvas.SetPos(xPos + 6, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h, MIDDLEBOX_WIDTH - 2*6, 5, 0, 5, 1, -5);

    // BOTTOM BORDER //
    Canvas.SetPos(xPos + 8, yPos + MIDDLEBOX_HEIGHT - 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h2, MIDDLEBOX_WIDTH - 2*8, 4, 0, 0, 1, 4);

    // LEFT BORDER //
    Canvas.SetPos(xPos, yPos + 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v, 4, MIDDLEBOX_HEIGHT - 10, 0, 0, 4, 1);

    // RIGHT BORDER //
    Canvas.SetPos(xPos + MIDDLEBOX_WIDTH - 4, yPos + 6);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v, 4, MIDDLEBOX_HEIGHT - 10, 4, 0, -4, 1);

    // Inside Borders //

    DrawBlackBorders(Canvas, xPos + INSIDE_BOX_OFFSET_X, yPos + INSIDE_BOX_OFFSET_Y1, xPos + MIDDLEBOX_WIDTH - INSIDE_BOX_OFFSET_X,  yPos + INSIDE_BOX_OFFSET_Y1 + INSIDE_BOX_HEIGHT1);
    DrawBlackBorders(Canvas, xPos + INSIDE_BOX_OFFSET_X, yPos + INSIDE_BOX_OFFSET_Y2, xPos + MIDDLEBOX_WIDTH - INSIDE_BOX_OFFSET_X,  yPos + INSIDE_BOX_OFFSET_Y2 + INSIDE_BOX_HEIGHT2);

    // Info Text //

    DrawInfo(Canvas);

    // FILTER //
	Canvas.Style = ERenderStyle.STY_Alpha;
    Canvas.SetDrawColor(128,128,128);
    Canvas.SetPos(xPos + 5, yPos + 5);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_ombre, MIDDLEBOX_WIDTH - 2*5, MIDDLEBOX_HEIGHT - 9, 0, 1, 166, 4);

    // GREEN BACKGROUND //
    Canvas.SetDrawColor(64,64,64,255);
    Canvas.Style = ERenderStyle.STY_Modulated;

    Canvas.SetPos(xPos + 5, yPos + 5);
    Canvas.DrawTile(Texture'HUD.HUD.ETMenuBar', MIDDLEBOX_WIDTH - 2*5, MIDDLEBOX_HEIGHT - 9, 0, 0, 128, 2);

    Canvas.Style = ERenderStyle.STY_Normal;
}

/*-----------------------------------------------------------------------------
 Function:      DrawBlackBorders

 Description:   -
-----------------------------------------------------------------------------*/
function DrawBlackBorders(ECanvas Canvas, int AX, int AY, int BX, int BY)
{
    Canvas.DrawLine(AX + 1, AY + 1, BX-AX - 2, BY-AY - 2, Canvas.white, -1, eLevel.TGAME);

    Canvas.SetDrawColor(128,128,128);
	Canvas.Style = ERenderStyle.STY_Alpha;
    // TOP LEFT CORNER //
    Canvas.SetPos(AX, AY);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_coin, 8, 8, 0, 0, 8, 8);

    // TOP RIGHT CORNER //
    Canvas.SetPos(BX - 8, AY);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_coin, 8, 8, 8, 0, -8, 8);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(AX, BY - 8);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_coin, 8, 8, 0, 8, 8, -8);

    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(BX - 8, BY - 8);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_coin, 8, 8, 8, 8, -8, -8);

    // TOP BORDER //
    Canvas.SetPos(AX + 8, AY);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, BX - AX - 2*8, 1, 0, 0, 1, 1);

    // BOTTOM BORDER //
    Canvas.SetPos(AX + 8, BY - 1);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, BX - AX - 2*8, 1, 0, 0, 1, 1);

    // LEFT BORDER //
    Canvas.SetPos(AX, AY + 8);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 1, BY - AY - 2*8, 0, 0, 1, 1);

    // RIGHT BORDER //
    Canvas.SetPos(BX - 1, AY + 8);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 1, BY - AY - 2*8, 0, 0, 1, 1);
	Canvas.Style = ERenderStyle.STY_Normal;
}

/*-----------------------------------------------------------------------------
 Function:      DrawInfo

 Description:   -
-----------------------------------------------------------------------------*/
function DrawInfo(ECanvas Canvas)
{
    local int xPos, yPos;
	local float xLen, yLen;

    Canvas.Font = Canvas.ETextFont;
	
    xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - LBAR_WIDTH - TURRET_WIDTH + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_X;
    yPos = eGame.HUD_OFFSET_Y + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_Y;

	Canvas.Style = ERenderStyle.STY_Alpha;
	Canvas.SetDrawColor(128,128,128);
    // FILL SELECTED SECTION BACKGROUND //
    if(CursorPos == EXIT)
        Canvas.DrawLine(xPos + INSIDE_BOX_OFFSET_X + 1, yPos + INSIDE_BOX_OFFSET_Y1, MIDDLEBOX_WIDTH - 2*(INSIDE_BOX_OFFSET_X + 1), INSIDE_BOX_HEIGHT1, Canvas.black, 255 * 0.20f, eLevel.TGAME);
    else
        Canvas.DrawLine(xPos + INSIDE_BOX_OFFSET_X + 1, yPos + INSIDE_BOX_OFFSET_Y2, MIDDLEBOX_WIDTH - 2*(INSIDE_BOX_OFFSET_X + 1), INSIDE_BOX_HEIGHT2, Canvas.black, 255 * 0.20f, eLevel.TGAME);	
    
    // ACTIVATE ~ DEACTIVATE //   
    if( LinkedTurret.GetStateName() == 's_Deactivated' )
        DrawCheckBox(Canvas, xPos + TEXT_POS_X, yPos + TEXT_DEACTIVATE_Y + 1, true);
    else
        DrawCheckBox(Canvas, xPos + TEXT_POS_X, yPos + TEXT_DEACTIVATE_Y + 1, false);

	Canvas.Style = ERenderStyle.STY_Alpha;
	Canvas.SetDrawColor(128,128,128);

	Canvas.TextSize(Canvas.LocalizeStr("DEACTIVATE"), xLen, yLen);

    if(CursorPos == DEACTIVATE)
    {   		
        Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2 - 8, 
			           yPos + TEXT_DEACTIVATE_Y - 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_cote, 8, 15, 8, 0, -8, 15);

		Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2, 
			          yPos + TEXT_DEACTIVATE_Y - 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, xLen, 15, 0, 0, 8, 15);

        Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 + xLen/2, 
			           yPos + TEXT_DEACTIVATE_Y - 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_cote, 8, 15, 0, 0, 8, 15);
        
        
		

        Canvas.DrawColor = Canvas.white;
    }
    else
        Canvas.DrawColor = Canvas.TextGreen;

    Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2 , 
		           yPos + TEXT_DEACTIVATE_Y - 2);
	Canvas.DrawText(Canvas.LocalizeStr("DEACTIVATE"));

    // ENABLE_IFF ~ DISABLE_IFF //
    if( LinkedTurret.SensorDetectionType == SCAN_AllChangedActors )
        DrawCheckBox(Canvas, xPos + TEXT_POS_X, yPos + TEXT_DISABLE_Y + 1, true);
    else
        DrawCheckBox(Canvas, xPos + TEXT_POS_X, yPos + TEXT_DISABLE_Y + 1, false);

	Canvas.Style = ERenderStyle.STY_Alpha;
	Canvas.SetDrawColor(128,128,128);

	Canvas.TextSize(Canvas.LocalizeStr("DISABLEIFF"), xLen, yLen);

    if(CursorPos == DISABLE_IFF)
    {  
        Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2 - 8, 
			           yPos + TEXT_DISABLE_Y - 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_cote, 8, 15, 8, 0, -8, 15);

		Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2, 
			           yPos + TEXT_DISABLE_Y - 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, xLen, 15, 0, 0, 8, 15);

        Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 + xLen/2, 
			           yPos + TEXT_DISABLE_Y - 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_cote, 8, 15, 0, 0, 8, 15);
        
        

        Canvas.DrawColor = Canvas.white;
    }
    else
        Canvas.DrawColor = Canvas.TextGreen;

    Canvas.SetPos( xPos + 10 + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2 , 
		           yPos + TEXT_DISABLE_Y - 2);
    Canvas.DrawText(Canvas.LocalizeStr("DISABLEIFF"));

    // EXIT //

	Canvas.TextSize(Canvas.LocalizeStr("EXIT"), xLen, yLen);

    if(CursorPos == EXIT)
    {        
		// Left side of selector
        Canvas.SetPos( xPos + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2 - 8, 
			           yPos + TEXT_EXIT);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_cote, 8, 15, 8, 0, -8, 15);

		// Middle part of selector
		Canvas.SetPos( xPos + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2, 
			           yPos + TEXT_EXIT);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, xLen , 15, 0, 0, 8, 15);

		// Right part of selector
        Canvas.SetPos( xPos + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 + xLen/2, 
			           yPos + TEXT_EXIT);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_cote, 8, 15, 0, 0, 8, 15);
        
        Canvas.DrawColor = Canvas.white;
    }
    else
        Canvas.DrawColor = Canvas.TextGreen;
    
    

    Canvas.SetPos(xPos + (TURRET_WIDTH - (TURRET_BORDER_WIDTH*2) - (MIDDLEBOX_OFFSET_X*2))/2 - xLen/2, yPos + TEXT_EXIT);
    Canvas.DrawText(Canvas.LocalizeStr("EXIT"));
}

/*-----------------------------------------------------------------------------
 Function:      DrawCheckBox

 Description:   -
-----------------------------------------------------------------------------*/
function DrawCheckBox(ECanvas Canvas, int xPos, int yPos, bool bSelected)
{
    Canvas.SetDrawColor(128,128,128);
	Canvas.Style = ERenderStyle.STY_Alpha;

    Canvas.SetPos(xPos, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 2, 0, -2, 2);

    Canvas.SetPos(xPos + 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 0, 0, 2, 2);

    Canvas.SetPos(xPos, yPos + 8);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 2, 2, -2, -2);

    Canvas.SetPos(xPos + 8, yPos + 8);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 0, 2, 2, -2);

    Canvas.SetPos(xPos + 2, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 6, 1, 0, 0, 1, 1);

    Canvas.SetPos(xPos + 2, yPos + 9);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 6, 1, 0, 0, 1, 1);

    Canvas.SetPos(xPos, yPos + 2);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 1, 6, 0, 0, 1, 1);

    Canvas.SetPos(xPos + 9, yPos + 2);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 1, 6, 0, 0, 1, 1);

    if(bSelected)
    {
        Canvas.SetPos(xPos + 3, yPos + 2);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 4, 1, 0, 0, 1, 1);

        Canvas.SetPos(xPos + 2, yPos + 3);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 6, 4, 0, 0, 1, 1);

        Canvas.SetPos(xPos + 3, yPos + 7);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.tu_select_mil, 4, 1, 0, 0, 1, 1);
    }
	Canvas.Style = ERenderStyle.STY_Normal;
}

/*-----------------------------------------------------------------------------
 Function:      DrawAnimText

 Description:   -
-----------------------------------------------------------------------------*/
function DrawAnimText(ECanvas Canvas)
{
    local int length, i, xPos, yPos;

	Canvas.SetDrawColor(51, 51, 51);

    // UPDATE TEXT TO OUTPUT //
    if( (Level.TimeSeconds - fLastRotationTime) > TEXT_ROTATION_TIME )
    {
        // UPDATE BINARY TEXT //
        /*if(rand(10) == 5)
            szTopBynaryText    = " "$Left(szTopBynaryText, 26);
        else*/
            szTopBynaryText    = Chr(48 + Rand(2))$Left(szTopBynaryText, 26);
        /*if(rand(10) == 5)
            szBottomBynaryText = " "$Left(szBottomBynaryText, 26);
        else*/
            szBottomBynaryText = Chr(48 + Rand(2))$Left(szBottomBynaryText, 26);

        // UPDATE HACKING TURRET SOFTWARE TEXT//
        szHackingText = Canvas.LocalizeStr("HackingTurret");

        length = Len(szHackingText);

        iHackingIndex++;
        if(iHackingIndex >= length / 2)
            iHackingIndex = 0;

        szHackingText = Mid(szHackingText, iHackingIndex, 18);

        fLastRotationTime = Level.TimeSeconds;
    }

    Canvas.Font = font'Verdana9';

	xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - LBAR_WIDTH;
    yPos = eGame.HUD_OFFSET_Y;

    Canvas.SetPos(xPos - TURRET_WIDTH + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_X, yPos+8);
    Canvas.DrawText(szTopBynaryText);

    Canvas.SetPos(xPos - TURRET_WIDTH + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_X, yPos+28);
    Canvas.DrawText(szBottomBynaryText);

    Canvas.font = Canvas.ETextFont;

    Canvas.SetPos(xPos - TURRET_WIDTH + TURRET_BORDER_WIDTH + MIDDLEBOX_OFFSET_X, yPos+16);
    Canvas.DrawText(szHackingText);

    // PATCH - Overwrite the end of text //
    Canvas.DrawLine(xPos - TURRET_BORDER_WIDTH - 5, yPos+16, 5, 15, Canvas.black, -1, eLevel.TGAME);
    // END PATCH //
}

defaultproperties
{
    bDamageable=false
    StaticMesh=StaticMesh'EMeshIngredient.Object.TurretCase'
    CollisionRadius=30.000000
    CollisionHeight=29.000000
    CollisionPrimitive=StaticMesh'EMeshIngredient.Object.TurretCase_COL'
    bStaticMeshCylColl=false
    bCPBlockPlayers=true
    InteractionClass=Class'ETurretInteraction'
}