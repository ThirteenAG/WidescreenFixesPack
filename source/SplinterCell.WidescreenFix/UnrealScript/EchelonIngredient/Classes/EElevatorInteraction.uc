class EElevatorInteraction extends EPopObjectInteraction;

var EElevatorPanel		Panel;

function string	GetDescription()
{
	return Localize("Interaction", "Elevator", "Localization\\HUD");
}

function InitInteract( Controller Instigator )
{
	Panel = EElevatorPanel(Owner);
	if( Panel == None )
		Log("ERROR: problem with EElevatorInteraction owner "$Owner);

	if( Panel.bPowered )
	{
		Super.InitInteract(Instigator);

		if( Instigator.bIsPlayer )
		{
		Instigator.GotoState('s_KeyPadInteract');
			Panel.GotoState('s_Use');
		}
		else
		{
			Instigator.GotoState('s_ElevatorInteract');
		}
	}
	else if( Instigator.bIsPlayer )
	{
		EPlayerController(Instigator).SendTransmissionMessage(Localize("Transmission", "ElevatorPower", "Localization\\HUD"), TR_CONSOLE);
		EPlayerController(Instigator).ReturnFromInteraction();
	}
}

function SetInteractLocation( Pawn InteractPawn )
{
	local Vector X, Y, Z, MovePos;
	local EPawn InteractEPawn;

	InteractEPawn = EPawn(InteractPawn);

	if (InteractEPawn != none)
	{
		// get owner rotation axes for positioning
		GetAxes(Owner.Rotation, X, Y, Z);

		MovePos = Owner.Location;
		MovePos -= (0.5 * InteractEPawn.CollisionRadius) * Y;
		MovePos += (1.25 * InteractEPawn.CollisionRadius) * X;
        if (InteractPawn.bIsPlayerPawn)
        {
		    MovePos.Z = InteractEPawn.Location.Z;
        }
        else    // Set real Z for NPCs
        {
            MovePos.Z = Owner.Location.Z;
        }

		InteractEPawn.m_locationStart	= InteractEPawn.Location;
		InteractEPawn.m_orientationStart= InteractEPawn.Rotation;
		InteractEPawn.m_locationEnd		= MovePos;
		InteractEPawn.m_orientationEnd	= Rotator(-X);
	}
}

function KeyEvent( String Key, EInputAction Action, float Delta, optional bool bAuto )
{
	// Process Npc interaction
	if( bAuto )
	{
		Action		= IST_Press;
		Key			= "Interaction";

		if( Panel.SelectedFloor < Panel.Floors.Length-1 )
			Panel.SelectedButton = 2;
		else
			Panel.SelectedButton = 1;
	}

	if( Action == IST_Press )
	{
		switch( Key )
		{
		case "AnalogUp" :
		case "MoveForward" :
			if( Panel.SelectedButton < Panel.KeyButtons.Length-1 )
				Panel.SelectedButton++;
			break;

		case "AnalogDown" :
		case "MoveBackward" :
			if( Panel.SelectedButton > 0 )
				Panel.SelectedButton--;
			break;

		case "Interaction" :
			if(bAuto)
				Panel.KeyPushed();
			break;

		case "Fire" :
			PostInteract(InteractionController);
			break;
		}

		Panel.GlowSelected();
	}
}

defaultproperties
{
    ViewRelativeLocation=(X=30.000000,Y=-8.000000,Z=0.000000)
    ViewRelativeRotation=(Pitch=-2000,Yaw=32768,Roll=0)
}