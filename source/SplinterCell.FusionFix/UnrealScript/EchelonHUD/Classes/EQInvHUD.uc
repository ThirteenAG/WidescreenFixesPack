/******************************************************************************

 Class:         EQInvHUD

 Description:   Quick Inventory HUD


 Reference:     -

******************************************************************************/
class EQInvHUD extends Actor;

var int			CurrentCategory,
				CurrentItem,
				Gap;


var EPlayerController Epc;
var	EInventory	PCInventory;

var color		TextColor,
                TextSelectedColor,
                TextDisabledColor,
				HUDColor;

var	EchelonGameInfo	 eGame;
var EchelonLevelInfo eLevel;

var bool		bPreviousConfig;
var bool        bStopDrawing;
var int         iCurrentPos;
var FLOAT       fTimerValue;
var bool        bStartTimer;
var string      szAlarmText;
var int         iAlarmIndex;
var FLOAT       fTime;
var bool        bBlink;
var FLOAT       fBlinkTime;
var bool        bStopBlinkRecon;
var bool        bStopBlinkGoal;
var bool        bStopBlinkNote;
var FLOAT       fNbrReconBlink;
var FLOAT       fNbrGoalBlink;
var FLOAT       fNbrNoteBlink;

var string      sCurrentGoal;

/*-----------------------------------------------------------------------------
                      T Y P E   D E F I N I T I O N S
-----------------------------------------------------------------------------*/

const SCREEN_END_X   = 640;
const SCREEN_END_Y   = 480;

const ITEMBOX_WIDTH_L  = 96;      // Width of little box
const ITEMBOX_WIDTH_B  = 122;     // Width of big box
const ITEMBOX_HEIGHT_L = 25;      // Height of little box
const ITEMBOX_HEIGHT_B = 49;      // Height of big box
const CURRENT_GOAL_WIDTH = 475;   // Width of current goal
const CG_EXTRA_SPACE = 2;
const CG_BORDER_WIDTH = 2;
const CG_CORNER_WIDTH = 3;
const CG_CORNER_HEIGHT = 3;

const fHIDE_TIME = 1.0f;



const TEXTBOX_HIGHT    = 15;      // Height of text box
const SEC_AMMO_WIDTH   = 26;

const STEALTH_METER_BREAK = 25;
const SEPARATOR           = 8;

const SPACE_BETWEEN_BOX   = 2;
const SPACE_EXTRA_GOAL    = 4;

const TEXTBOX_HEIGHT = 23;
const FIRSTCAT_OFFSET    = 135;
const SPACE_BETWEEN_CAT = 9;

const ITEMBOX_HEIGHT_CAT = 40;
const SEPARATOR_HEIGTHT  = 1;

// Stealth Meter //
const FULLY_VISIBLE_VALUE = 255.0f;
const STEALTH_METER_WIDTH = 76;
const STEALTH_METER_HEIGHT = 9;
const NB_STEALTH_STATUS   = 5;

const INSIDE_BORDER_ALPHA = 77;     // 70%
const BLACK_BORDER_ALPHA = 153;     // 40%

const ALARM_TEXT_ROT_TIME       = 0.1f;     // Set the speed of animated text
const ICON_HEIGHT               = 30;
const ICON_SPACING              = 20;
const FBLINKINGTIME	            = 0.45f;
const MAX_BLINK_TIME	        = 120.0f;

// Fake Window
const CAT0_X		= 409;
const CAT1_X		= 304;
const CAT2_X		= 199;

const ALLCAT_Y		= 387;


function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta );

function Tick(float Delta)
{
	fTime += Delta;

	if ( bStartTimer )
	{
		fTimerValue += Delta;
	}

	if ( bStopDrawing )
	{
		if ( iCurrentPos < 100 )
		{
			iCurrentPos +=  int(Delta * 100);
		}
		else
		{
			if ( !bStartTimer)
			{
				bStartTimer = true;
				fTimerValue = 0.0;
			}
		}
	}
	else
	{
		iCurrentPos -=  int(Delta * 100);
		if (iCurrentPos < 0 ) iCurrentPos = 0;
	}

	//Manage the blinking state
	fBlinkTime += Delta;

	if(fBlinkTime >= FBLINKINGTIME)
	{
		fBlinkTime = fBlinkTime-FBLINKINGTIME;
		if(bBlink)
			bBlink=false;
		else
			bBlink=true;
	}
}

function PostRender(Canvas C)
{
	local ECanvas Canvas;

	Canvas = ECanvas(C);

    DrawHandItem(Canvas, SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L - ITEMBOX_HEIGHT_B - SPACE_BETWEEN_BOX - SPACE_EXTRA_GOAL, false);
    DrawStealthMeter(Canvas);
    DrawRateOfFire(Canvas);



	// Display current goal
	if ( Epc.CurrentGoal != "" && (Epc.CurrentGoalSection != "") && (Epc.CurrentGoalKey != "") && (Epc.CurrentGoalPackage != "") )
	{
		sCurrentGoal = Localize(Epc.CurrentGoalSection, Epc.CurrentGoalKey, Epc.CurrentGoalPackage);

		if ( sCurrentGoal != "(null)" )
		DisplayCurrentGoal(Canvas);
	}

	// Display icon
	if (Epc.bNewGoal || Epc.bNewNote || Epc.bNewRecon )
	{
		DisplayIconsGoalNoteRecon(Canvas);
	}


}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
state s_QDisplay
{
	function BeginState()
	{
		CurrentCategory	= -1;
		CurrentItem		= -1;
		Gap				= 17;

		// Joshua - Use timer delay before showing menu for controller
		if( eGame.bUseController )
		{
			bPreviousConfig = true;
			SetTimer(0.15f, false);
		}
		else
		{
			bPreviousConfig = false;
			Epc.SetPause(true);
			Epc.FakeMouseToggle(true);

			if ( !Epc.EPawn.IsPlaying(Sound'Interface.Play_OpenPackSac') )
				Epc.EPawn.playsound(Sound'Interface.Play_OpenPackSac', SLOT_Interface);
		}
	}

	function Timer()
	{
		// Joshua - Use timer delay before showing menu for controller
		if( !Epc.CanAccessQuick() || Epc.bStopInput )
		{
			GotoState('');
			Owner.GotoState(EchelonMainHud(Owner).RestoreState());
			return;
		}

		Epc.SetPause(true);
		bPreviousConfig = false;

		if ( !Epc.EPawn.IsPlaying(Sound'Interface.Play_OpenPackSac') )
			Epc.EPawn.playsound(Sound'Interface.Play_OpenPackSac', SLOT_Interface);
	}

	function EndState()
	{
		// Quick tap to get back to previous item
		if( !bPreviousConfig )
			Epc.EPawn.playsound(Sound'Interface.Play_ClosePackSac', SLOT_Interface);

		Epc.FakeMouseToggle(false);
		Epc.SetPause(false);

		CurrentItem		= -1;
		CurrentCategory = -1;
		Gap				= 0;
	}

	//
	// Do the Fake Mouse thing
	//
	function Tick( float DeltaTime )
	{
		local int nbItems, HoldCategory, HoldItem;
		local EInventoryItem	Item;

		// Joshua - use KeyEvent for controller navigation, skip mouse logic
		if( eGame.bUseController )
		{
			return;
		}

		HoldCategory = CurrentCategory;
		HoldItem = CurrentItem;

		// Abort any selection
		CurrentCategory = -1;
		CurrentItem = -1;

		//
		// Crappy category selection
		//
		if (Epc.m_FakeMouseX > (CAT2_X - eGame.HUD_OFFSET_X) && Epc.m_FakeMouseX < (CAT2_X - eGame.HUD_OFFSET_X) + ITEMBOX_WIDTH_L)
			CurrentCategory = 2;
		else if (Epc.m_FakeMouseX > (CAT1_X - eGame.HUD_OFFSET_X) && Epc.m_FakeMouseX < (CAT1_X - eGame.HUD_OFFSET_X) + ITEMBOX_WIDTH_L)
			CurrentCategory = 1;
		else if (Epc.m_FakeMouseX > (CAT0_X - eGame.HUD_OFFSET_X) && Epc.m_FakeMouseX < (CAT0_X - eGame.HUD_OFFSET_X) + ITEMBOX_WIDTH_L)
			CurrentCategory = 0;

		// Make sure chosen category is available
		if( !IsCategoryAvailable(GetCategory(CurrentCategory)) )
			CurrentCategory = -1;

		//
		// Could-be-worst item selection
		//
		if( CurrentCategory != -1 && Epc.m_FakeMouseY < ALLCAT_Y )
		{
			CurrentItem = int(abs((Epc.m_FakeMouseY - ALLCAT_Y)/(ITEMBOX_HEIGHT_CAT + SEPARATOR_HEIGTHT)));
			if( CurrentItem >= PCInventory.GetNbItemInCategory(GetCategory(CurrentCategory)) )
				CurrentItem = -1;
		}

		//
		// If there was no item selection, cancel selection
		//
		if( CurrentCategory == -1 || CurrentItem == -1 )
		{
			CurrentCategory = -1;
			CurrentItem = -1;
		}

		// If selection has changed, play sound
		if( HoldCategory != -1 && HoldCategory != CurrentCategory || HoldItem != CurrentItem )
			Epc.Pawn.playsound(Sound'Interface.Play_NavigatePackSac', SLOT_Interface);

		//
		// Manage Mouse click
		//
		if( Epc.m_FakeMouseClicked )
		{
			// On valid exit, check for any selected item
			if( CurrentCategory != -1 && CurrentItem != -1 )
			{
				Item = PCInventory.GetItemInCategory(GetCategory(CurrentCategory), CurrentItem + 1);
				if( !PCInventory.IsSelected(Item) )
    				PCInventory.SetSelectedItem(Item);
	    		else if( !Item.IsA('EMainGun') && !Item.IsA('EOneHandedWeapon') )
		    		PCInventory.UnEquipItem(Item);
			}
/*			else if( bPreviousConfig )
			{
				PCInventory.SetPreviousConfig();
			}*/

			Epc.m_FakeMouseClicked = false;

			GotoState('');
			Owner.GotoState(EchelonMainHud(Owner).RestoreState());
		}

		Epc.m_FakeMouseClicked = false;
	}

	// Joshua - Controller D-Pad/Analog navigation
	function bool KeyEvent(string Key, EInputAction Action, float Delta)
	{
		local EInventoryItem	Item;
		local int nbItem, HoldCategory, HoldItem;

		if( eGame.bUseController )
		{
			if( Action == IST_Press )
			{
				// ignore Timer, reset it now.
				if( bPreviousConfig )
					Timer();

				HoldCategory = CurrentCategory;
				HoldItem = CurrentItem;

				switch( Key )
				{
				case "DPadUp" :
				case "AnalogUp" :
				case "MoveForward" :
					// if over category name and not last item .. move to above item
					if( CurrentCategory != -1 )
						CurrentItem = Min(CurrentItem+1, PCInventory.GetNbItemInCategory(GetCategory(CurrentCategory))-1);
					break;

				case "DPadDown" :
				case "AnalogDown" :
				case "MoveBackward" :
					if( CurrentCategory != -1 )
						CurrentItem = Max(CurrentItem-1, -1);
					break;

				case "DPadRight" :
				case "AnalogRight" :
				case "StrafeRight" :
					if( CurrentCategory > -1 )
						CurrentCategory--;

					while( !IsCategoryAvailable(GetCategory(CurrentCategory)) && CurrentCategory > -1 )
						CurrentCategory--;

					CurrentItem = Min(HoldItem, PCInventory.GetNbItemInCategory(GetCategory(CurrentCategory))-1);

					break;

				case "DPadLeft" :
				case "AnalogLeft":
				case "StrafeLeft" :
					while( true )
					{
						CurrentCategory++;
						if( CurrentCategory <= PCInventory.GetNumberOfCategories()-1 && IsCategoryAvailable(GetCategory(CurrentCategory)) )
							break;
						else if( CurrentCategory == PCInventory.GetNumberOfCategories() )
						{
							CurrentCategory = HoldCategory;
							break;
						}
					}
					CurrentItem = Min(HoldItem, PCInventory.GetNbItemInCategory(GetCategory(CurrentCategory))-1);

					break;

				case "FullInventory" :
					// Exit when selecting Fullinventory or ghost
					GotoState('');
					Owner.GotoState(EchelonMainHud(Owner).RestoreState());

					break;
				}

				// If selection has changed, play sound
				if( HoldCategory != CurrentCategory || HoldItem != CurrentItem )
					Epc.Pawn.playsound(Sound'Interface.Play_NavigatePackSac', SLOT_Interface);
			}
			else if( Action == IST_Release )
			{
				if( Key == "QuickInventory" )
				{
					// On valid exit, check for any selected item
					if(CurrentCategory != -1 && CurrentItem != -1 )
					{
						Item = PCInventory.GetItemInCategory(GetCategory(CurrentCategory), CurrentItem + 1);
						if( !PCInventory.IsSelected(Item) )
							PCInventory.SetSelectedItem(Item);
						else if( !Item.IsA('EMainGun') && !Item.IsA('EOneHandedWeapon') )
							PCInventory.UnEquipItem(Item);
					}
					else if( bPreviousConfig )
					{
						PCInventory.SetPreviousConfig();
					}

					GotoState('');
					Owner.GotoState(EchelonMainHud(Owner).RestoreState());
				}
			}

			return false;
		}

		if( Action == IST_Release )
		{
			if( Key == "QuickInventory" )
            {
				GotoState('');
				Owner.GotoState(EchelonMainHud(Owner).RestoreState());
			}
		}

		return false;
	}

    function PostRender(Canvas C)
	{
        local ECanvas			Canvas;

		// Joshua - Draw normal view until timer expired
		if( eGame.bUseController && bPreviousConfig )
		{
			Global.PostRender(C);
			return;
		}

        Canvas = ECanvas(C);

        DrawHandItem(Canvas, SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L  - TEXTBOX_HEIGHT - ITEMBOX_HEIGHT_B - SPACE_BETWEEN_BOX - SPACE_EXTRA_GOAL, true);
        DrawInfoBar(Canvas);
        DrawNavigationText(Canvas);
        DrawCategory(Canvas);
    }

/*	function Timer()
	{
		Epc.SetPause(true);
		bPreviousConfig = false;

		Epc.FakeMouseToggle(true);

		if ( !Epc.EPawn.IsPlaying(Sound'Interface.Play_OpenPackSac') )
		Epc.EPawn.playsound(Sound'Interface.Play_OpenPackSac', SLOT_Interface);
	}*/
}

/*-----------------------------------------------------------------------------
                            F U N C T I O N S
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 Function:      PostBeginPlay

 Description:   -
-----------------------------------------------------------------------------*/
function PostBeginPlay()
{
	Epc = EPlayerController(Owner.Owner);
	if( Epc == None )
		Log("ERROR: Getting PlayerController in quick inv");

	PCInventory = Epc.ePawn.FullInventory;
	if( PCInventory == None )
		Log("Problem to get player inventory in Quick inv. HUD");

    eGame  = EchelonGameInfo(Level.Game);
    eLevel = EchelonLevelInfo(Level);
	bStopDrawing = false;
	iCurrentPos = 0;
	fTimerValue = 0.0f;
	bStartTimer = false;
	iAlarmIndex = 0;
	fTime = 0.0f;
	fBlinkTime = 0.0f;
	bStopBlinkRecon = false;
	bStopBlinkGoal = false;
	bStopBlinkNote = false;
	fNbrReconBlink = 0;
	fNbrGoalBlink = 0;
	fNbrNoteBlink = 0;
}

// CRAP CRAP CRAP CRAP CRAP
/*-----------------------------------------------------------------------------
 Function:      GetCategory

 Description:   -
-----------------------------------------------------------------------------*/
function eInvCategory GetCategory(int c)
{
	switch(c)
	{
		case 0: return CAT_MAINGUN;
		case 1: return CAT_GADGETS;
		case 2: return CAT_ITEMS;
		case 3: return CAT_INFO;
	}
}
// CRAP CRAP CRAP CRAP CRAP

/*-----------------------------------------------------------------------------
 Function:      IsCategoryAvailable

 Description:   -
------------------------------------------------------------------------------*/
function bool IsCategoryAvailable( eInvCategory cat )
{
    if(PCInventory.GetNbItemInCategory(cat) == 0)
        return false;

    if( cat != CAT_MAINGUN && (Epc.GetStateName() == 's_FirstPersonTargeting' ||
							   Epc.GetStateName() == 's_RappellingTargeting' ||
							   Epc.GetStateName() == 's_SplitTargeting') )
        return false;

    return true;
}

/*-----------------------------------------------------------------------------
 Function:      WriteText

 Description:   -
-----------------------------------------------------------------------------*/
function WriteText(ECanvas Canvas, float X, float Y, coerce string t, Color c )
{
	Canvas.DrawColor = c;
	Canvas.SetPos(X, Y);
	Canvas.DrawText(t, false);
	Canvas.DrawColor = HUDColor;
}

/*-----------------------------------------------------------------------------
 Function:      DrawStealthMeter

 Description:   -
-----------------------------------------------------------------------------*/
function DrawStealthMeter(ECanvas Canvas)
{
    local int xPos, yPos, xBkgPos, yBkgPos, stealStatusWidth, stealStatusHeight, i;
    local float  visibility, vPos;

    xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - ITEMBOX_WIDTH_L;
    yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L * 2 - ITEMBOX_HEIGHT_B - SPACE_BETWEEN_BOX * 2 - SPACE_EXTRA_GOAL;

	DrawBoxBorders(Canvas, xPos, yPos, ITEMBOX_WIDTH_L, ITEMBOX_HEIGHT_L,,true);

    xBkgPos = xPos + 5;
    yBkgPos = yPos + 6;

    // METER BORDERS //

    xPos += (ITEMBOX_WIDTH_L - STEALTH_METER_WIDTH)/2;
    yPos += (ITEMBOX_HEIGHT_L - STEALTH_METER_HEIGHT)/2 + 1;

    Canvas.DrawLine(xPos + 1, yPos, STEALTH_METER_WIDTH - 2, 1, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    Canvas.DrawLine(xPos + 1, yPos + STEALTH_METER_HEIGHT - 1, STEALTH_METER_WIDTH - 2, 1, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    Canvas.DrawLine(xPos, yPos + 1, 1, STEALTH_METER_HEIGHT - 2, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    Canvas.DrawLine(xPos + STEALTH_METER_WIDTH - 1, yPos + 1, 1, STEALTH_METER_HEIGHT - 2, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);

    Canvas.DrawLine(xPos + 1, yPos + 1, 1, 1, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    Canvas.DrawLine(xPos + 1, yPos + STEALTH_METER_HEIGHT - 2, 1, 1, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    Canvas.DrawLine(xPos + STEALTH_METER_WIDTH - 2, yPos + 1, 1, 1, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    Canvas.DrawLine(xPos + STEALTH_METER_WIDTH - 2, yPos + STEALTH_METER_HEIGHT - 2, 1, 1, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);

    stealStatusWidth  = (STEALTH_METER_WIDTH - 2 - (NB_STEALTH_STATUS-1)) / NB_STEALTH_STATUS;
    stealStatusHeight = (STEALTH_METER_HEIGHT - 3)/2;

    for(i = 0; i < NB_STEALTH_STATUS - 1; i++)
    {
        Canvas.DrawLine(xPos + (i+1) * (stealStatusWidth+1), yPos + 1, 1, stealStatusHeight, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
        Canvas.DrawLine(xPos + (i+1) * (stealStatusWidth+1), yPos + STEALTH_METER_HEIGHT - stealStatusHeight - 1, 1, stealStatusHeight, Canvas.black, BLACK_BORDER_ALPHA, eLevel.TGAME);
    }

    Canvas.SetDrawColor(128,128,128,BLACK_BORDER_ALPHA);
    Canvas.SetPos(xPos + 1, yPos);
    eLevel.TMENU.DrawTileFromManager(Canvas, eLevel.TMENU.slidder_degrade, STEALTH_METER_WIDTH - 2, STEALTH_METER_HEIGHT, 0, 0, eLevel.TMENU.GetWidth(eLevel.TMENU.slidder_degrade), 1);

    visibility = Epc.ePawn.Visibilityfactor;

    // METER //
    if(visibility >= FULLY_VISIBLE_VALUE)
        visibility = FULLY_VISIBLE_VALUE;

    if(visibility < eGame.VisBarelyThreshold)
    {
        vPos = (visibility / eGame.VisBarelyThreshold) * stealStatusWidth;
        DrawStealthMeterSelectBox(Canvas, xPos + 1 + vPos, yPos - 2);
    }
    else if(visibility < eGame.VisPartiallyThreshold)
    {
        vPos = ((visibility - eGame.VisBarelyThreshold) / (eGame.VisPartiallyThreshold - eGame.VisBarelyThreshold)) * (stealStatusWidth + 1);
        DrawStealthMeterSelectBox(Canvas, xPos + stealStatusWidth + 1 + vPos, yPos - 2);
    }
    else if(visibility < eGame.VisMostlyThreshold)
    {
        vPos = ((visibility - eGame.VisPartiallyThreshold) / (eGame.VisMostlyThreshold - eGame.VisPartiallyThreshold)) * (stealStatusWidth + 1);
        DrawStealthMeterSelectBox(Canvas, xPos + (stealStatusWidth + 1) * 2 + vPos, yPos - 2);
    }
    else if(visibility < eGame.VisFullyThreshold)
    {
        vPos = ((visibility - eGame.VisMostlyThreshold) / (eGame.VisFullyThreshold - eGame.VisMostlyThreshold)) * (stealStatusWidth + 1);
        DrawStealthMeterSelectBox(Canvas, xPos + (stealStatusWidth + 1) * 3 + vPos, yPos - 2);
    }
    else //if(visibility < FULLY_VISIBLE_VALUE)
    {
        vPos = ((visibility - eGame.VisFullyThreshold) / (FULLY_VISIBLE_VALUE - eGame.VisFullyThreshold)) * (stealStatusWidth + 1 - 6);
        DrawStealthMeterSelectBox(Canvas, xPos + (stealStatusWidth + 1) * 4 + vPos, yPos - 2);
    }

    // BACKGROUND //
    Canvas.SetDrawColor(64,64,64,255);
    Canvas.Style = ERenderStyle.STY_Modulated;

    Canvas.SetPos(xBkgPos, yBkgPos);
    Canvas.DrawTile(Texture'HUD.HUD.ETMenuBar', ITEMBOX_WIDTH_L - 10, ITEMBOX_HEIGHT_L - 10, 0, 0, 128, 2);

    Canvas.Style = ERenderStyle.STY_Normal;
}

/*-----------------------------------------------------------------------------
 Function:      DrawStealthMeterSelectBox

 Description:   -
-----------------------------------------------------------------------------*/
function DrawStealthMeterSelectBox(ECanvas Canvas, int xPos, int yPos)
{
    Canvas.DrawLine(xPos + 1, yPos + 1, 4, ITEMBOX_HEIGHT_L - 14, Canvas.white, 192, eLevel.TGAME);

    Canvas.DrawColor = HUDColor;
    Canvas.DrawColor.A = BLACK_BORDER_ALPHA;

    // TOP LEFT CORNER //
    Canvas.SetPos(xPos, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 2, 0, -2, 2);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(xPos, yPos + ITEMBOX_HEIGHT_L - 14);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 2, 2, -2, -2);

    // TOP RIGHT CORNER //
    Canvas.SetPos(xPos + 4, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 0, 0, 2, 2);

    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(xPos + 4, yPos + ITEMBOX_HEIGHT_L - 14);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.li_coin, 2, 2, 0, 2, 2, -2);

    // TOP BORDER //
    Canvas.SetPos(xPos + 2, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 2, 1, 0, 0, 1, 1);

    // BOTTOM BORDER //
    Canvas.SetPos(xPos + 2, yPos + ITEMBOX_HEIGHT_L - 13);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 2, 1, 0, 0, 1, 1);

    // LEFT BORDER //
    Canvas.SetPos(xPos, yPos + 2);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 1, ITEMBOX_HEIGHT_L - 16, 0, 0, 1, 1);

    // RIGHT BORDER //
    Canvas.SetPos(xPos + 5, yPos + 2);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 1, ITEMBOX_HEIGHT_L - 16, 0, 0, 1, 1);
}

/*-----------------------------------------------------------------------------
 Function:      DrawRateOfFire

 Description:   -
-----------------------------------------------------------------------------*/
function DrawRateOfFire(ECanvas Canvas)
{
    local int xPos, yPos, sel, j;
    local eWeapon selWeapon;

    xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - ITEMBOX_WIDTH_L;
    yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L - SPACE_EXTRA_GOAL;

	DrawBoxBorders(Canvas, xPos, yPos, ITEMBOX_WIDTH_L, ITEMBOX_HEIGHT_L);

    // RATE OF FIRE //
    selWeapon = eWeapon(PCInventory.GetSelectedItem());

    Canvas.DrawColor = HUDColor;

    if(selWeapon != None)
    {
        // Single //
        if(!selWeapon.IsROFModeAvailable(ROF_Single))
            Canvas.DrawColor.A = 25;
        else if(selWeapon.eROFMode == ROF_Single)
            Canvas.DrawColor.A = BLACK_BORDER_ALPHA;
        else
            Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;

        Canvas.SetPos(xPos + 16, yPos + 8);
		Canvas.Style = ERenderStyle.STY_Alpha;
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_fire, 5, 8, 0, 0, 5, 8);
		Canvas.Style = ERenderStyle.STY_Normal;

        // Auto //
        if(!selWeapon.IsROFModeAvailable(ROF_Auto))
            Canvas.DrawColor.A = 25;
        else if(selWeapon.eROFMode == ROF_Auto)
            Canvas.DrawColor.A = BLACK_BORDER_ALPHA;
        else
            Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;

       for(j = 0; j < 5; j++)
       {
            Canvas.SetPos(xPos + 54 + j*5, yPos + 8);
			Canvas.Style = ERenderStyle.STY_Alpha;
            eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_fire, 5, 8, 0, 0, 5, 8);
			Canvas.Style = ERenderStyle.STY_Normal;
       }
    }
}

function GetWeaponInfo( EWeapon Weapon, out int nbAmmo, out int nbClip )
{
	local float f;

	nbAmmo = int(float(Weapon.ClipAmmo)/float(Weapon.ClipMaxAmmo)*10.0f + 0.99f);

    nbClip = (Weapon.Ammo - Weapon.ClipAmmo)/Weapon.ClipMaxAmmo;
	f = float(Weapon.Ammo - Weapon.ClipAmmo)/Weapon.ClipMaxAmmo;
	if( f - nbClip > 0 )
		nbClip++;
}

/*-----------------------------------------------------------------------------
 Function:      DrawHandItem

 Description:   -
-----------------------------------------------------------------------------*/
function DrawHandItem(ECanvas Canvas, int yPos, bool bDisplayState)
{
    local int xPos, width, i, offset, nbAmmo, nbClip, maxClip;
    local float xLen, yLen;
    local EInventoryItem Item;
    local EMainGun MainGun;
    local EWeapon Weapon;
    local ESecondaryAmmo SecAmmo;
    local string szText;

    Canvas.DrawColor = HUDColor;
    Canvas.Font = Canvas.ETextFont;

    Item = PCInventory.GetSelectedItem();

    // SET BOX SIZE //
	MainGun = EMainGun(Item);
	if(MainGun != None && MainGun.SecondaryAmmo != None)
    {
        xPos  = SCREEN_END_X - eGame.HUD_OFFSET_X - ITEMBOX_WIDTH_B;
        width = ITEMBOX_WIDTH_B;
        offset = SEC_AMMO_WIDTH;
    }
    else
    {
        xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - ITEMBOX_WIDTH_L;
        width = ITEMBOX_WIDTH_L;
        offset = 0;
    }

    // BOX //
	DrawBoxBorders(Canvas, xPos, yPos, width, ITEMBOX_HEIGHT_B);

    Item = PCInventory.GetSelectedItem();
    if(Item != None)
    {
        Canvas.SetDrawColor(64,64,64);
        Canvas.Style = ERenderStyle.STY_Modulated;

        // draw icon //
        Canvas.SetPos(xPos + width - ITEMBOX_WIDTH_L + 4, yPos + 4);
        eLevel.TICON.DrawTileFromManager(Canvas, Item.HUDTex, eLevel.TICON.GetWidth(Item.HUDTex), eLevel.TICON.GetHeight(Item.HUDTex), 0, 0, eLevel.TICON.GetWidth(Item.HUDTex), eLevel.TICON.GetHeight(Item.HUDTex));

        Canvas.Style = ERenderStyle.STY_Normal;
    }

    // WEAPON DISPLAY //
    Weapon = EWeapon(Item);
	if( Weapon != None )
	{
        Canvas.DrawColor = Canvas.TextBlack;
        Canvas.SetPos(xPos + offset + 36, yPos + 29);
        Canvas.DrawTextRightAligned(string(Weapon.Ammo));

        Canvas.DrawColor = HUDColor;

		GetWeaponInfo(Weapon, nbAmmo, nbClip);

        for(i = 0; i < 10; i++)
		{
            Canvas.DrawColor.A = BLACK_BORDER_ALPHA;

			Canvas.SetPos(xPos + offset + 38 + i*5, yPos + 35);
			Canvas.Style = ERenderStyle.STY_Alpha;
            eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 3, 1, 0, 0, 1, 1);
			Canvas.Style = ERenderStyle.STY_Normal;

            if(i < nbAmmo)
                Canvas.DrawColor.A = BLACK_BORDER_ALPHA;
            else
                Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;

            Canvas.SetPos(xPos + offset + 38 + i*5, yPos + 37);
			Canvas.Style = ERenderStyle.STY_Alpha;
            eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bullet, 3, 5, 0, 0, 3, 5);
			Canvas.Style = ERenderStyle.STY_Normal;

            if(i < nbClip)
            {
                Canvas.DrawColor.A = BLACK_BORDER_ALPHA;

                Canvas.SetPos(xPos + offset + 38 + i*5, yPos + 30);
				Canvas.Style = ERenderStyle.STY_Alpha;
                eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_clip, 3, 4, 0, 0, 3, 4);
				Canvas.Style = ERenderStyle.STY_Normal;
            }
		}

        Canvas.DrawColor = HUDColor;

        // SECONDARY AMMO //
        if(MainGun != None && MainGun.SecondaryAmmo != None)
        {
            SecAmmo = ESecondaryAmmo(MainGun.SecondaryAmmo);

            Canvas.SetPos(xPos + SEC_AMMO_WIDTH + 1, yPos + 6);
            eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v2, 4, ITEMBOX_HEIGHT_B - 10, 0, 0, 4, 1);

            if(SecAmmo.MaxQuantity > 1)
            {
                szText = string(SecAmmo.Quantity);
                Canvas.TextSize(szText, xLen, yLen);
                WriteText(Canvas, xPos + 3 + SEC_AMMO_WIDTH/2 - xLen/2, yPos + 30, szText, Canvas.TextBlack);
            }

            // DRAW ICON //
			Canvas.SetDrawColor(64,64,64);
            Canvas.Style = ERenderStyle.STY_Modulated;

            Canvas.SetPos(xPos + 5, yPos + 5);
            eLevel.TICON.DrawTileFromManager(Canvas, SecAmmo.HUDTexSD, eLevel.TICON.GetWidth(SecAmmo.HUDTexSD), eLevel.TICON.GetHeight(SecAmmo.HUDTexSD), 0, 0, eLevel.TICON.GetWidth(SecAmmo.HUDTexSD), eLevel.TICON.GetHeight(SecAmmo.HUDTexSD));

            Canvas.Style = ERenderStyle.STY_Normal;
        }
    }
    else if( Item != None && Item.MaxQuantity > 1 )
    {
		WriteText(Canvas, xPos+70, yPos+9, "x", Canvas.TextBlack);

        if(Item.Quantity < 10)
        {
            if(Item.Quantity == 1)
                WriteText(Canvas, xPos+69, yPos+24, Item.Quantity, Canvas.TextBlack);
            else
                WriteText(Canvas, xPos+70, yPos+24, Item.Quantity, Canvas.TextBlack);
        }
        else
            WriteText(Canvas, xPos+65, yPos+24, Item.Quantity, Canvas.TextBlack);
    }

	// Joshua - Controller selection filter
	// NOT SELECTED FILTER //
    if(bDisplayState && eGame.bUseController)
    {
        if(!(CurrentCategory == -1))
        {
			Canvas.Style = ERenderStyle.STY_Alpha;
            Canvas.DrawLine(xPos + offset + 5, yPos + 6, ITEMBOX_WIDTH_L - 10, ITEMBOX_HEIGHT_B - 10, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);

            if(MainGun != None && MainGun.SecondaryAmmo != None)
                Canvas.DrawLine(xPos + 5, yPos + 6, SEC_AMMO_WIDTH - 4, ITEMBOX_HEIGHT_B - 10, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);
			Canvas.Style = ERenderStyle.STY_Normal;
         }
    }

    // Joshua - Highlight backpack when selected on controller
    Canvas.DrawColor = HUDColor;
    Canvas.DrawColor.A = 128;
    if(bDisplayState && eGame.bUseController && CurrentCategory == -1)
    {
		Canvas.Style = ERenderStyle.STY_Alpha;
        Canvas.SetPos(xPos + width - ITEMBOX_WIDTH_L + 5, yPos + 6);
        eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_light, 35, 37, 0, 2, 35, 37);
		Canvas.Style = ERenderStyle.STY_Normal;
    }
}

/*-----------------------------------------------------------------------------
 Function:      DrawInfoBar

 Description:   -
-----------------------------------------------------------------------------*/
function DrawInfoBar(Ecanvas Canvas)
{
	local EInventoryItem Item;
    local int xPos, yPos, nbCat, width;

	nbCat = PCInventory.GetNumberOfCategories();
	width = FIRSTCAT_OFFSET + nbCat * ITEMBOX_WIDTH_L + (nbCat-1) * SPACE_BETWEEN_CAT;
    xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - FIRSTCAT_OFFSET - nbCat * ITEMBOX_WIDTH_L - (nbCat-1) * SPACE_BETWEEN_CAT;
    yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L - SPACE_EXTRA_GOAL;

	DrawBoxBorders(Canvas, xPos, yPos, width, ITEMBOX_HEIGHT_L);

    // No current cursor over item
	if( CurrentCategory == -1 || CurrentItem == -1 )
	{
		Canvas.Font = Canvas.ETextFont;
		Canvas.DrawColor = Canvas.TextBlack;
		Canvas.SetPos(xPos + width - 15, yPos + 5);

		if ( (Epc.CurrentGoalSection != "") && (Epc.CurrentGoalKey != "") && (Epc.CurrentGoalPackage != "") )
		{
			if ( Localize(Epc.CurrentGoalSection, Epc.CurrentGoalKey, Epc.CurrentGoalPackage) != "(null)" )
			{
				Canvas.DrawTextRightAligned(Localize(Epc.CurrentGoalSection, Epc.CurrentGoalKey, Epc.CurrentGoalPackage));
			}
		}
		return;
	}


    Canvas.Font = Canvas.ETextFont;
    Canvas.DrawColor = Canvas.TextBlack;

	Item = PCInventory.GetItemInCategory(GetCategory(CurrentCategory), CurrentItem+1);
    if(Item != None)
    {
        Canvas.SetPos(xPos + width - 15, yPos + 5);
        WriteText(Canvas, xPos + 15, yPos + 5, Localize("InventoryItem", Item.ItemName, "Localization\\HUD"), Canvas.TextBlack);
    }
}

/*-----------------------------------------------------------------------------
 Function:      DrawNavigationText

 Description:   -
-----------------------------------------------------------------------------*/
function DrawNavigationText(Ecanvas Canvas)
{
    local int xPos, yPos, width, j, hold, nbCat;
    local float xLen, yLen;
    local bool bBackPack;
    local string szText;
    local color drawColor;

    //Canvas.Font = font'EHUDFont';
	Canvas.Font = font'ETextFont';
    Canvas.DrawColor = HUDColor;

    nbCat = PCInventory.GetNumberOfCategories();

    width = FIRSTCAT_OFFSET + nbCat * ITEMBOX_WIDTH_L + (nbCat-1) * SPACE_BETWEEN_CAT;
    xPos  = SCREEN_END_X - eGame.HUD_OFFSET_X - FIRSTCAT_OFFSET - nbCat * ITEMBOX_WIDTH_L - (nbCat-1) * SPACE_BETWEEN_CAT;
    yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L - (TEXTBOX_HEIGHT + SPACE_BETWEEN_BOX) - SPACE_EXTRA_GOAL;

    DrawBoxBorders(Canvas, xPos, yPos, width, TEXTBOX_HEIGHT, true);

	// FILL IN BLACK //
    Canvas.DrawColor.A = 255 * 0.60;
    Canvas.SetPos(xPos + 4, yPos + 3);

    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, width - 9, TEXTBOX_HEIGHT - 7, 0, 0, 1, 1);

    szText = PCInventory.GetPackageName();
    Canvas.TextSize(szText, xLen, yLen);
    WriteText(Canvas, xPos + width - ITEMBOX_WIDTH_L/2 - xLen/2, yPos + 4, szText, TextColor);

    hold = CurrentCategory;
    for(j = 0; j < nbCat; j++)
    {
        CurrentCategory = j;
        szText = PCInventory.GetCategoryName(GetCategory(CurrentCategory));

        if(j == hold)
        {
            drawColor = TextSelectedColor;
            szText = "-"$szText$"-";
        }
        else if(IsCategoryAvailable(GetCategory(j)))
            drawColor = TextColor;
        else
            drawColor = TextDisabledColor;

        Canvas.TextSize(szText, xLen, yLen);
        WriteText(Canvas, xPos + width - FIRSTCAT_OFFSET - j*(ITEMBOX_WIDTH_L+SPACE_BETWEEN_CAT) - ITEMBOX_WIDTH_L/2 - xLen/2, yPos + 4, szText, drawColor);
    }
    CurrentCategory = hold;
}

//------------------------------------------------------------------------
// Description
//		Draw a standard 3d-style quick inventory box
//------------------------------------------------------------------------
function DrawBoxBorders(ECanvas Canvas, int xPos, int yPos, int width, int height, optional bool bNoDrawBackGround, optional bool bOpacityOnly)
{
    Canvas.DrawColor = HUDColor;

	Canvas.Style = ERenderStyle.STY_Alpha;
    // TOP LEFT CORNER //
    Canvas.SetPos(xPos, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 0, 7, 8, -7);

    // TOP RIGHT CORNER //
    Canvas.SetPos(xPos + width - 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 8, 7, -8, -7);

    // BOTTOM LEFT CORNER //
    Canvas.SetPos(xPos, yPos + height - 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin2, 8, 4, 8, 0, -8, 4);

    // BOTTOM RIGHT CORNER //
    Canvas.SetPos(xPos + width - 8, yPos + height - 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_coin1, 8, 7, 8, 0, -8, 7);

    // TOP BORDER //
    Canvas.SetPos(xPos + 8, yPos);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h, width - 16, 6, 0, 6, 1, -6);

    // BOTTOM BORDER //
    Canvas.SetPos(xPos + 8, yPos + height - 4);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_h2, width - 13, 4, 0, 0, 1, 4);

    // LEFT BORDER //
    Canvas.SetPos(xPos, yPos + 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v, 5, height - 11, 0, 0, 5, 1);

    // RIGHT BORDER //
    Canvas.SetPos(xPos + width - 5, yPos + 7);
    eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bord_v, 5, height - 11, 5, 0, -5, 1);

	if( bNoDrawBackGround )
		return;

    // OPACITY BACKGROUND //
    Canvas.DrawLine(xPos + 5, yPos + 6, width - 10, height - 10, Canvas.white, -1, eLevel.TGAME);
    Canvas.DrawRectangle(xPos + 5, yPos + 6, width - 10, height - 10, 1, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);

	if( bOpacityOnly )
		return;

    // TRANSPARENCY BACKGROUND //
    Canvas.SetDrawColor(64,64,64,255);
    Canvas.Style = ERenderStyle.STY_Modulated;

    Canvas.SetPos(xPos + 5, yPos + 6);
    Canvas.DrawTile(Texture'HUD.HUD.ETMenuBar', width - 10, height - 10, 0, 0, 128, 2);

    Canvas.Style = ERenderStyle.STY_Normal;
}

/*-----------------------------------------------------------------------------
 Function:      DrawCategory

 Description:   -
-----------------------------------------------------------------------------*/
function DrawCategory(ECanvas Canvas)
{
    local int xPos, yPos, xLightPos, yLightPos, i, nbItems, height, CatIndex;

	for( CatIndex=0; CatIndex<PCInventory.GetNumberOfCategories()/*-1*/; CatIndex++ )
	{
		// Filter not-available categories
		if( !IsCategoryAvailable(GetCategory(CatIndex)) )
			continue;

		// Filter empty categories
		nbItems = PCInventory.GetNbItemInCategory(GetCategory(CatIndex));
		if( nbItems == 0 )
			continue;

		xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - FIRSTCAT_OFFSET - (CatIndex + 1) * ITEMBOX_WIDTH_L - CatIndex * SPACE_BETWEEN_CAT;
		yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L - (TEXTBOX_HEIGHT + SPACE_BETWEEN_BOX) - SPACE_EXTRA_GOAL;

		height = nbItems * ITEMBOX_HEIGHT_CAT + (nbItems - 1) * SEPARATOR_HEIGTHT + 12;

		Canvas.Style = ERenderStyle.STY_Alpha;
		DrawBoxBorders(Canvas, xPos, yPos-height, ITEMBOX_WIDTH_L, height);
		Canvas.Style = ERenderStyle.STY_Normal;

		// SEPARATOR //
		Canvas.DrawColor = HUDColor;

		// ITEMS //
		for(i = 1; i <= nbItems; i++)
		{
			Canvas.Style = ERenderStyle.STY_Alpha;
			Canvas.DrawLine(xPos + 7, yPos - ITEMBOX_HEIGHT_CAT * i - SEPARATOR_HEIGTHT * i - 5, ITEMBOX_WIDTH_L - 14, 1, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);
			Canvas.DrawLine(xPos + 6, yPos - ITEMBOX_HEIGHT_CAT * i - SEPARATOR_HEIGTHT * i - 6, 1, 3, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);
			Canvas.DrawLine(xPos + ITEMBOX_WIDTH_L - 7, yPos - ITEMBOX_HEIGHT_CAT * i - SEPARATOR_HEIGTHT * i - 6, 1, 3, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);

			if( CurrentCategory == CatIndex && i-1 == CurrentItem)
			{
				xLightPos = xPos + 5;
				yLightPos = yPos - ITEMBOX_HEIGHT_CAT * i - SEPARATOR_HEIGTHT * i - 4;
				DrawItem(Canvas, xPos + 5,  yPos - ITEMBOX_HEIGHT_CAT * i - SEPARATOR_HEIGTHT * i - 4, PCInventory.GetItemInCategory(GetCategory(CatIndex), i), true);
			}
			else
			{
				DrawItem(Canvas, xPos + 5,  yPos - ITEMBOX_HEIGHT_CAT * i - SEPARATOR_HEIGTHT * i - 4, PCInventory.GetItemInCategory(GetCategory(CatIndex), i), false);
			}
			Canvas.Style = ERenderStyle.STY_Normal;
		}

		// LIGHT //
		if( CurrentCategory == CatIndex && CurrentItem > -1 )
		{
			Canvas.SetDrawColor(128,128,128,128);
			Canvas.SetPos(xLightPos, yLightPos);
			Canvas.Style = ERenderStyle.STY_Alpha;
			eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_light, 35, ITEMBOX_HEIGHT_CAT, 0, 1, 35, ITEMBOX_HEIGHT_CAT);
			Canvas.Style = ERenderStyle.STY_Normal;
		}
	}
}

/*-----------------------------------------------------------------------------
 Function:      DrawItem

 Description:   -
-----------------------------------------------------------------------------*/
function DrawItem(ECanvas Canvas, int xPos, int yPos, EInventoryItem Item, bool bSelected)
{
    local int i, nbAmmo, nbClip;
    local EMainGun MainGun;
    local EWeapon Weapon;
    local bool bEquipedItem;
    local color txtColor;

    Canvas.DrawColor = HUDColor;
    Canvas.Font = Canvas.ETextFont;

    MainGun = EMainGun(PCInventory.GetSelectedItem());
    if( Item == PCInventory.GetSelectedItem() || (MainGun != None && MainGun.SecondaryAmmo != None && Item == MainGun.SecondaryAmmo) )
       bEquipedItem = true;
    else
       bEquipedItem = false;

    Canvas.SetDrawColor(64,64,64);

    Canvas.Style = ERenderStyle.STY_Modulated;

    Canvas.SetPos(xPos, yPos);
    eLevel.TICON.DrawTileFromManager(Canvas, Item.HUDTex, eLevel.TICON.GetWidth(Item.HUDTex), eLevel.TICON.GetHeight(Item.HUDTex), 0, 0, eLevel.TICON.GetWidth(Item.HUDTex), eLevel.TICON.GetHeight(Item.HUDTex));

    Canvas.Style = ERenderStyle.STY_Normal;

	//return;

      // WEAPON DISPLAY //
    Weapon = EWeapon(Item);
	if( Weapon != None )
	{
        Canvas.DrawColor = Canvas.TextBlack;

        Canvas.SetPos(xPos + 31, yPos + 25);
        Canvas.DrawTextRightAligned(String(Weapon.Ammo));

        Canvas.DrawColor = HUDColor;

		GetWeaponInfo(Weapon, nbAmmo, nbClip);

        for(i = 0; i < 10; i++)
		{
            Canvas.DrawColor = HUDColor;
            if(bEquipedItem)
                Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;
            else
                Canvas.DrawColor.A = BLACK_BORDER_ALPHA;

			Canvas.SetPos(xPos + 33 + i*5, yPos + 31);
			Canvas.Style = ERenderStyle.STY_Alpha;
            eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.sl_stroke, 3, 1, 0, 0, 1, 1);
			Canvas.Style = ERenderStyle.STY_Normal;

            if(i < nbAmmo)
            {
                if(bEquipedItem)
                    Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;
                else
                    Canvas.DrawColor.A = BLACK_BORDER_ALPHA;
            }
            else
            {
                if(bEquipedItem)
                    Canvas.DrawColor.A = INSIDE_BORDER_ALPHA / 2;
                else
                    Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;
            }

            Canvas.SetPos(xPos + 33 + i*5, yPos + 33);
			Canvas.Style = ERenderStyle.STY_Alpha;
            eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_bullet, 3, 5, 0, 0, 3, 5);
			Canvas.Style = ERenderStyle.STY_Normal;

            if(i < nbClip)
            {
                if(bEquipedItem)
                    Canvas.DrawColor.A = INSIDE_BORDER_ALPHA;
                else
                    Canvas.DrawColor.A = BLACK_BORDER_ALPHA;

                Canvas.SetPos(xPos + 33 + i*5, yPos + 26);
				Canvas.Style = ERenderStyle.STY_Alpha;
                eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.qi_clip, 3, 4, 0, 0, 3, 4);
				Canvas.Style = ERenderStyle.STY_Normal;
            }
		}
    }
	else if( Item != None && Item.MaxQuantity > 1 )
    {
        txtColor = Canvas.TextBlack;

		WriteText(Canvas, xPos+65, yPos+5, "x", txtColor);

        if(Item.Quantity < 10)
        {
            if(Item.Quantity == 1)
                WriteText(Canvas, xPos+64, yPos+20, Item.Quantity, txtColor);
            else
                WriteText(Canvas, xPos+65, yPos+20, Item.Quantity, txtColor);
        }
        else
            WriteText(Canvas, xPos+60, yPos+20, Item.Quantity, txtColor);
    }

    if(!bSelected)
	{
		Canvas.Style = ERenderStyle.STY_Alpha;
        Canvas.DrawLine(xPos, yPos - 1, ITEMBOX_WIDTH_L - 10, ITEMBOX_HEIGHT_CAT + 2, Canvas.black, INSIDE_BORDER_ALPHA, eLevel.TGAME);
		Canvas.Style = ERenderStyle.STY_Normal;
	}
}

//-----------------------------------------------------------------------------
// Function:      DisplayCurrentGoal
//
// Description:   -
//-----------------------------------------------------------------------------
function DisplayCurrentGoal(ECanvas Canvas)
{
	local int xPos, yPos, blackHeight;
	local float xLen, yLen;

	// Get font size
	Canvas.Font = Canvas.ETextFont;
    Canvas.TextSize("T", xLen, yLen);

	blackHeight	= yLen + 10;

	xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - ITEMBOX_WIDTH_L - CURRENT_GOAL_WIDTH - 5;
    yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y + 1 - ITEMBOX_HEIGHT_L - SPACE_EXTRA_GOAL;


	if ( VSize(Epc.ePawn.Velocity) != 0.0 )
	{
		fTimerValue = 0.0;
		bStopDrawing = true;
	}
	else
	{
		if ( bStartTimer && (fTimerValue > fHIDE_TIME) )
		{
			bStopDrawing = false;
			bStartTimer = false;
		}
	}

	yPos += iCurrentPos;


    Canvas.SetDrawColor(128,128,128);
	Canvas.Style=ERenderStyle.STY_Alpha;

	// Draw the second box with the interaction messages
	// TOP LEFT CORNER
	Canvas.SetPos(xPos, yPos);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 0, 3, 3, -3);

	// BOTTOM LEFT CORNER
	Canvas.SetPos(xPos, yPos + blackHeight - 3);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 0, 0, 3, 3);

	// TOP RIGHT CORNER
	Canvas.SetPos(xPos + CURRENT_GOAL_WIDTH - 3, yPos);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 3, 3, -3, -3);

	// BOTTOM RIGHT CORNER
	Canvas.SetPos(xPos + CURRENT_GOAL_WIDTH - 3, yPos + blackHeight - 3);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_coin, 3, 3, 3, 0, -3, 3);

	// LEFT BORDER
	Canvas.SetPos(xPos, yPos + 3);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_v,2, blackHeight - 3, 0, 0, 2, 1);

	// RIGHT
	Canvas.SetPos(xPos + CURRENT_GOAL_WIDTH - 2, yPos + 3);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_v,2, blackHeight - 3, 2, 1, -2, -1);

	// TOP
	Canvas.SetPos(xPos + 3, yPos);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_h, CURRENT_GOAL_WIDTH - 6, 2, 0, 0, 1, 2);

	// BOTTOM
	Canvas.SetPos(xPos + 3, yPos + blackHeight - 2);
	eLevel.TGAME.DrawTileFromManager(Canvas, eLevel.TGAME.int_bord_h, CURRENT_GOAL_WIDTH - 6, 2, 0, 0, 1, 2);
	Canvas.Style=ERenderStyle.STY_Normal;

	// Fill the background
	Canvas.DrawLine(xPos + 2, yPos + 2, CURRENT_GOAL_WIDTH - 4, blackHeight - 4, Canvas.black, 200, eLevel.TMENU);

	// Write the mission current goal
	Canvas.SetClip(CURRENT_GOAL_WIDTH, yLen);
	Canvas.SetPos(xPos + CURRENT_GOAL_WIDTH - 5, yPos + 4);
	Canvas.SetDrawColor(68,77,55);
	//DrawAnimText(Canvas, xPos, yPos, xLen, yLen);

	Canvas.DrawTextAligned(sCurrentGoal,TXT_RIGHT);


	Canvas.Style = ERenderStyle.STY_Normal;
	Canvas.SetClip(640,480);
}

/*-----------------------------------------------------------------------------
 Function:      DrawAnimText

 Description:   -
-----------------------------------------------------------------------------*/
function DrawAnimText(ECanvas Canvas, int xPos, int yPos, FLOAT xLen, FLOAT yLen)
{
    local int length, i, iEmptySpot,y;

    // Update text
    if(fTime > ALARM_TEXT_ROT_TIME)
    {
        // Update current Alarm or Goal text
        szAlarmText = Epc.CurrentGoal;

        length = Len(szAlarmText);

		// Fill with blank
		iEmptySpot = int( (CURRENT_GOAL_WIDTH - ( length * xLen ))/xLen );
		if ( iEmptySpot > 0 )
		{
			for (y = 0; y < iEmptySpot; y++)
			{
				szAlarmText = " " $ szAlarmText;
			}
		}

		// Real length
		length = Len(szAlarmText);

        iAlarmIndex++;
        if(iAlarmIndex >= length)
            iAlarmIndex = 0;

        szAlarmText = Mid(szAlarmText, iAlarmIndex, length + iEmptySpot);

        fTime = 0.0f;
    }

    //Canvas.font = Canvas.ETextFont;
    Canvas.SetPos(xPos, yPos + yLen/4);
    Canvas.DrawText(szAlarmText);
}


/*-----------------------------------------------------------------------------
 Function:      DisplayIconsGoalNoteRecon

 Description:   - When you got a new RECON,GOAL or NOTE, displays an icon
-----------------------------------------------------------------------------*/
function DisplayIconsGoalNoteRecon(ECanvas Canvas)
{
	local int xPos, yPos, temp;
	local FLOAT test;


	xPos = SCREEN_END_X - eGame.HUD_OFFSET_X - ITEMBOX_WIDTH_L;
    yPos = SCREEN_END_Y - eGame.HUD_OFFSET_Y - ITEMBOX_HEIGHT_L * 2 - ITEMBOX_HEIGHT_B - SPACE_BETWEEN_BOX * 2 - SPACE_EXTRA_GOAL - ICON_HEIGHT;

	Canvas.Style = ERenderStyle.STY_Alpha;
	Canvas.SetDrawColor(128,128,128,255);
	Canvas.SetPos(xPos,yPos);

	temp = ITEMBOX_WIDTH_L / 3;

	if ( Epc.bNewGoal && ( bBlink || bStopBlinkGoal) )
	{
		fNbrGoalBlink += FBLINKINGTIME;
		if ( fNbrGoalBlink > MAX_BLINK_TIME )
			bStopBlinkGoal = true;

		Canvas.SetPos(xPos + (temp/2) - eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_goals)/2 ,yPos);
		eLevel.TGAME.DrawTileFromManager( Canvas,
										  eLevel.TGAME.com_ic_goals,
		                                  eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_goals),
										  eLevel.TGAME.GetHeight(eLevel.TGAME.com_ic_goals),
										  0,
										  0,
										  eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_goals),
										  eLevel.TGAME.GetHeight(eLevel.TGAME.com_ic_goals) );
	}
	else
	{
		if ( bStopBlinkGoal )
		{
			bStopBlinkGoal = false;
			fNbrGoalBlink = 0;
		}
	}

	if ( Epc.bNewNote && ( bBlink || bStopBlinkNote) )
	{
		fNbrNoteBlink += FBLINKINGTIME;
		if ( fNbrNoteBlink > MAX_BLINK_TIME )
			bStopBlinkNote = true;

		Canvas.SetPos(xPos + temp + (temp/2) - eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_notes)/2 ,yPos);
		eLevel.TGAME.DrawTileFromManager( Canvas,
										  eLevel.TGAME.com_ic_notes,
		                                  eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_notes),
										  eLevel.TGAME.GetHeight(eLevel.TGAME.com_ic_notes),
										  0,
										  0,
										  eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_notes),
										  eLevel.TGAME.GetHeight(eLevel.TGAME.com_ic_notes) );
	}
	else
	{
		if ( bStopBlinkNote )
		{
			bStopBlinkNote = false;
			fNbrNoteBlink = 0;
		}
	}

	if ( Epc.bNewRecon && ( bBlink || bStopBlinkRecon) )
	{
		fNbrReconBlink += FBLINKINGTIME;
		if ( fNbrReconBlink > MAX_BLINK_TIME )
			bStopBlinkRecon = true;

		Canvas.SetPos(xPos + 2*temp + (temp/2) - eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_recon)/2 ,yPos);
		eLevel.TGAME.DrawTileFromManager( Canvas,
										  eLevel.TGAME.com_ic_recon,
		                                  eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_recon),
										  eLevel.TGAME.GetHeight(eLevel.TGAME.com_ic_recon),
										  0,
										  0,
										  eLevel.TGAME.GetWidth(eLevel.TGAME.com_ic_recon),
										  eLevel.TGAME.GetHeight(eLevel.TGAME.com_ic_recon) );
	}
	else
	{
		if ( bStopBlinkRecon )
		{
			bStopBlinkRecon = false;
			fNbrReconBlink = 0;
		}
	}

	Canvas.Style = ERenderStyle.STY_Normal;
}

defaultproperties
{
    TextColor=(R=75,G=83,B=60,A=255)
    TextSelectedColor=(R=96,G=101,B=79,A=255)
    TextDisabledColor=(R=51,G=56,B=41,A=255)
    HUDColor=(R=128,G=128,B=128,A=255)
    bHidden=true
    bAlwaysTick=true
}