	//=============================================================================
// WindowConsole - console replacer to implement UWindow UI System
//=============================================================================
class WindowConsole extends Console;

// Constants.
const MaxLines=64;
const TextMsgSize=128;

// Variables.
var viewport            Viewport;
var int                 Scrollback, NumLines, TopLine, TextLines;
var float               MsgTime, MsgTickTime;
var string              MsgText[64];
var float               MsgTick[64];
var int                 ConsoleLines;
var bool                bNoStuff, bTyping;
var bool                bShowLog;


// ---------


var UWindowRootWindow	Root;
var() config string		RootWindow;

var float				OldClipX;
var float				OldClipY;
var bool				bCreatedRoot;
var float				MouseX;
var float				MouseY;

var class<UWindowConsoleWindow> ConsoleClass;
var config float		MouseScale;

var config bool			bShowConsole;
var bool				bBlackout;
var bool				bUWindowType;

var bool				bUWindowActive;
var bool				bQuickKeyEnable;
var bool				bLocked;
var bool				bLevelChange;
var string				OldLevel;

var config EInputKey	UWindowKey;

//ORIGINAL UNREAL CONSOLE var UWindowConsoleWindow ConsoleWindow; 



function ResetUWindow()
{
	if(Root != None)
		Root.Close();
	Root = None;
	bCreatedRoot = False;
//ORIGINAL UNREAL CONSOLE	ConsoleWindow = None;
	bShowConsole = False;
	CloseUWindow();
}

function bool KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
{
	local byte k;
	k = Key;
	switch(Action)
	{
	case IST_Press:
		switch(k)
		{
		case EInputKey.IK_Escape:
			log("WindowConsole.uc, KeyEvent, IST_Press IK_Escape");
			if (bLocked)
				return true;

			bQuickKeyEnable = False;
			LaunchUWindow();
			return true;
		case ConsoleKey:
			if (bLocked)
				return true;

			bQuickKeyEnable = True;
			LaunchUWindow();
			if(!bShowConsole)
				ShowConsole();
			return true;
		}
		break;
	}

	return False; 
	//!! because of ConsoleKey
	//!! return Super.KeyEvent(Key, Action, Delta);
}

function ShowConsole()
{
	bShowConsole = true;
//ORIGINAL UNREAL CONSOLE	if(bCreatedRoot)
//ORIGINAL UNREAL CONSOLE		ConsoleWindow.ShowWindow();
}

function HideConsole()
{
	ConsoleLines = 0;
	bShowConsole = false;
//ORIGINAL UNREAL CONSOLE	if (ConsoleWindow != None)
//ORIGINAL UNREAL CONSOLE		ConsoleWindow.HideWindow();
}

event Tick( float Delta )
{
	Super.Tick(Delta);

	if(bLevelChange && Root != None && string(ViewportOwner.Actor.Level) != OldLevel)
	{
		OldLevel = string(ViewportOwner.Actor.Level);
		// if this is Entry, we could be falling through to another level...
		if(ViewportOwner.Actor.Level != ViewportOwner.Actor.GetEntryLevel())
			bLevelChange = False;
		Root.NotifyAfterLevelChange();
	}
}

state UWindow
{
	event Tick( float Delta )
	{
		Global.Tick(Delta);
		if(Root != None)
			Root.DoTick(Delta);
	}

	function PostRender( canvas Canvas )
	{
		if(Root != None)
			Root.bUWindowActive = True;
		RenderUWindow( Canvas );
	}

	function bool KeyType( EInputKey Key )
	{
		if (Root != None)
			Root.WindowEvent(WM_KeyType, None, MouseX, MouseY, Key);
		return True;
	}

	function bool KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
	{
		local byte k;
		k = Key;

		switch (Action)
		{
		case IST_Release:
			switch (k)
			{
			case EInputKey.IK_LeftMouse:
				if(Root != None) 
					Root.WindowEvent(WM_LMouseUp, None, MouseX, MouseY, k);
				break;
			case EInputKey.IK_RightMouse:
				if(Root != None)
					Root.WindowEvent(WM_RMouseUp, None, MouseX, MouseY, k);
				break;
			case EInputKey.IK_MiddleMouse:
				if(Root != None)
					Root.WindowEvent(WM_MMouseUp, None, MouseX, MouseY, k);
				break;
			default:
				if(Root != None)
					Root.WindowEvent(WM_KeyUp, None, MouseX, MouseY, k);
				break;
			}
			break;

		case IST_Press:
			switch (k)
			{
			case EInputKey.IK_F9:	// Screenshot
				return Global.KeyEvent(Key, Action, Delta);
				break;
			case ConsoleKey:
				if (bShowConsole)
				{
					HideConsole();
					if(bQuickKeyEnable)
						CloseUWindow();
				}
				else
				{
					if(Root.bAllowConsole)
						ShowConsole();
					else
						Root.WindowEvent(WM_KeyDown, None, MouseX, MouseY, k);
				}
				break;
			case EInputKey.IK_Escape:
				log("WindowConsole.uc, state UWindow, KeyEvent, IST_Press IK_Escape");
				if(Root != None)
					Root.CloseActiveWindow();
				break;
			case EInputKey.IK_LeftMouse:
				if(Root != None)
					Root.WindowEvent(WM_LMouseDown, None, MouseX, MouseY, k);
				break;
			case EInputKey.IK_RightMouse:
				if(Root != None)
					Root.WindowEvent(WM_RMouseDown, None, MouseX, MouseY, k);
				break;
			case EInputKey.IK_MiddleMouse:
				if(Root != None)
					Root.WindowEvent(WM_MMouseDown, None, MouseX, MouseY, k);
				break;
			default:
				if(Root != None)
					Root.WindowEvent(WM_KeyDown, None, MouseX, MouseY, k);
				break;
			}
			break;
		case IST_Axis:
			switch (Key)
			{
			case IK_MouseX:
				MouseX = MouseX + (MouseScale * Delta);
				break;
			case IK_MouseY:
				MouseY = MouseY - (MouseScale * Delta);
				break;					
			}
		default:
			break;
		}

		return true;
	}

Begin:
}

function ToggleUWindow()
{
}

function LaunchUWindow()
{

	ViewportOwner.bSuspendPrecaching = True;
	bUWindowActive = !bQuickKeyEnable;
	ViewportOwner.bShowWindowsMouse = True;

	if(Root != None)
		Root.bWindowVisible = True;

	GotoState('UWindow');
}

function CloseUWindow()
{
	if(!bQuickKeyEnable && ViewportOwner.Actor != None)
		ViewportOwner.Actor.SetPause( False );

	bQuickKeyEnable = False;
	bUWindowActive = False;
	ViewportOwner.bShowWindowsMouse = False;

	if(Root != None)
		Root.bWindowVisible = False;
	GotoState('');
	ViewportOwner.bSuspendPrecaching = False;
}

function CreateRootWindow(Canvas Canvas)
{
	local int i;

	if(Canvas != None)
	{
		OldClipX = Canvas.ClipX;
		OldClipY = Canvas.ClipY;
	}
	else
	{
		OldClipX = 0;
		OldClipY = 0;
	}
	
	Log("Creating root window: "$RootWindow);
	
	Root = New(None) class<UWindowRootWindow>(DynamicLoadObject(RootWindow, class'Class'));

	Root.BeginPlay();
	Root.WinTop = 0;
	Root.WinLeft = 0;

	if(Canvas != None)
	{
		Root.WinWidth = Canvas.ClipX / Root.GUIScale;
		Root.WinHeight = Canvas.ClipY / Root.GUIScale;
		Root.RealWidth = Canvas.ClipX;
		Root.RealHeight = Canvas.ClipY;
	}
	else
	{
		Root.WinWidth = 0;
		Root.WinHeight = 0;
		Root.RealWidth = 0;
		Root.RealHeight = 0;
	}

	Root.ClippingRegion.X = 0;
	Root.ClippingRegion.Y = 0;
	Root.ClippingRegion.W = Root.WinWidth;
	Root.ClippingRegion.H = Root.WinHeight;

	Root.Console = Self;

	Root.bUWindowActive = bUWindowActive;

	Root.Created();
	bCreatedRoot = True;

	// Create the console window.
//ORIGINAL UNREAL CONSOLE	ConsoleWindow = UWindowConsoleWindow(Root.CreateWindow(ConsoleClass, 100, 100, 200, 200));
	if(!bShowConsole)
		HideConsole();

//ORIGINAL UNREAL CONSOLE	UWindowConsoleClientWindow(ConsoleWindow.ClientArea).TextArea.AddText(" ");
//ORIGINAL UNREAL CONSOLE	for (I=0; I<4; I++)
//ORIGINAL UNREAL CONSOLE		UWindowConsoleClientWindow(ConsoleWindow.ClientArea).TextArea.AddText(MsgText[I]);
}

function RenderUWindow( canvas Canvas )
{
	local UWindowWindow NewFocusWindow;

	Canvas.bNoSmooth = False;
	Canvas.Z = 1;
	Canvas.Style = 1;
	Canvas.DrawColor.r = 255;
	Canvas.DrawColor.g = 255;
	Canvas.DrawColor.b = 255;

// R6CODE
    MouseScale = 0.6f;


	if(ViewportOwner.bWindowsMouseAvailable && Root != None)
	{
		MouseX = ViewportOwner.WindowsMouseX/Root.GUIScale;
		MouseY = ViewportOwner.WindowsMouseY/Root.GUIScale;
	}

	if(!bCreatedRoot) 
		CreateRootWindow(Canvas);

	Root.bWindowVisible = True;
	Root.bUWindowActive = bUWindowActive;
	Root.bQuickKeyEnable = bQuickKeyEnable;

	if(Canvas.ClipX != OldClipX || Canvas.ClipY != OldClipY)
	{
		OldClipX = Canvas.ClipX;
		OldClipY = Canvas.ClipY;
		
		Root.WinTop = 0;
		Root.WinLeft = 0;
		Root.WinWidth = Canvas.ClipX / Root.GUIScale;
		Root.WinHeight = Canvas.ClipY / Root.GUIScale;

		Root.RealWidth = Canvas.ClipX;
		Root.RealHeight = Canvas.ClipY;

		Root.ClippingRegion.X = 0;
		Root.ClippingRegion.Y = 0;
		Root.ClippingRegion.W = Root.WinWidth;
		Root.ClippingRegion.H = Root.WinHeight;

		Root.Resized();
	}

	//if(MouseX > Root.WinWidth) MouseX = Root.WinWidth;
	//if(MouseY > Root.WinHeight) MouseY = Root.WinHeight;
    if(MouseX > 629) MouseX   = 629; //640-11 //clauzon fix, mouse out of window
    if(MouseY > 461) MouseY   = 461; //480-19
	if(MouseX < 0) MouseX = 0;
	if(MouseY < 0) MouseY = 0;


	// Check for keyboard focus
	NewFocusWindow = Root.CheckKeyFocusWindow();

	if(NewFocusWindow != Root.KeyFocusWindow)
	{
		Root.KeyFocusWindow.KeyFocusExit();		
		Root.KeyFocusWindow = NewFocusWindow;
		Root.KeyFocusWindow.KeyFocusEnter();
	}

	

	Root.MoveMouse(MouseX, MouseY);
	Root.WindowEvent(WM_Paint, Canvas, MouseX, MouseY, 0);
	if ((bUWindowActive || bQuickKeyEnable) && ViewportOwner.bShowWindowsMouse)
		Root.DrawMouse(Canvas);
}

event Message( coerce string Msg, float MsgLife )
{
	Super.Message( Msg, MsgLife );

	if ( ViewportOwner.Actor == None )
		return;

//ORIGINAL UNREAL CONSOLE	if( (Msg!="") && (ConsoleWindow != None) )
//ORIGINAL UNREAL CONSOLE		UWindowConsoleClientWindow(ConsoleWindow.ClientArea).TextArea.AddText(MsgText[TopLine]);
}

function UpdateHistory()
{
	// Update history buffer.
	History[HistoryCur++ % MaxHistory] = TypedStr;
	if( HistoryCur > HistoryBot )
		HistoryBot++;
	if( HistoryCur - HistoryTop >= MaxHistory )
		HistoryTop = HistoryCur - MaxHistory + 1;
}

function HistoryUp()
{
	if( HistoryCur > HistoryTop )
	{
		History[HistoryCur % MaxHistory] = TypedStr;
		TypedStr = History[--HistoryCur % MaxHistory];
	}
}

function HistoryDown()
{
	History[HistoryCur % MaxHistory] = TypedStr;
	if( HistoryCur < HistoryBot )
		TypedStr = History[++HistoryCur % MaxHistory];
	else
		TypedStr="";
}

function NotifyLevelChange()
{
//	Super.NotifyLevelChange();
    log("WindowConsole NotifyLevelChange");
	bLevelChange = True;
	if(Root != None)
		Root.NotifyBeforeLevelChange();
}

function NotifyAfterLevelChange()
{
    log("WindowConsole NotifyAfterLevelChange");
	if(bLevelChange && Root != None)
	{	
	    bLevelChange = False;
		Root.NotifyAfterLevelChange();
	}
}

defaultproperties
{
    RootWindow="UWindow.UWindowRootWindow"
    MouseScale=0.600000
}