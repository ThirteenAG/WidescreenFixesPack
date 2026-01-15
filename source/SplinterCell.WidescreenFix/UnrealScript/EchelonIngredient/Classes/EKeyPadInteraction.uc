class EKeyPadInteraction extends EPopObjectInteraction;

var EKeyPad		MyKeyPad;

function PostBeginPlay()
{
	GetKeyPad();
	Super.PostBeginPlay();
}

function string	GetDescription()
{
	return Localize("Interaction", "KeyPad", "Localization\\HUD");
}

function bool IsAvailable()
{
	return MyKeyPad.GetStateName() == 's_Idle' && Super.IsAvailable();
}

function LockOwner( bool bLocked )
{
	if( MyKeyPad == None )
		return;

	if( bLocked )
		MyKeyPad.GotoState('s_Use');
	else
		MyKeyPad.GotoState('s_Idle');
}

function InitInteract( Controller Instigator )
{
	if( MyKeyPad == None || MyKeyPad.LinkedActors.Length == 0 )
	{
		Log("Something wrong in EKeyPadInteraction"@Owner@MyKeyPad.LinkedActors.Length);
		return;
	}

	Super.InitInteract(Instigator);

	// Lock as soon as used.  Npc lock it upon reaching the keypad
	if( Instigator.bIsPlayer )
		LockOwner(true);
	Instigator.GotoState('s_KeyPadInteract');
}

function GetKeyPad()
{
	MyKeyPad = EKeyPad(Owner);
	if ( MyKeyPad == None )
		Log("ERROR: problem with EKeyPadInteraction owner "$Owner);
}

function SetInteractLocation( Pawn InteractPawn )
{
	local Vector X, Y, Z, MovePos;
	local EPawn InteractEPawn;
	local vector HitLocation, HitNormal;

	InteractEPawn = EPawn(InteractPawn);

	if (InteractEPawn != none)
	{
		// get MyKeyPad object rotation axes for positioning
		GetAxes(MyKeyPad.Rotation, X, Y, Z);
		MovePos = MyKeyPad.Location;

		MovePos += (1.3f*InteractEPawn.CollisionRadius) * X;

		if(InteractEPawn.bIsPlayerPawn)
		{
			MovePos.Z	= InteractEPawn.Location.Z;									// keep on same Z
		}
		else
		{
			if( Trace(HitLocation, HitNormal, MovePos + vect(0,0,-200), MovePos,,,,,true) != None )
			{
				HitLocation.Z += InteractEPawn.CollisionHeight;
				MovePos = HitLocation;
			}
		}

		InteractEPawn.m_locationStart	= InteractEPawn.Location;
		InteractEPawn.m_orientationStart= InteractEPawn.Rotation;
		InteractEPawn.m_locationEnd		= MovePos;
		InteractEPawn.m_orientationEnd	= Rotator(-X);
	}
}

function KeyEvent( String Key, EInputAction Action, float Delta, optional bool bAuto )
{
	local EPlayerController EPC; // Joshua - Adding controller support for keypads
	EPC = EPlayerController(InteractionController);

	// Process Npc interaction
	if( bAuto )
	{
		Action		= IST_Press;
		Key			= "Interaction";
		MyKeyPad.SelectedButton	= GetValidkey();
		if( MyKeyPad.SelectedButton == -1 )
			return;
	}

	if( MyKeyPad.GetStateName() != 's_Use' )
		return;

	if(Action == IST_Press)
	{
		switch( Key )
		{
		case "AnalogUp" :
		case "MoveForward" :
			if( MyKeyPad.SelectedButton > 2 )
				MyKeyPad.SelectedButton -= 3;
			break;

		case "AnalogDown" :
		case "MoveBackward" :
			if( MyKeyPad.SelectedButton < 9 )
				MyKeyPad.SelectedButton += 3;
			break;

		case "AnalogLeft" :
		case "StrafeLeft" :
			if( MyKeyPad.SelectedButton > 0 && MyKeyPad.SelectedButton % 3 != 0 )
				MyKeyPad.SelectedButton -= 1;
			break;

		case "AnalogRight" :
		case "StrafeRight" :
			if( MyKeyPad.SelectedButton < 11 && (MyKeyPad.SelectedButton+1) % 3 != 0 )
				MyKeyPad.SelectedButton += 1;
			break;

		case "Fire" :
			PostInteract(InteractionController);
			break;

		case "Interaction" :
			// Joshua - Adding controller support for keypads
			if (EPC != None && EPC.eGame.bUseController)
				MyKeyPad.KeyPushed();
			else if (bAuto)
				MyKeyPad.KeyPushed();
			break;
		}

		MyKeyPad.GlowSelected();
	}
}

//------------------------------------------------------------------------
// Description		Must return the valid key for Npc
//------------------------------------------------------------------------
function int GetValidkey()
{
	local int i, Index;

	Index = Len(MyKeyPad.Inputedcode);
	for( i=0; i<ArrayCount(MyKeyPad.KeyButtons); i++ )
	{
		if( MyKeyPad.KeyButtons[i].Value == Mid(MyKeyPad.AccessCode, Index, 1) )
			return i;
	}

	return -1;
}

defaultproperties
{
    ViewRelativeLocation=(X=15.000000,Y=-4.000000,Z=0.000000)
    ViewRelativeRotation=(Pitch=-2000,Yaw=32768,Roll=0)
}