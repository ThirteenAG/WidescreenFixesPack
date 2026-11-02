class EPickLockSystem extends Actor;

#exec OBJ LOAD FILE=..\Animations\ESkelIngredients.ukx

// pick lock variable
enum EPickLockQuadrant
{
	PL_UpLeft,
	PL_UpRight,
	PL_DownLeft,
	PL_DownRight,
	PL_None,
};

var int					CurrentPin;
var EPickLockQuadrant	CurrentQuadrant;
var float				PrevF, PrevS;
var float				fTimeElapsedInQuadrant, ttt;
var int					TiltInQuadrant;
var Controller			PickLocker;
var EPickLockInteraction	PLI;

function PostBeginPlay()
{
	Super.PostBeginPlay();
	// Bring pieces together
	PlayAnim('Neutral');
}

// Init system
function Init( Controller Instigator, EPickLockInteraction plint )
{
	PickLocker	= Instigator;
	PLI			= plint;

	CurrentPin	= 5;

	bRenderAtEndOfFrame=true;

	PlaySound(PLI.LockPick.StartLockPick, SLOT_Interface);
}

//------------------------------------------------------------------------
// Description
//		Move picks around
//------------------------------------------------------------------------
function MoveAxis( float aUpDown, float aLeftRight )
{
	local EPickLockQuadrant Quadrant;
	Quadrant = FindCurrentQuadrant(aUpDown, aLeftRight);

	// if we moved the pick in the same quadrant
	if( Quadrant == CurrentQuadrant && Abs(PrevF-aUpDown) + Abs(PrevS-aLeftRight) > PLI.MyDoor.MoveThreshold )
	{
		//Log("Tilting same quadrant"@CurrentQuadrant@TiltInQuadrant@fTimeElapsedInQuadrant@aUpDown@aLeftRight);

		// Send event
		Tilt(Quadrant, aUpDown, aLeftRight);
	}
	// if we moved in a different Quadrant
	else if( Quadrant != CurrentQuadrant )
	{
		//Log("Changing quadrant from"@CurrentQuadrant@"to"@Quadrant);

		// Set current quadrant
		CurrentQuadrant = Quadrant;

		// Send event
		ChangeQuadrant(Quadrant);
	}
}

function EPickLockQuadrant FindCurrentQuadrant( float aUpDown, float aLeftRight )
{
	// Joshua - Adding controller support for lockpicks
	local EPlayerController EPC;

	EPC = EPlayerController(PickLocker);

	if (!EPC.eGame.bUseController)
	{
		// For the PC, we want a single direction to avoid the wiggling on the keyboard.
		// UpLeft == up, UpRight == right, DownLeft == left, DownRight == down
		if (aUpDown == 1)
			return PL_UpLeft; // up
		else if (aLeftRight == 1)
			return PL_UpRight; // right
		else if (aLeftRight == -1)
			return PL_DownLeft; // left
		else if (aUpDown == -1)
			return PL_DownRight; // down
		else if (abs(aUpDown) == 1 && abs(aLeftRight) == 1)	// 2 keys are pressed, invalid selection
			return PL_None;
		else										// no quadrant change if border lines
			return CurrentQuadrant;
	}
	else
	{
		if (aUpDown > 0 && aLeftRight < 0)
			return PL_UpLeft;
		else if (aUpDown > 0 && aLeftRight > 0)
			return PL_UpRight;
		else if (aUpDown < 0 && aLeftRight < 0)
			return PL_DownLeft;
		else if (aUpDown < 0 && aLeftRight > 0)
			return PL_DownRight;
		else if (aUpDown == 0 && aLeftRight == 0)	// only change to None when in full center
			return PL_None;
		else										// no quadrant change if border lines
			return CurrentQuadrant;
	}
}

//------------------------------------------------------------------------
// Description
//		Playing in the same quadrant
//------------------------------------------------------------------------
function Tilt( EPickLockQuadrant Q, float UpDown, float LeftRight );

//------------------------------------------------------------------------
// Description
//		Changing quadrant ..
//------------------------------------------------------------------------
function ChangeQuadrant( EPickLockQuadrant Q );

//------------------------------------------------------------------------
// Description
//		Victory over CurrentPin, switch to next
//------------------------------------------------------------------------
function ClickNext()
{
	local name Anim;
	//Log("Q["$CurrentPin+1$"] down .. ");

	if( CurrentPin == 0 || PLI.MyDoor.Combinaison[CurrentPin-1] == PL_None )
	{
		GotoState('s_Picked');
	}
	else
	{
		switch( CurrentPin )
		{
		case 1 : Anim = 'Nt5'; break;
		case 2 : Anim = 'Nt4'; break;
		case 3 : Anim = 'Nt3'; break;
		case 4 : Anim = 'Nt2'; break;
		case 5 : Anim = 'Nt1'; break;
		}
		PlayAnim(Anim,,0.05);

		// Get down to new pin
		CurrentPin--;

		PlaySound(PLI.LockPick.ReleasePine, SLOT_Interface);

		GotoState('s_TryLock');
	}
}

auto state s_TryLock
{
	function ChangeQuadrant( EPickLockQuadrant Q )
	{
		if( Q == PLI.MyDoor.Combinaison[CurrentPin] )
			GotoState('s_PickLocking');
	}
}

state s_PickLocking
{
	function BeginState()
	{
		//Log("Valid quadrant["$CurrentPin+1$"] found ...");

		Level.RumbleShake(0.05f, 1.f);

		PlaySound(PLI.LockPick.FindPine, SLOT_Interface);

		// Reset quadrant values
		TiltInQuadrant			= 0;
		fTimeElapsedInQuadrant	= 0;
	}

	function Tick( float DeltaTime )
	{
		Level.RumbleShake(0.05f, FRand()/5.f);

		fTimeElapsedInQuadrant += DeltaTime;

		global.tick(deltaTime);

		// After [PLI.MyDoor.TimePerQuadrant] sec. in this quadrant and [TiltInQuadrant] Tilt, change to next
		//Log(DeltaTime@fTimeElapsedInQuadrant@TiltInQuadrant);
		if( fTimeElapsedInQuadrant >= PLI.MyDoor.TimePerQuadrant && TiltInQuadrant > PLI.MyDoor.TiltPerQuadrant )
			ClickNext();
	}

	function Tilt( EPickLockQuadrant Q, float UpDown, float LeftRight )
	{
		local name Anim;

		// Save current values
		PrevF = UpDown; PrevS = LeftRight;

		// Add a tilt in same quadrant
		TiltInQuadrant++;

		if(!IsPlaying(PLI.LockPick.TryLockPick))
			PlaySound(PLI.LockPick.TryLockPick, SLOT_Interface);

		switch( CurrentPin )
		{
		case 0 : Anim = 'Pin6'; break;
		case 1 : Anim = 'Pin5'; break;
		case 2 : Anim = 'Pin4'; break;
		case 3 : Anim = 'Pin3'; break;
		case 4 : Anim = 'Pin2'; break;
		case 5 : Anim = 'Pin1'; break;
		}

		if( !IsAnimating() )
			PlayAnim(Anim,2);
	}

	function ChangeQuadrant( EPickLockQuadrant Q )
	{
		GotoState('s_TryLock');
	}
}

state s_Picked
{
	function BeginState()
	{
		//Log("SUCCESS OVER LOCK");
		PlaySound(PLI.LockPick.OpenLock, SLOT_Interface);
		PlayAnim('Open');

		// Unlock door as soon as picked
		PLI.Interact(PickLocker);
	}

	function AnimEnd( optional int Channel )
	{
		PLI.PostInteract(PickLocker);
	}
}

defaultproperties
{
    DrawType=DT_Mesh
    Mesh=SkeletalMesh'ESkelIngredients.lockMesh'
    AmbientGlow=64
    SoundRadiusSaturation=500.000000
    bSpecialLit=true
}