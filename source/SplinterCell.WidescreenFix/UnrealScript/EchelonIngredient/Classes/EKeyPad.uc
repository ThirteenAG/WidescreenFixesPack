class EKeyPad extends EDoorOpener;

#exec OBJ LOAD FILE=..\StaticMeshes\EMeshIngredient.usx
#exec OBJ LOAD FILE=..\Sounds\Electronic.uax

var			EKeyButton		KeyButtons[12];		// All KeyPad buttons
var			EGameplayObject MeshCode[6];
var			EGameplayObject	RedLight,
							GreenLight,
							dDenied,
							dGranted;

var()		String			AccessCode;			// Code to open the door
var()		float			CodeIntensity[6];	// intensity Value for each inputed key
var()		float			FadeOutTimer;
var()		name			FadeOutJumpLabel;
var			bool			bButtonFaded;
var			bool			bRunTimer;
var			bool			bNoTimer;
var			String			InputedCode;		// Player inputed code
var			int				SelectedButton;
var			float			FadeElapsedTime;

var const	vector			CONST_TextStart;

function PostBeginPlay()
{
	// Creating all buttons
	KeyButtons[0] = CreateButton(StaticMesh'keypadt1', Vect(-0.155,2.986,0.051), "1", StaticMesh'keypad_1digit');
	KeyButtons[1] = CreateButton(StaticMesh'keypadt2', Vect(-0.155,-0.175,0.051), "2", StaticMesh'keypad_2digit');
	KeyButtons[2] = CreateButton(StaticMesh'keypadt3', Vect(-0.155,-3.149,0.051), "3", StaticMesh'keypad_3digit');
	KeyButtons[3] = CreateButton(StaticMesh'keypadt4', Vect(-0.155,2.986,-1.37), "4", StaticMesh'keypad_4digit');
	KeyButtons[4] = CreateButton(StaticMesh'keypadt5', Vect(-0.155,-0.175,-1.37), "5", StaticMesh'keypad_5digit');
	KeyButtons[5] = CreateButton(StaticMesh'keypadt6', Vect(-0.155,-3.149,-1.37), "6", StaticMesh'keypad_6digit');
	KeyButtons[6] = CreateButton(StaticMesh'keypadt7', Vect(-0.155,2.986,-2.856), "7", StaticMesh'keypad_7digit');
	KeyButtons[7] = CreateButton(StaticMesh'keypadt8', Vect(-0.155,-0.175,-2.856), "8", StaticMesh'keypad_8digit');
	KeyButtons[8] = CreateButton(StaticMesh'keypadt9', Vect(-0.155,-3.149,-2.856), "9", StaticMesh'keypad_9digit');
	KeyButtons[9] = CreateButton(StaticMesh'keypadtStar', Vect(-0.155,2.986,-4.434), "*", StaticMesh'keypad_Sdigit');
	KeyButtons[10] = CreateButton(StaticMesh'keypadt0', Vect(-0.155,-0.175,-4.434), "0", StaticMesh'keypad_0digit');
	KeyButtons[11] = CreateButton(StaticMesh'keypadtNumber', Vect(-0.155,-3.149,-4.434), "#", StaticMesh'keypad_Ndigit');

	// Creating array of outputed mesh text
	CreateTextMesh();

	// Creating led access lights
	RedLight = spawn(class'EGameplayObject', self);
	RedLight.SetBase(self);
	RedLight.SetStaticMesh(StaticMesh'keypadRedLgt');
	RedLight.SetRelativeLocation(Vect(-0.242,-2.615,4.209));
	RedLight.bGlowdisplay = true;
	RedLight.bHidden = true;

	GreenLight = spawn(class'EGameplayObject', self);
	GreenLight.SetBase(self);
	GreenLight.SetStaticMesh(StaticMesh'keypadGreenLgt');
	GreenLight.SetRelativeLocation(Vect(-0.242,-4.183,4.209));
	GreenLight.bGlowdisplay = true;
	GreenLight.bHidden = true;

	// Creating Access messages
	dDenied = spawn(class'EGameplayObject', self);
	dDenied.SetBase(self);
	dDenied.SetStaticMesh(StaticMesh'keypad_denied');
	dDenied.SetRelativeLocation(CONST_TextStart);
	dDenied.Style = STY_Modulated;
	dDenied.bHidden = true;
	dDenied.bUnlit = true;

	dGranted = spawn(class'EGameplayObject', self);
	dGranted.SetBase(self);
	dGranted.SetStaticMesh(StaticMesh'keypad_granted');
	dGranted.SetRelativeLocation(CONST_TextStart);
	dGranted.Style = STY_Modulated;
	dGranted.bHidden = true;
	dGranted.bUnlit = true;

	SelectedButton = -1;
	Reset();

	Super.postBeginPlay();
}

function EKeyButton CreateButton( StaticMesh MeshName, Vector offset, string Val, optional StaticMesh TMesh )
{
	local EKeyButton B;
	B = spawn(class'EKeyButton', self);
	B.SetStaticMesh(MeshName);
	B.SetRelativeOffset(Offset);
	B.Value = Val;
	B.TextMesh = TMesh;
	return B;
}

function CreateTextMesh()
{
	local int i;
	local vector pos;

	for( i=0; i<ArrayCount(MeshCode); i++ )
	{
		pos = CONST_TextStart;
		pos.Y -= i * 0.6f;

		MeshCode[i] = spawn(class'EGameplayObject', self);
		MeshCode[i].SetCollision(false);
		MeshCode[i].SetBase(self);
		MeshCode[i].SetStaticMesh(None);
		MeshCode[i].SetRelativeLocation(pos);
		MeshCode[i].Style = STY_Modulated;
		MeshCode[i].bHidden = true;
		MeshCode[i].bUnlit = true;
		MeshCode[i].HeatIntensity = 1;
	}
}

//------------------------------------------------------------------------
// Description
//		Simple comparison on unique access code
//------------------------------------------------------------------------
function bool ProcessAccessCode()
{
	// If both code have same length, make validation
	if( Len(InputedCode) == Len(AccessCode) )
	{
		if( InputedCode == AccessCode )
			GotoState('s_AccessGranted');
		else
			GotoState('s_AccessDenied');

		return true;
	}
	return false;
}

//------------------------------------------------------------------------
// Description
//
//------------------------------------------------------------------------
function GlowSelected()
{
	local int i;

	for( i=0; i<ArrayCount(KeyButtons); i++ )
	{
		// set keypad setting
		KeyButtons[i].bRenderAtEndOfFrame = bRenderAtEndOfFrame;
		KeyButtons[i].bSpecialLit = bSpecialLit;
		KeyButtons[i].AmbientGlow = AmbientGlow;
	}

	if( SelectedButton != -1 )
		//return;
		KeyButtons[SelectedButton].AmbientGlow *= 2;

	for( i=0; i<ArrayCount(MeshCode); i++ )
		MeshCode[i].bRenderAtEndOfFrame = bRenderAtEndOfFrame;

	RedLight.bRenderAtEndOfFrame = bRenderAtEndOfFrame;
	GreenLight.bRenderAtEndOfFrame = bRenderAtEndOfFrame;
	dDenied.bRenderAtEndOfFrame = bRenderAtEndOfFrame;
	dGranted.bRenderAtEndOfFrame = bRenderAtEndOfFrame;
}

//------------------------------------------------------------------------
// Description
//		key was pushed, process it
//------------------------------------------------------------------------
function KeyPushed()
{
	GotoState('s_Use');
	KeyButtons[SelectedButton].Push(EKeyPadInteraction(Interaction).InteractionController.bIsPlayer);

	if( !EKeyPadInteraction(Interaction).InteractionController.bIsPlayer )
	{
		// Overwrites keyButton intensity code
		KeyButtons[SelectedButton].HeatIntensity = CodeIntensity[Len(InputedCode)];

		// Start thermal signature fadeout on first button
		if( Len(InputedCode) == 0 && !bNoTimer )
		{
			//Log(self$" BegIN FADE"@Interaction@EKeyPadInteraction(Interaction).InteractionController);
			FadeElapsedTime = 0;
			bButtonFaded = false;
			bRunTimer = true;
		}
	}

	if(EKeyPadInteraction(Interaction).InteractionController.bIsPlayer)
		PlaySound(Sound'Electronic.Play_keyPadButton', SLOT_Interface);
	else
		PlaySound(Sound'Electronic.Play_keyPadButton', SLOT_SFX);


	// RESET button
	if( SelectedButton == 9 )
	{
		Reset();
		return;
	}

	// Concatenate new value
	InputedCode = InputedCode $ KeyButtons[SelectedButton].Value;

	// Show letter mesh
	if( !ProcessAccessCode() )
		SetLetterMesh();
}

function SetLetterMesh()
{
	local int index;
	index = Len(InputedCode) - 1;
	MeshCode[index].SetStaticMesh(KeyButtons[SelectedButton].TextMesh);
	MeshCode[index].bHidden = false;
}
function ClearLetterMesh()
{
	local int i;
	for( i=0; i<ArrayCount(MeshCode); i++ )
		MeshCode[i].bHidden = true;
}

function Reset()
{
	InputedCode = "";
	ClearLetterMesh();
}

function Tick( float DeltaTime )
{
	local int i;

	if( !bRunTimer || bNoTimer )
		return;

	// Fade out timer
	FadeElapsedTime += DeltaTime;

	//Log(self$" tick"@FadeElapsedTime@FadeOutTimer);
	if( FadeElapsedTime < FadeOutTimer-5) // 5 = button fading time
		return;

	if( !bButtonFaded )
	{
		//Log(self$" Fading buttons");
		bButtonFaded = true;
		for( i=0; i<ArrayCount(KeyButtons); i++ )
			KeyButtons[i].bFading = true;
	}

	if( FadeElapsedTime < FadeOutTimer) // 5 = button fading time
		return;

	//Log(self$" Sending fadeout timer");
	if( GroupAI != None && FadeOutJumpLabel != '' )
		GroupAI.SendJumpEvent(FadeOutJumpLabel, false, false);
	bRunTimer = false;
}


auto state s_Idle
{
	function BeginState()
	{
		// No special display if not player
		if( Interaction != None &&
			EKeyPadInteraction(Interaction).InteractionController != None &&
			EKeyPadInteraction(Interaction).InteractionController.bIsPlayer )
		{
			bRenderAtEndOfFrame = false;
			bSpecialLit = false;
			AmbientGlow = 0;
		}
		GlowSelected();

		Reset();
	}
}

state s_Use
{
	function BeginState()
	{
		local EPlayerController Epc;
		Epc = EPlayerController(EKeyPadInteraction(Interaction).InteractionController);

		// No special display if not player
		if( Epc != None )
		{
			Epc.FakeMouseToggle(true);
			bRenderAtEndOfFrame = true;
			bSpecialLit = true;
			AmbientGlow = default.AmbientGlow;
		}
		SelectedButton = 4;
		GlowSelected();

		RedLight.bHidden = true;
		GreenLight.bHidden = true;
	}

	function EndState()
	{
		local EPlayerController Epc;
		Epc = EPlayerController(EKeyPadInteraction(Interaction).InteractionController);

		// No special display if not player
		if( Epc != None )
			Epc.FakeMouseToggle(false);
	}

	function Tick( float DeltaTime )
	{
		local int OldSelectedButton;
		local EPlayerController Epc;
		Epc = EPlayerController(EKeyPadInteraction(Interaction).InteractionController);

		OldSelectedButton = SelectedButton;

		if( Epc == None )
			return;

		//
		// Crappy button selection
		//
		if( CoordinateWithin(Epc, 96, 224, 65, 30) )
			SelectedButton = 0;
		else if( CoordinateWithin(Epc, 184, 224, 65, 30) )
			SelectedButton = 1;
		else if( CoordinateWithin(Epc, 266, 224, 65, 30) )
			SelectedButton = 2;
		else if( CoordinateWithin(Epc, 100, 262, 65, 30) )
			SelectedButton = 3;
		else if( CoordinateWithin(Epc, 187, 262, 65, 30) )
			SelectedButton = 4;
		else if( CoordinateWithin(Epc, 268, 262, 65, 30) )
			SelectedButton = 5;
		else if( CoordinateWithin(Epc, 104, 300, 65, 30) )
			SelectedButton = 6;
		else if( CoordinateWithin(Epc, 188, 300, 65, 30) )
			SelectedButton = 7;
		else if( CoordinateWithin(Epc, 269, 300, 65, 30) )
			SelectedButton = 8;
		else if( CoordinateWithin(Epc, 107, 340, 65, 30) )
			SelectedButton = 9;
		else if( CoordinateWithin(Epc, 192, 340, 65, 30) )
			SelectedButton = 10;
		else if( CoordinateWithin(Epc, 269, 340, 65, 30) )
			SelectedButton = 11;
		else
			SelectedButton = -1;

		//
		// Manage Mouse click
		//
		if( Epc.m_FakeMouseClicked )
		{
			if( SelectedButton != -1 )
				KeyPushed();
			else if( !CoordinateWithin(Epc, 80, 75, 277, 307) )
				Interaction.PostInteract(EKeyPadInteraction(Interaction).InteractionController);
		}
		Epc.m_FakeMouseClicked = false;

		// Change selection
		if( OldSelectedButton != SelectedButton )
			GlowSelected();
	}

	function bool CoordinateWithin( EPlayerController Epc, float x, float y, int w, int h )
	{
		return Epc.m_FakeMouseX > x && Epc.m_FakeMouseX < x + w &&
			   Epc.m_FakeMouseY > y && Epc.m_FakeMouseY < y + h;
	}
}

state s_Access
{
	function BeginState()
	{
		ClearLetterMesh();
		if( EKeyPadInteraction(Interaction).InteractionController.bIsPlayer )
			SetTimer(1.5f, false);
		else
			Timer();
	}

	function EndState()
	{
		Reset();
		Interaction.PostInteract(EKeyPadInteraction(Interaction).InteractionController);
	}

	function Timer()
	{
		GotoState('s_Idle');
	}
}

state s_AccessGranted extends s_Access
{
	Ignores KeyPushed;

	function BeginState()
	{
		if(EKeyPadInteraction(Interaction).InteractionController.bIsPlayer)
			PlaySound(Sound'Electronic.Play_KeypadOk', SLOT_Interface);
		else
			PlaySound(Sound'Electronic.Play_KeypadOk', SLOT_SFX);

		// If player enters code before button fading
		if( EKeyPadInteraction(Interaction).InteractionController.bIsPlayer )
		{
			bRunTimer = false;
			bNoTimer = true;
		}

		// Trigger pattern upon access granted
		TriggerPattern(EKeyPadInteraction(Interaction).InteractionController.pawn);

		dGranted.bHidden = false;
		GreenLight.bHidden = false;

		Super.BeginState();
	}

	function EndState()
	{
		local bool bShouldDestroy;
		bShouldDestroy = EKeyPadInteraction(Interaction).InteractionController.bIsPlayer;

		Super.EndState();
		dGranted.bHidden = true;

		// destroy keypad interaction once successfully used by player only
		if( bShouldDestroy )
			Interaction.SetCollision(false);
	}

	function Timer()
	{
		TriggerLinkedActors(EKeyPadInteraction(Interaction).InteractionController.Pawn);
		Super.Timer();
	}
}

state s_AccessDenied extends s_Access
{
	Ignores KeyPushed;

	function BeginState()
	{
		if(EKeyPadInteraction(Interaction).InteractionController.bIsPlayer)
			PlaySound(Sound'Electronic.Play_KeyPadNOk', SLOT_Interface);
		else
			PlaySound(Sound'Electronic.Play_KeyPadNOk', SLOT_SFX);

		dDenied.bHidden = false;
		RedLight.bHidden = false;

		Super.BeginState();
	}

	function EndState()
	{
		Super.EndState();
		dDenied.bHidden = true;
	}
}

defaultproperties
{
    AccessCode="12369#"
    CodeIntensity(0)=1.000000
    CodeIntensity(1)=1.000000
    CodeIntensity(2)=1.000000
    CodeIntensity(3)=1.000000
    CodeIntensity(4)=1.000000
    CodeIntensity(5)=1.000000
    FadeOutTimer=15.000000
    CONST_TextStart=(X=-0.400000,Y=1.000000,Z=2.785000)
    bIsKeypad=true
    AlarmLinkType=EAlarm_Trigger
    bAcceptsProjectors=false
    StaticMesh=StaticMesh'EMeshIngredient.KeyPadTech.keypadt'
    AmbientGlow=64
    CollisionRadius=4.000000
    CollisionHeight=4.000000
    HeatIntensity=0.800000
    InteractionClass=Class'EKeyPadInteraction'
}