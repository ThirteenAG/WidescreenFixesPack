class EElevatorPanel extends EGameplayObject;

#exec OBJ LOAD FILE=..\StaticMeshes\EMeshIngredient.usx
#exec OBJ LOAD FILE=..\Sounds\Door.uax

struct EElevatorFloor
{
	var()	int					FloorNumber;				// Floor #
	var()	int					ElevatorKeyNum;				// Elevator mover key
	var		Vector				SavedKeyPosition;			// Saved position for the elevator at current floor
	var()	array<EDoorMover>	FloorElevatorDoors;			// Doors to open at current floor
	var()	EElevatorButton		FloorButton;				// Button to call the elevator on that floor
};

var()	bool						bPowered;
var()	Mover						Elevator;				// Elevator mover
var()	Array<EDoorMover>			ElevatorDoors;			// Elevator Inside doors
var()	array<EElevatorFloor>		Floors;					// All floors for the elevator
var()	array<EElevatorDisplay>		Displays;				// All display panel for elevator
var		int							PendingFloor;			// If a floor is chosen while Panel is locked.
var		int							SelectedFloor;

var		array<EKeyButton>			KeyButtons;					// All Key buttons
var		array<EGameplayObject>		MeshCode;
var		int							SelectedButton;
var		int							DoorEndEvents;
var		bool						bCanPushButton;			// Can push as soon as it's released
var		bool						bLocked;
var		bool						bWasUsingController;	// Joshua - Track previous controller state

// Numbering vars
var		float						TimeBetweenEachFloor;
var		float						StartTime;
var		int							PreviousFloor;			// Numberings
var		int							InterpolatingFloor;		// Current floor displayed while interpolating

function PostBeginPlay()
{
	local int i,j;

	CreateKey(StaticMesh'elevatorpanel_buttonopen', "Open");
	CreateKey(StaticMesh'elevatorpanel_buttondown', "Down");
	CreateKey(StaticMesh'elevatorpanel_buttonup', "Up");

	// Elevator checks
	if( Elevator == None )
		Log("ERROR: problem with owner elevator for "$self);
	else if( Elevator.InitialState != 'TriggerControl' )
		Log("ERROR: problem with elevator .. not in TriggerControl state "$Elevator.InitialState);

	Elevator.InitialState = 'TriggerControl';
	Elevator.MoverEncroachType = ME_IgnoreWhenEncroach;

	if( AttachTag == '' )
	{
		Log(self$" WARNING: AttachTag is '' for panel .. Will be automatically attached to Elevator["$Elevator$"]");
		SetBase(Elevator);
	}

	// Lock linked doors to prevent bump opening
	for( i=0; i<ElevatorDoors.Length; i++ )
	{
		if( ElevatorDoors[i].AttachTag == '' )
		{
			Log(self$" WARNING: AttachTag is '' for elevator doors .. Will be automatically attached to Elevator["$Elevator$"]");
			ElevatorDoors[i].SetBase(Elevator);
		}
		ElevatorDoors[i].Locked = true;
		ElevatorDoors[i].bNoPlayerStayOpen = true;
	}

	for( i=0; i<Floors.Length; i++ )
	{
		Floors[i].SavedKeyPosition = Elevator.KeyPos[i];

		// Set Panel in the button
		if( Floors[i].FloorButton != None )
			Floors[i].FloorButton.SetController(self);

		// Lock linked floor doors to prevent bump opening
		for( j=0; j<Floors[i].FloorElevatorDoors.Length; j++ )
		{
			Floors[i].FloorElevatorDoors[j].Locked = true;
			Floors[i].FloorElevatorDoors[j].bNoPlayerStayOpen = true;
		}
	}

	// Create number mesh
	CreateTextMesh();

	// Init all
	PendingFloor = -1;
	SelectedFloor = Elevator.KeyNum;
	UpdateDisplays(Floors[SelectedFloor].FloorNumber);
	CheckDoorEndEvents();

	Super.postBeginPlay();
}

function CreateKey( StaticMesh Mesh, string Val )
{
	KeyButtons[KeyButtons.Length] = spawn(class'EKeyButton', self);
	KeyButtons[KeyButtons.Length-1].SetStaticMesh(Mesh);
	KeyButtons[KeyButtons.Length-1].SetRelativeOffset(Vect(0,0,0));
	KeyButtons[KeyButtons.Length-1].value = Val;
}

function CreateTextMesh()
{
	local int i;
	local vector pos;

	for( i=0; i<2; i++ )
	{
		pos = Vect(0.15,-3,8.4);
		pos.Y += i * 0.6f * 3; // drawscale

		MeshCode[i] = spawn(class'EGameplayObject', self);
		MeshCode[i].SetCollision(false);
		MeshCode[i].SetBase(self);
		MeshCode[i].SetStaticMesh(None);
		MeshCode[i].SetRelativeLocation(pos);
		MeshCode[i].SetDrawScale(3.f);
		MeshCode[i].Style = STY_Modulated;
		MeshCode[i].bUnlit = true;
		MeshCode[i].HeatIntensity = 0.5;
	}
}

//------------------------------------------------------------------------
// Description
//		When you push an elevator button (floor)
//------------------------------------------------------------------------
function RequestElevator( EElevatorButton Button )
{
	local int i;

	if( !bPowered )
		return;

	//Log("RequestElevator SelectedFloor["$SelectedFloor$"] Locked["$bLocked$"] DoorEnd["$DoorEndEvents$"]");

	if( Button == None )
		Log(self$" PROBLEM: button not valid");

	for( i=0; i<Floors.Length; i++ )
	{
		if( Floors[i].FloorButton == Button )
		{
			SendElevator(i);
			break;
		}
	}
}

//------------------------------------------------------------------------
// Description
//		Show selected
//------------------------------------------------------------------------
function GlowSelected()
{
	local int i;
	for( i=0; i<KeyButtons.Length; i++ )
	{
		// set panel setting
		KeyButtons[i].bRenderAtEndOfFrame = bRenderAtEndOfFrame;
		KeyButtons[i].bSpecialLit = bSpecialLit;
		KeyButtons[i].AmbientGlow = AmbientGlow;
	}
	if( SelectedButton != -1 )
		KeyButtons[SelectedButton].AmbientGlow *= 2;

	for( i=0; i<MeshCode.Length; i++ )
	{
		MeshCode[i].bRenderAtEndOfFrame = bRenderAtEndOfFrame;
		MeshCode[i].bSpecialLit = bSpecialLit;
	}
}
//------------------------------------------------------------------------
// Description
//		Auto choose what floor
//------------------------------------------------------------------------
function KeyPushed()
{
	if( !bPowered )
		return;

	//Log("Key pushed");

	KeyButtons[SelectedButton].Push(true);
	PlaySound(Sound'Electronic.Play_keyPadButton', SLOT_Interface);

	switch( KeyButtons[SelectedButton].Value )
	{
	case "Up" :
		if( SelectedFloor < Floors.Length-1 )
			SendElevator(SelectedFloor+1);
		break;

	case "Down" :
		if( SelectedFloor > 0 )
			SendElevator(SelectedFloor-1);
		break;

	case "Open" :
		SendElevator(SelectedFloor);
		break;
	}
}

//------------------------------------------------------------------------
// Description
//		Send elevator to selected floor
//------------------------------------------------------------------------
function SendElevator( int SentFloor, optional bool bAutomatic )
{
	//Log("Sending elevator from current["$SelectedFloor$"] to ["$SentFloor$"]");
	PendingFloor = -1;

	// Try to remove panel when button is pushed
	if( !bAutomatic &&
		Interaction != None &&
		EElevatorInteraction(Interaction).InteractionController != None &&
		EElevatorInteraction(Interaction).InteractionController.bIsPlayer )
	{
		Interaction.PostInteract(EElevatorInteraction(Interaction).InteractionController);
	}

	// Do nothing if elevator is already at selected floor but open doors
	if( SelectedFloor == SentFloor && Elevator.BasePos+Floors[SelectedFloor].SavedKeyPosition == Elevator.Location )
	{
		//Log("Elevator is at same floor as SelectedFloor");

		// reset this to have door opening right away
		DoorEndEvents = -1;
		EndEvent();

		return;
	}
	else if( bLocked )
	{
		//Log("set pending"@sentfloor);
		PendingFloor = SentFloor;

		return;
	}

	//
	// PREPARE FLOOR NUMBERING
	//
	PreviousFloor		= SelectedFloor;
	InterpolatingFloor	= Floors[SelectedFloor].FloorNumber;
	SelectedFloor		= SentFloor;
	StartTime			= Level.TimeSeconds;
	TimeBetweenEachFloor = Elevator.MoveTime / Abs(Floors[SelectedFloor].FloorNumber - Floors[PreviousFloor].FloorNumber);
	//Log("From "$Floors[PreviousFloor].FloorNumber$" to "$Floors[SelectedFloor].FloorNumber$" in["$TimeBetweenEachFloor$"] secs each");

	// TriggerControl always sends to key 1 .. so take saved pos and put it in key 1
	Elevator.KeyPos[1] = Floors[SelectedFloor].SavedKeyPosition;

	// Trigger elevator move
	Elevator.Trigger(self, None);

	Elevator.PlaySound(Sound'Door.Play_ModernLiftRun', SLOT_SFX);

	bLocked = true;
	//Log("Sent to keynum"@Floors[SelectedFloor].ElevatorKeyNum);
}

function Trigger( Actor Other, Pawn EventInstigator, optional name InTag )
{
	Super.Trigger(Other, EventInstigator, InTag);

	//Log(self$" Triggered["$!bPowered$"] by ["$Other$"]");

	bPowered = !bPowered;
}

function CheckDoorEndEvents()
{
	if( DoorEndEvents == 0 )
	{
		//Log(self$" gets unlocked after door endEvents()");
		DoorEndEvents = -1;
		bLocked = false;
	}
	else
		bLocked = true;
}

//------------------------------------------------------------------------
// Description
//		When the elevator/doors finishes it's move
//------------------------------------------------------------------------
function EndEvent()
{
	local int i;

	//Log(self$" Receives a EndEvent() left["$DoorEndEvents$"]");

	// Check Door EndEvent
	if( DoorEndEvents != -1 )
	{
		DoorEndEvents--;
		CheckDoorEndEvents();

		if( !bLocked && PendingFloor != -1 )
		{
			//Log("send elevator to pending"@pendingfloor);
			SendElevator(PendingFloor, true);
		}
		return;
	}

	if ( Elevator.IsPlaying(Sound'Door.Play_ModernLiftRun') )
		Elevator.PlaySound(Sound'Door.Stop_ModernLiftRun', SLOT_SFX);

	// Displays
	UpdateDisplays(Floors[SelectedFloor].FloorNumber);

	// Floor Button
	Floors[SelectedFloor].FloorButton.UnTrigger(self,None);

	// Set this value when elevator gets to floor.
	// Floor doors will now each send an EndEvent to self,
	// after all doors close, elevator gets its control unlocked to change floor
	DoorEndEvents = Floors[SelectedFloor].FloorElevatorDoors.Length + ElevatorDoors.Length;
	// Lock elevator panel
	CheckDoorEndEvents();

	// Link elevator doors to floor doors
	for( i=0; i<ElevatorDoors.Length; i++ )
	{
		if( i+1 != ElevatorDoors.Length ) // i door linked to i+1
			ElevatorDoors[i].LinkedDoor = ElevatorDoors[i+1];
		else if( Floors[SelectedFloor].FloorElevatorDoors.Length > 0 ) // last door linked to first floor door
			ElevatorDoors[i].LinkedDoor = Floors[SelectedFloor].FloorElevatorDoors[0];
	}
	for( i=0; i<Floors[SelectedFloor].FloorElevatorDoors.Length; i++ )
	{
		if( i+1 != Floors[SelectedFloor].FloorElevatorDoors.Length ) // i floor door linked to i+1
			Floors[SelectedFloor].FloorElevatorDoors[i].LinkedDoor = Floors[SelectedFloor].FloorElevatorDoors[i+1];
		else if( ElevatorDoors.Length > 0 ) // last door linked to first floor door
			Floors[SelectedFloor].FloorElevatorDoors[i].LinkedDoor = ElevatorDoors[0];
	}

	// Open elevator doors
	for( i=0; i<ElevatorDoors.Length; i++ )
	{
		// do this to assure that triggering the button will always open the doors.
		if( ElevatorDoors[i].bClosing )
			ElevatorDoors[i].Enable('Trigger');
		ElevatorDoors[i].Trigger(self, None);
	}

	// Open floor linked doors
	for( i=0; i<Floors[SelectedFloor].FloorElevatorDoors.Length; i++ )
	{
		// do this to assure that triggering the button will always open the doors.
		if( Floors[SelectedFloor].FloorElevatorDoors[i].bClosing )
			Floors[SelectedFloor].FloorElevatorDoors[i].Enable('Trigger');
		Floors[SelectedFloor].FloorElevatorDoors[i].Trigger(self, None);
	}
}

// Support to go from Basement 9 to Floor 9
function UpdateDisplays( int FloorNumber )
{
	local StaticMesh NewDigit0, NewDigit1;
	local int i;

	// Set B if under 0 else stay None
	if( FloorNumber < 0 )
		NewDigit1 = StaticMesh'keypad_Bdigit';

	switch( Abs(FloorNumber) )
	{
	case 0 : NewDigit0 = StaticMesh'keypad_Gdigit'; break;
	case 1 : NewDigit0 = StaticMesh'keypad_1digit'; break;
	case 2 : NewDigit0 = StaticMesh'keypad_2digit'; break;
	case 3 : NewDigit0 = StaticMesh'keypad_3digit'; break;
	case 4 : NewDigit0 = StaticMesh'keypad_4digit'; break;
	case 5 : NewDigit0 = StaticMesh'keypad_5digit'; break;
	case 6 : NewDigit0 = StaticMesh'keypad_6digit'; break;
	case 7 : NewDigit0 = StaticMesh'keypad_7digit'; break;
	case 8 : NewDigit0 = StaticMesh'keypad_8digit'; break;
	case 9 : NewDigit0 = StaticMesh'keypad_9digit'; break;
	}

	//Log("Updating displays to floor["$FloorNumber$"] mesh["$NewDigit$"]");

	for( i=0; i<Displays.Length; i++ )
		Displays[i].UpdateDisplays(NewDigit0, NewDigit1);

	// Change my mesh
	MeshCode[0].SetStaticMesh(NewDigit0);
	MeshCode[1].SetStaticMesh(NewDigit1);
}

function Tick( float DeltaTime )
{
	if( Elevator == None || !Elevator.bInterpolating )
		return;

	if( Level.TimeSeconds - StartTime >= TimeBetweenEachFloor )
	{
		// Update start time
		StartTime += TimeBetweenEachFloor;

		// fond new floor
		if( Floors[PreviousFloor].FloorNumber < Floors[SelectedFloor].FloorNumber )
			InterpolatingFloor++;
		else
			InterpolatingFloor--;

		// Update screens
		UpdateDisplays(InterpolatingFloor);
	}
}

auto state s_Idle
{
	function BeginState()
	{
		// No special display if not player
		if( Interaction != None &&
			EElevatorInteraction(Interaction).InteractionController != None &&
			EElevatorInteraction(Interaction).InteractionController.bIsPlayer )
		{
		bRenderAtEndOfFrame = false;
		bSpecialLit = false;
		AmbientGlow = 0;
		}
		GlowSelected();
	}
}

state s_Use
{
	function BeginState()
	{
		local EPlayerController Epc;
		Epc = EPlayerController(EElevatorInteraction(Interaction).InteractionController);

		// No special display if not player
		if( Epc != None )
		{
			// Joshua - Adding controller support for elevators
			bWasUsingController = Epc.eGame.bUseController;
			if( !Epc.eGame.bUseController )
				Epc.FakeMouseToggle(true);
			bRenderAtEndOfFrame = true;
			bSpecialLit = true;
			AmbientGlow = default.AmbientGlow;
		}
		GlowSelected();
	}

	function EndState()
	{
		local EPlayerController Epc;
		Epc = EPlayerController(EElevatorInteraction(Interaction).InteractionController);

		// No special display if not player
		if( Epc != None )
			Epc.FakeMouseToggle(false);
	}

	function Tick( float DeltaTime )
	{
		local int OldSelectedButton;
		local EPlayerController Epc;
		Epc = EPlayerController(EElevatorInteraction(Interaction).InteractionController);

		OldSelectedButton = SelectedButton;

		Global.Tick(DeltaTime);

		if( Epc == None )
			return;

		// Joshua - Detect controller state changes and toggle fake mouse
		if (Epc.eGame.bUseController != bWasUsingController)
		{
			bWasUsingController = Epc.eGame.bUseController;
			if (bWasUsingController)
			{
				// Joshua - Switched to controller
				Epc.FakeMouseToggle(false);

				// Joshua - If no button selected, select Open button
				if (SelectedButton == -1)
				{
					SelectedButton = 0;
					GlowSelected();
				}
			}
			else
			{
				// Joshua - Switched to keyboard
				Epc.FakeMouseToggle(true);
			}
		}

		// Joshua - Only process mouse input when not using controller
		if (!Epc.eGame.bUseController)
		{
			//
			// Crappy button selection
			//
			if (CoordinateWithin(Epc, 178, 174, 55, 55))
				SelectedButton = 2;
			else if (CoordinateWithin(Epc, 182, 240, 55, 55))
				SelectedButton = 1;
			else if (CoordinateWithin(Epc, 188, 316, 55, 55))
				SelectedButton = 0;
			else
				SelectedButton = -1;

			//
			// Manage Mouse click
			//
			if (Epc.m_FakeMouseClicked)
			{
				if (SelectedButton != -1)
					KeyPushed();
				else if (!CoordinateWithin(Epc, 110, 70, 197, 317))
					Interaction.PostInteract(EElevatorInteraction(Interaction).InteractionController);
			}
			Epc.m_FakeMouseClicked = false;

			// Change selection
			if (OldSelectedButton != SelectedButton)
				GlowSelected();
		}
	}

	function bool CoordinateWithin( EPlayerController Epc, float x, float y, int w, int h )
	{
		return Epc.m_FakeMouseX > x && Epc.m_FakeMouseX < x + w &&
			   Epc.m_FakeMouseY > y && Epc.m_FakeMouseY < y + h;
	}
}

defaultproperties
{
    bPowered=true
    bDamageable=false
    bAcceptsProjectors=false
    StaticMesh=StaticMesh'EMeshIngredient.Elevator.elevatorpanel'
    AmbientGlow=64
    HeatIntensity=0.800000
    InteractionClass=Class'EElevatorInteraction'
}