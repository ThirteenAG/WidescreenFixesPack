//=============================================================================
// EPlayerController
//
// PlayerController specific for EchelonGameType
//=============================================================================
class EPlayerController extends PlayerController
	native;

#exec OBJ LOAD FILE=..\Sounds\FisherFoley.uax
#exec OBJ LOAD FILE=..\Sounds\FisherVoice.uax
#exec OBJ LOAD FILE=..\Sounds\FisherEquipement.uax
#exec OBJ LOAD FILE=..\Sounds\Gun.uax
#exec OBJ LOAD FILE=..\Sounds\Door.uax

var EPawn				ePawn;                          // reference to our ePawn - use this instead of Pawn
var EchelonGameInfo		eGame;							// reference to our GameInfo - use to store the gloabl variables and constantes
var bool				bInTransition;                  // am I in the middle of a non-interruptible anim?

var bool				bVisibilityFromCylinder;		// True if should use cylinder for Npc visibility detection. In stealth move for example

// SAVE GAME
var ESList              ProfileList;

var String              CurrentGoal;
var String              CurrentGoalSection;
var String              CurrentGoalKey;
var String              CurrentGoalPackage;
var bool                bNewGoal;
var bool                bNewNote;
var bool                bShouldTurnFast;
var bool                bNewRecon;

// ECHELON INVENTORY STUFF
var ESList				NoteBook;
var ESList				GoalList;
var ESList              ReconList;
var array<ETrainingData> TrainingList;
var EWeapon				MainGun,						// Main two handed weapon
						HandGun,						// Leg weapon
						ActiveGun;						// Currently active gun
var float				AccuracyModifier;
var EGoggle				Goggle;							// Sam's head gear
var bool				bInGunTransition;
var bool				bDrawWeapon;					// Since draw and sheath animation is the same.

var transient			EGameInteraction	egi;


// OUR INPUTS
var input byte			bResetCamera, bJump, bDPadUp, bDPadDown, bDPadLeft, bDPadRight;
var bool				bIncSpeedPressed;
var bool				bDecSpeedPressed;
var bool				bMustZoomIn;
var bool				bMustZoomOut;
var bool				bInteraction;

// STATE VARS
var ERopeActor			RapelRope;
var EPawn				m_AttackTarget;					// Target Actor for attack (short range/kick)
var Actor				m_targetObject;
var vector				m_WallHitNormal;
var vector				m_EnterDir;
var int					m_NbJump;
var int					RapelRopeType;
var float				prevWalkingRatio;
var bool				m_GrabTargeting;
var bool				m_BTWSide;
var bool				m_didCrossFence;
var bool				m_SMInTrans;
var	bool				m_holdingBreath;
var bool				m_rollScope;
var bool				m_rollGetUp;
var bool				m_rollCrouch;
var	bool				bThrowTargeting;
var	bool				bHideSam;
var	bool				bNoLedgePush;
var	bool				didGTicksPlayerInfo;
var	bool				didGTicksSavegame;
var	bool				didGTicksStartPattern;

var name				SpecialWaitAnim;
var name				JumpLabel;
var name				JumpLabelPrivate;


// DEBUGGING VARS
var bool				bDebugNavPoints;				// turned on by exec statement:
var bool				bDebugInput;                    // turned on by exec statement:  SHOWINPUT
var bool				bDebugStealth;					// turned on by exec statement:  STEALTH
var bool				bInvincible;					// turned on by exec statement:  INVINCIBLE
var bool				bFullAmmo;
var bool				bVideoMode;

// Timer tracking var
var float				m_LPStartTime;					// Timer stamp for pushing against ladders and pipes
var float				m_LastLedgeFenceTime;				// Timer stamp for letting go geo
var float				m_LastHOHTime;					//
var float				m_LastNLPipeTime;					//
var float				m_LastZipLineTime;				//
var float				m_LPSlideStartTime;		// Timer stamp for pull full back for slidding down pipes and ladders
var float				m_RunStartTime;
var float				m_SMTemp;
var float				m_HoistDeltaTime;
var float				m_HoistingDelta;
var name				m_HoistSt;
var name				m_HoistCr;
var vector				m_HoistStOffset;
var vector				m_HoistCrOffset;

var vector				m_ThrowSpeed;
var(Throw) config vector m_ThrowMinSpeed;
var(Throw) config vector m_ThrowMaxSpeed;
var(Throw) config float m_ThrowVarSpeed;

// Stuff for GEO
var bool				m_LedgeTurnRight;
var bool				m_GECanGoLeft;
var bool				m_GECanGoRight;
var bool				m_GECanGoForward;
var bool				m_GECanGoBackward;
var bool				m_GEgoingUp;
var bool				m_GECanSlide;
var bool				m_GECanGetOut;
var bool				m_GECanHoist;
var bool				m_LedgeElongated;
var bool				m_PipeInFeet;
var vector				m_FallGrabDir;
var float				m_Elongation;
var int					m_PType;

var vector				m_HoistAnimOffset;
var name				m_HoistingAnim;

// AI Special zones
var EVolume				CurrentVolume;
var EPattern			CurrentPattern;

// Sam speeds
var					int		m_curWalkSpeed;
var(Speed)	config	float	m_speedRun;
var(Speed)	config	float	m_speedRunCr;
var(Speed)	config	float	m_speedWalk;
var(Speed)	config	float	m_speedWalkCr;
var(Speed)	config	float   m_speedWalkFP;
var(Speed)	config	float	m_speedWalkFPCr;
var(Speed)	config	float	m_speedWalkSniping;
var(Speed)	config	float	m_speedBTWCr;
var(Speed)	config	float	m_speedBTW;
var(Speed)	config	float	m_speedFence;
var(Speed)	config	float	m_speedGrab;
var(Speed)	config	float	m_speedGrabFP;
var(Speed)	config	float	m_speedCarry;

var(Turning) float	m_turnMul;

var(Facing) float	m_towardAngle;

var float				m_FakeMouseX;
var float				m_FakeMouseY;
var bool				m_FakeMouseClicked;

// New Camera
var					EPlayerCam	m_camera;
var	bool						bLockedCamera;		// Used for Cinematic, air camera following, ...
var bool						m_useTarget;
var	float						m_lastRotationTick;
var	float						m_lastLocationTick;
var vector						m_targetLocation;
var Actor						m_targetActor;
var rotator                     hoistTargetRot;

// Used for Cinematic Camera //
var bool						bInCinematic;
var Rotator                     backupRotation;

struct native CameraInfo
{
	var Actor	Owner;
	var int		iPriority;
	var	float	FOV;
	var int		RendMap;
};

var array<CameraInfo> CamControllers;

// Rendering mode
var int	iRenderMask; // 1=SniperMode, 2=OpticCable, 3=LaserMic, 4=SmokeGrenade

// AI - Smell stuff //
var vector	SmellArray[20];
var int		CurrentSmellIndex;
var float	LastSmellUpdate;

var EDynamicNavPoint VisualSmell[20];
var vector LastVisibleLocation;

var ESurfaceType LastFenceType;

// Last 10 shots fired
struct HitInfo
{
    var vector Location;
    var float  Time;
};

var Array<HitInfo> LatestHits;


// Player Info //


var bool bShowController;

//BreathSounds variables
var float ElapsedTime;
var bool bIsPlaying;

var int iGameOverMsg;
var int iCheatMask;
var bool bQuickLoad;

const MAX_REGULAR_MAP = 13;

native(1290) final function SetCameraFOV( actor Owner, float NewFOV );
native(1291) final function SetCameraMode( actor Owner, int NewRendMap );
native(1292) final function PopCamera( actor Owner );

event EmptyRecons()
{
	NoteBook = None;
	GoalList = None;
	ReconList = None;
}

function bool IsInQuickInv()
{
	return (myHUD != None && myHUD.GetStateName() == 'QuickInventory');
}

event bool CanSaveGame()
{
	return (Pawn != None && Pawn.Health > 0 && !bStopInput && !Level.bIsStartMenu && !IsInQuickInv());
}

event bool CanLoadGame()
{
	return !IsInQuickInv();
}

event bool CanGoBackToGame()
{
	return (Pawn != None &&
			Pawn.Health > 0 &&
			!Level.bIsStartMenu &&
			myHUD != None &&
			!myHUD.IsPlayerGameOver());
}

function MouseMove(FLOAT MouseX, FLOAT MouseY)
{
	//Log("MouseMove X="$MouseX$" Y="$MouseY);
	m_FakeMouseX = MouseX;
	m_FakeMouseY = MouseY;
}

function MouseClick(FLOAT MouseX, FLOAT MouseY)
{
	//Log("MouseClick X="$MouseX$" Y="$MouseY);
	m_FakeMouseClicked = true;
	m_FakeMouseX = MouseX;
	m_FakeMouseY = MouseY;
}

function FakeMouseToggle( bool bStart )
{
    local ecanvas C;
    C = ECanvas(class'Actor'.static.GetCanvas());

	//Log("MouseToggle start="$bStart);

    if(bStart)
        C.Viewport.Console.ShowFakeWindow();
    else
        C.Viewport.Console.HideFakeWindow();

}

event bool GetPause()
{
	return Level.Game.GetPause();
}

function JumpRelease()
{
	local vector localSpeed;
	if(ePawn.FastPointCheck(ePawn.Location + vect(0,0,-10.0), ePawn.GetExtent(), true, true))
	{
		GoToState('s_PlayerJumping');
		localSpeed.X = -0.5;
		localSpeed.Y = aStrafe;
		SetPawnHoriVel(localSpeed, eGame.m_JumpOutSpeed);
		ePawn.m_topHanging = false;
		ePawn.m_topClimbing = false;
	}
	else
		GoToState(, 'OutBottom');
}

function float GetSniperPosVar()
{
	return 1.0;
}

native(1148) final function FlipCameraLocalTarget();

native(1177) final function MirrorCameraLocalTarget(bool right);

function NormalIKFade()
{
	ePawn.SetIKFade(4.0, 4.0);
}

function FastIKFade()
{
	ePawn.SetIKFade(8.0, 8.0);
}

event ReduceCameraSpeed(out float turnMul)
{
	turnMul = 1.0;
}

function ForceGunAway(optional bool forceAway)
{
	if(bInGunTransition || forceAway)
	{
		DoSheath();
		bInGunTransition = false;
	}
}

function bool CanResetCameraYaw()
{
	return VSize(ePawn.Velocity) == 0.0;
}

native(1149) final function KillPawnSpeed();

function UpdateCameraRotation(actor ViewActor)
{
    local vector		HitLocation, HitNormal, StartTrace, EndTrace;
	local rotator		ViewRotation;
	local int			PillTag;
	local ESniperGun	snipeGun;

	snipeGun = ESniperGun(ePawn.HandItem);
	if( snipeGun != None && snipeGun.bSniperMode && !bStopInput )
		ViewRotation = snipeGun.SniperNoisedRotation;
	else
		ViewRotation = Rotation;

	if( m_lastRotationTick != Level.TimeSeconds )
		m_camera.UpdateRotation(ViewActor);

	if( m_useTarget )
	{
		StartTrace	= GetFireStart();
		EndTrace	= StartTrace + 10000 * (Vect(1,0,0) >> ViewRotation);
		m_targetActor = ePawn.TraceBone(PillTag, HitLocation, HitNormal, EndTrace, StartTrace);

		if( m_targetActor == None )
			m_targetLocation = EndTrace;
		else
			m_targetLocation = HitLocation;
	}
	else
	{
		m_targetActor = None;
	}

	m_lastRotationTick = Level.TimeSeconds;
}

event AddMultiMapInformation()
{
	local EPattern startPattern;

	if(EchelonLevelInfo(Level).StartPatternClass != None)
	{
		startPattern = Spawn(EchelonLevelInfo(Level).StartPatternClass, Level);
		startPattern.InitPattern();
		startPattern.GotoPatternLabel('Begin');
	}
}

event StartInitialPattern()
{
	if(EchelonPlayerStart(StartSpot) != None)
		EchelonPlayerStart(StartSpot).SendInitialPattern();
}

event InitialSave()
{
	if( myHUD.GetStateName() != 's_QuickSaveMenu' )
	{
		EMainHUD(myHUD).SaveState();
		myHUD.GotoState('s_QuickSaveMenu');
	}
}

function PawnLookAt()
{
	local rotator lookAtRotPawn, lookAtRotEPC;

	lookAtRotPawn = ePawn.Rotation;
	lookAtRotPawn.Pitch = 0;
	lookAtRotEPC = Rotation;
	lookAtRotEPC.Pitch = 0;
	if(Vector(lookAtRotPawn) dot Vector(lookAtRotEPC) > -0.4)
	{
		if(ePawn.bIsCrouched)
			ePawn.LookAt(LANORMAL, Vector(Rotation), Vector(ePawn.Rotation), -45, 45, -45, 45);
		else
			ePawn.LookAt(LANORMAL, Vector(Rotation), Vector(ePawn.Rotation), -60, 60, -60, 60);
	}
}

native(1150) final function float ArrivalTimeApprox(float scale);

native(1151) final function bool IsPushing();
native(1152) final function bool IsPushingGentle();
native(1153) final function bool IsPushingFull();
native(1154) final function bool IsPushingToward(vector dir, float angle);
native(1155) final function bool IsPushingTowardFront(float angle);
native(1156) final function bool IsPushingTowardBack(float angle);
native(1157) final function float GetPushingForce();
native(1158) final function vector GetPushingDir();

native(1280) final function TCheck(int Entry, out int Value);

function PlayerMove(float DeltaTime)
{
}

function SetPawnRotation(float rotSpeed)
{
	local rotator newRot;
	local vector pushingDir;

	pushingDir = GetPushingDir();
	if(VSize(pushingDir) > 0.0)
	{
		newRot.Yaw = Rotation.Yaw + Rotator(pushingDir).Yaw;
		ePawn.RotateTowardsRotator(newRot, rotSpeed / 0.5, 0.5);
	}
}

function SetPawnAccel(vector X, float accel)
{
	local rotator targetRot;
	local vector NewAccel;

	if(IsPushing())
	{
		targetRot = Rotation + Rotator(GetPushingDir());
		targetRot.Pitch = 0;
		targetRot.Roll = 0;
		if(X dot Vector(targetRot) > 0.5)
			NewAccel = X;
	}
	NewAccel.Z          = 0;
	ePawn.Acceleration  = NewAccel;
	ePawn.Acceleration *= accel;
}

function SetPawnHoriVel(vector dir, float speed, optional bool add)
{
	dir.Z = 0.0;
	if(add)
		ePawn.Velocity += ePawn.ToWorldDir(Normal(dir)) * speed;
	else
		ePawn.Velocity = ePawn.ToWorldDir(Normal(dir)) * speed;
}

function HandleBreathSounds(float deltaTime)
{
	if ( VSize(ePawn.Velocity) != 0.0f )
		ElapsedTime += deltaTime;
	else
		ElapsedTime = 0.0f;

	if ( ElapsedTime > 5.0f )
	{
		if (!bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Play_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = true;
		}
	}
	else
	{
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
	}
}

//---------------------------------------[David Kalina - 17 Jul 2001]-----
//
// Description
//		Needed for MoveToDestination -
//		NOTE: If you change this,
//		change EAIController version to match.
//
//------------------------------------------------------------------------

function float ArrivalSpeedApprox(float scale)
{
	return FMax(eGame.m_minInterpolSpeed * scale, VSize(ePawn.Velocity));
}

event NoGE();
event bool FeetLedge(vector locationEnd, rotator orientationEnd);
event bool WaistLedge(vector locationEnd, rotator orientationEnd);
event bool Ledge(vector locationEnd, rotator orientationEnd, int LType);
event bool HandOverHand(vector locationEnd, rotator orientationEnd);
event bool NarrowLadder(vector locationEnd, rotator orientationEnd, int NLType);
event bool TopNarrowLadder(vector locationEnd, rotator orientationEnd);
event bool Pipe(vector locationEnd, rotator orientationEnd);
event bool TopPipe(vector locationEnd, rotator orientationEnd);
event bool ZipLine(vector locationEnd, rotator orientationEnd, int ZLType);
event bool FenceTop(vector locationEnd, rotator orientationEnd);
event bool Pole(vector locationEnd, rotator orientationEnd, int PLType);

//---------------------------------------[David Kalina - 18 Dec 2000]-----
//
// Description
//      Called immediately after play begins
//
//------------------------------------------------------------------------
function PostBeginPlay()
{
	//Init NoteBook
	NoteBook     = new class'ESList';
	GoalList     = new class'ESList';
	ReconList     = new class'ESList';
	ProfileList  = new class'ESList';

	iGameOverMsg = -1;

    Super.PostBeginPlay();

	m_camera = Spawn(class'EPlayerCam', self);
	m_camera.m_EPC = self;

	// Set current value to desired value from ini
	SetCameraFOV(self, DesiredFov);
}

event PlayerGiven()
{
	if( Player == None )
		Log(self$" Player shouldn't be None");

	egi	= EGameInteraction(Player.InteractionMaster.AddInteraction("Echelon.EGameInteraction", Player));
	egi.Setup(self);

	// Give HUD after all epc and pawn setup and before InitialPattern
	myHUD = spawn(class<HUD>(DynamicLoadObject("EchelonHUD.EchelonMainHUD", class'Class')),self);
}

event InitLoadGame()
{
	SetViewTarget(Pawn);

	Pawn.bStasis = false;
	PawnClass = Pawn.Class;
	egi.Setup(self);
}

//---------------------------------[Francois Schelling - 24 Mar 2002]-----
//
// Description: Add Training data to TrainingList
//
//------------------------------------------------------------------------
function AddTrainingData(EPattern TrainingPattern, String TrainingDesc, array<EchelonEnums.eKEY_BIND> TrainingKey, bool bHideController)
{
	local ETrainingData tData;
    local int i;

	// Set pattern for EGameMenuHud
	CurrentPattern = TrainingPattern;

	//Create the object
	tData = new class'ETrainingData';
	tData.Description = TrainingDesc;

    for(i = 0; i < TrainingKey.length; i++)
    {
        tData.ArrayKey[i] = TrainingKey[i];
    }

    tData.bShowController = (!bHideController);

    TrainingList.Length = TrainingList.Length+1;
    TrainingList[TrainingList.Length-1] = tData;

	if( MyHud.GetStateName() != 's_Training' )
	{
		EMainHUD(myHud).SaveState();
		myHud.GotoState('s_Training');
	}
}

//------------------------------------------------------------------------
// Description
//		Close and cleanup
//------------------------------------------------------------------------
event CloseTrainingData()
{
	TrainingList.Remove(0, TrainingList.Length);
	CurrentPattern = None;
	MyHUD.GotoState(EMainHUD(myHUD).RestoreState());
}

//---------------------------------------[Frederic Blais - 23 Apr 2001]-----
//
// Description: Add Note to NoteBook
//
//
//------------------------------------------------------------------------
event AddNote(string NoteString, string Section, string Key, string Package, bool insertAtEnd)
{
	local ENote Note;

	// Set flag to true to display icon in HUD
	bNewNote = true;

	//Create the object
	Note = new class'ENote';
	if(NoteString != "")
		Note.Note = NoteString;
	else if(Section != "" && Key != "" && Package != "")
		Note.Note = Localize(Section, Key, Package);
	else
		Note.Note = "killme";

	// For localization
	Note.Section = Section;
	Note.Key = Key;
	Note.Package = Package;

	// Always insert notes at begining of the list
	if ( NoteBook.FirstNode != None && !insertAtEnd)
		NoteBook.InsertBefore( Note, ENote(NoteBook.FirstNode.Data) );
	else
	NoteBook.InsertAtEnd(Note);
}

//---------------------------------------[Yanick Mimee - 26 Juin 2002]-----
//
// Description: Add Recon to Recon List
//
//
//------------------------------------------------------------------------
event AddRecon( class<ERecon> _ClassName, optional bool Completed )
{
	local ERecon Recon;

	//Create the object
	Recon = new _ClassName;
	if ( Recon != none)
	{
		// Set flag to true to display icon in HUD
		bNewRecon = true;

		Recon.InitRecon();
		Recon.bIsRead = Completed;
		ReconList.InsertAtEnd(Recon);
	}
}

//---------------------------------------[Yanick Mimee - 7 Aout 2002]-----
//
// Description: Add Recon of type text to Recon List
//
//
//------------------------------------------------------------------------
event AddReconText(	string ObjName,
					string MemoryTextSection,
					string From,
					string To,
					string SentStringID,
					string SubjectStringID,
					string MemoryTextID,
					bool bCompleted)
{
	local EReconDataStick DataStick;

	//Create the object
	DataStick = new class'EReconDataStick';
	if(DataStick != none)
	{
		// Set flag to true to display icon in HUD
		bNewRecon = true;
		DataStick.MakeText(ObjName, MemoryTextSection, From, To, SentStringID, SubjectStringID, MemoryTextID);
		DataStick.bIsRead = bCompleted;

		ReconList.InsertAtEnd(DataStick);
	}
}

//---------------------------------------[Frederic Blais - 23 Apr 2001]-----
//
// Description: Add Goal to the Goal List
//
//
//------------------------------------------------------------------------

event AddGoal(Name NameID, String GoalString, string GoalSection, string GoalKey, string GoalPackage, optional int iGoalPriority, optional string GoalStringShort, optional string ShortSection, optional string ShortKey, optional string ShortPackage, optional bool Completed)
{
	local ENote     Goal;
    local ENote     oTempGoal;
    local ESList    oOldList;
    local EListNode Node;
	local bool      bGotNewGoal;

	bGotNewGoal = false;

	// Create the object
	Goal = new class'ENote';

	if(GoalString != "")
	{
		Goal.Note = GoalString;
	}
	else if(GoalSection != "" && GoalKey != "" && GoalPackage != "")
	{
		Goal.Note = Localize(GoalSection, GoalKey, GoalPackage);
	}
	else
		Goal.Note = "killme";

	if(GoalStringShort != "")
		Goal.NoteShrink = GoalStringShort;
	else if(ShortSection != "" && ShortKey != "" && ShortPackage != "")
		Goal.NoteShrink = Localize(ShortSection, ShortKey, ShortPackage);
	else
		Goal.NoteShrink = "killme";

	Goal.ID = NameID;
	Goal.bCompleted = Completed;

	Goal.Section = GoalSection;
	Goal.Key = GoalKey;
	Goal.Package = GoalPackage;
	Goal.SectionShrink = ShortSection;
	Goal.KeyShrink = ShortKey;
	Goal.PackageShrink = ShortPackage;

        // Set flag to true to display icon in HUD
	bNewGoal = true;

    if (iGoalPriority != 0)
    {
        // If priority is entered, set it
        Goal.iPriority = iGoalPriority;
    }
    else
    {
        // Otherwise, set to default priority of 10
        Goal.iPriority = 10;
    }

    if (GoalStringShort == "")
    {
        GoalStringShort = "[GoalStringShort]";
    }

    if (GoalList.FirstNode != None)
    {
        // Insert node at beginning of nodes with our priority
        for(Node = GoalList.FirstNode; Node != None; Node = Node.NextNode)
        {
            oTempGoal = ENote(Node.Data);

            if(oTempGoal.iPriority >= Goal.iPriority)
            {
                GoalList.InsertBefore(Goal, oTempGoal);
                break;
            }

            // Were at end, gotta insert!
            if (Node.NextNode == None)
            {
                GoalList.InsertAtEnd(Goal);
                break;
            }
        }
    }
    else
    {
        GoalList.InsertAtEnd(Goal);
    }

	// Find the high priority goal
	Node = GoalList.FirstNode;
	while(Node != None)
	{
		Goal = ENote(Node.Data);
		if( !Goal.bCompleted )
		{
			bGotNewGoal = true;
			CurrentGoal = Goal.NoteShrink;
			CurrentGoal = Localize(Goal.SectionShrink, Goal.KeyShrink, Goal.PackageShrink);
			CurrentGoalSection = Goal.SectionShrink;
			CurrentGoalKey     = Goal.KeyShrink;
			CurrentGoalPackage = Goal.PackageShrink;

			break;
		}
		Node = Node.NextNode;
	}
	// If we didn`t find one, they`re all completed so display nothing
	if ( !bGotNewGoal ) CurrentGoal = "";
}

//---------------------------------[Francois Schelling - 4 June 2001]-----
//
// Description: Change status (completed or not) of Goal.
//
//
//------------------------------------------------------------------------
event SetGoalStatus(Name ID, bool status)
{
	local EListNode Node;
	local ENote		Note;
	local bool      bChangeGoal;
	local bool      bGotNewGoal;

	bChangeGoal = false;
	bGotNewGoal = false;
	Node = GoalList.FirstNode;

	while(Node != None)
	{
		Note = ENote(Node.Data);
		if(Note.ID == ID)
		{
			// We completed the current goal, we'll have to search for
			// a new one to display
			if ( Note.NoteShrink == CurrentGoal )
			{
				bChangeGoal = true;
			}
			Note.bCompleted = status;
			break;
		}
		Node = Node.NextNode;
	}

	// We need to display a new one, scan the list
	if ( bChangeGoal )
	{
		Node = GoalList.FirstNode;
		while(Node != None)
		{
			Note = ENote(Node.Data);
			if( !Note.bCompleted )
			{
				bNewGoal = true;
				bGotNewGoal = true;
				CurrentGoal = Note.NoteShrink;
				CurrentGoalSection = Note.SectionShrink;
				CurrentGoalKey     = Note.KeyShrink;
				CurrentGoalPackage = Note.PackageShrink;
				break;
			}
			Node = Node.NextNode;
		}
		// If we didn`t find one, they`re all completed so display nothing
		if ( !bGotNewGoal )
		{
			CurrentGoal = "";
			bNewGoal = false;
		}
	}
}

//----------------------------------------[David Kalina - 7 Mar 2001]-----
//
// Description
//      Important so we know when the Pawn variable is set
// Input
//		aPawn : Pawn we are possessing.
//
//------------------------------------------------------------------------
function Possess(Pawn aPawn)
{
	local EchelonPlayerStart EPlayerStart;
	local int i,j, Index;

	// Get the EPawn
	ePawn = EPawn(aPawn);       // set our ePawn reference
	eGame = EchelonGameInfo(Level.Game);
	eGame.pPlayer = self;

	LastVisibleLocation = ePawn.Location;

	if  (ePawn == none)
		Log("ECHELON--WARNING!!  Possessed Pawn NOT an ePawn!!");

	ePawn.Tag = 'ESam';			// set our pawn's tag

	// Copy _UNIQUE IN MAP_ EchelonPlayerStart Initial Inventory in the EPAWN
	EPlayerStart = EchelonPlayerStart(StartSpot);
	if(EPlayerStart != None)
	{
		for(i=0; i<EPlayerStart.DynInitialInventory.Length; i++ )
		{
			if( EPlayerStart.DynInitialInventory[i].ItemClass == None ||
				EPlayerStart.DynInitialInventory[i].Qty <= 0 )
				continue;

			// From the first (1) to Qty
			for( j=0; j<EPlayerStart.DynInitialInventory[i].Qty; j++ )
			{
				ePawn.DynInitialInventory[Index] = EPlayerStart.DynInitialInventory[i].ItemClass;
				Index++;
			}
		}
	}

	Super.Possess(aPawn);

	//
	// Init Inventory stuff
	//

	// Give Sam's Goggle
	Goggle = spawn(class'EGoggle', self);
	if(EPlayerStart != None)
		Goggle.bNoThermalAvailable = EPlayerStart.bNoThermalAvailable;

	// Set weapon
	if( HandGun != None )
		AttachObject(HandGun, HandGun.AttachAwayTag);

	// Try to give one of the guns
	SelectWeapon();

	if( MainGun != None )
		DoSheath();

	// Init animation
	ePawn.PlayAnim(ePawn.AWait);

	// Init PlayerInfo Stuff (TEMP for debuging) (Yanick Mimee)
/*	if ( playerInfo != None)
	{
		playerInfo.PlayerName = "";
		playerInfo.ControllerScheme = 1;
		playerInfo.Difficulty = 0;

		if (eGame != None)
		{
			if ( eGame.bDemoMode )
				playerInfo.GameSave[0] = "LAST CHECKPOINT";
			else
				playerInfo.GameSave[0] = "";
		}

		playerInfo.GameSave[1] = "";
		playerInfo.GameSave[2] = "";
		playerInfo.MapCompleted = 22;
		iCheatMask = 1;
	}*/

	m_camera.m_prevTargetZ = ePawn.Location.Z + ePAwn.CollisionHeight;
}

//----------------------------------------[Matthew Clarke - June 5th]-----
//
// Description
//  Adds an hit to the HitInfo array
//
//------------------------------------------------------------------------
function AddHit(vector HitLocation, float HitTime)
{
    local HitInfo HI;

    HI.Location = HitLocation;
    HI.Time = HitTime;

    if (LatestHits.Length == 10)
    {
        // Remove oldest hit
        LatestHits.Remove(0, 1);
    }



    LatestHits[LatestHits.Length] = HI;
}

function Destroyed()
{
	m_camera.Destroy();
	egi.Master.RemoveInteraction(egi);

	Super.Destroyed();
}

function PawnDied()
{
	if( ePawn != None )
	{
		SetLocation(ePawn.Location);
		ePawn.UnPossessed();
	}

	// Reset Interaction
	Interaction = None;
	GotoState('s_Dead'); // can respawn
}

function EnterStartState()
{
	GotoState('s_PlayerWalking');
}

//
//	DDDD    EEEEE   BBBB    UU  U    GGGG            AAA    N   N   DDDD             CCCC   HH  H   EEEEE    AAA    TTTTT
//	DD DD   EE      BB  B   UU  U   GG              AA  A   NN  N   DD DD           CC      HH  H   EE      AA  A    TT
//	DD  D   EEEE    BBBB    UU  U   GG GG           AAAAA   N N N   DD  D           CC      HHHHH   EEEE    AAAAA    TT
//	DD DD   EE      BB  B   UU  U   GG  G           AA  A   N  NN   DD DD           CC      HH  H   EE      AA  A    TT
//	DDDD    EEEEE   BBBB    UUUUU    GGGG           AA  A   N   N   DDDD             CCCC   HH  H   EEEEE   AA  A    TT
//
function ShowDebugInput( canvas Canvas )
{
    Canvas.DrawText
       ("AMOUSE X:" @ aMouseX @ "Y:" @ aMouseY @
        "AFORWARD :" @ aForward @
        "ASTRAFE : " @ aStrafe @
        "ATURN : " @ aTurn @
        "ALOOKUP: " @ aLookUp @
        "BPRESSEDJUMP : " @ bPressedJump @
        "bInterpolating : " @ bInterpolating @
        "ePawn.bInterpolating : " @ ePawn.bInterpolating);
}

exec function FreezePawns()
{
	local EPawn P;

	ForEach DynamicActors(class'EPawn', P)
	{
		if(!P.bIsPlayerPawn)
			P.bDisableAI = true;
	}
}

native(1165) final function ListItemsNative();
exec function ListItem()
{
	ListItemsNative();
}

//---------------------------------------[David Kalina - 28 Aug 2001]-----
// Description
//		Debug for making an NPC ViewTarget reverse his direction on a patrol network.
//------------------------------------------------------------------------
exec function Reverse()
{
	local EGoal goal;
	local EAIController AI;

	if ( ViewTarget != none && EPawn(ViewTarget) != none )
		AI = EAIController(EPawn(ViewTarget).Controller);
	else
		return;

	if ( AI != none )
	{
		goal = AI.m_pGoalList.GetCurrent();

		if ( goal != none && goal.m_GoalType == GOAL_Patrol )
		{
			if (AI.m_pNextPatrolPoint != none)
			{
				AI.SubGoal_PatrolTurnAround();
			}
		}
	}
}


//---------------------------------------[Cyrille Lauzon - 5 Nov 2002]-----
// Description
//		Increment the walking speed
//------------------------------------------------------------------------
exec function IncSpeed()
{
	bIncSpeedPressed = true;
	bMustZoomIn		 = true;

	m_curWalkSpeed += 1 ;
	if(m_curWalkSpeed > eGame.m_maxSpeedInterval)
	{
		m_curWalkSpeed = eGame.m_maxSpeedInterval;
	}
}

//---------------------------------------[Cyrille Lauzon - 5 Nov 2002]-----
// Description
//		Decrement the walking speed
//------------------------------------------------------------------------
exec function DecSpeed()
{
	bDecSpeedPressed = true;
	bMustZoomOut	 = true;

	m_curWalkSpeed -= 1;
	if(m_curWalkSpeed <= 0 )
	{
		m_curWalkSpeed = 1;
	}
}

//---------------------------------------[David Kalina - 12 Mar 2001]-----
// Description
//      Makes noise from the player's position
//      Used for debugging NPC response to unexpected sound
//------------------------------------------------------------------------
exec function Noise(float volume, optional NoiseType ntype, optional float zthreshold)
{
	if (ntype == 0)			ntype = NOISE_HeavyFootstep;
	if (zthreshold == 0.0f) zthreshold = EPawn.CollisionHeight * 2.0f;

    ePawn.MakeNoise(volume, ntype, zthreshold);
}

//----------------------------------------[David Kalina - 2 May 2001]-----
// Description
//		Exec statement that will toggle display of navigation points
//------------------------------------------------------------------------
exec function ShowNavPoints()
{
	local NavigationPoint nav;
	local EInfoPoint key;

	bDebugNavPoints = !bDebugNavPoints;

	for ( nav = Level.NavigationPointList; nav != None; nav = nav.NextNavigationPoint )
		nav.bHidden = bDebugNavPoints;

	for ( key = eGame.ELevel.InfoPointList; key != none; key = key.NextInfoPoint )
		key.bHidden = bDebugNavPoints;

}

exec function Invincible()
{
	bInvincible = !bInvincible;
	if ( bInvincible )
		Pawn.Style = STY_Translucent;
	else
		Pawn.style = STY_Normal;
}

exec function Health()
{
	if( ePawn.Health > 0 )
		ePawn.Health = ePawn.InitialHealth;
}

exec function StopTimer()
{
	if( EMainHUD(MyHud).TimerView != None )
		EMainHUD(MyHud).TimerView.Disable('Tick');
}

exec function Ammo()
{
	if( MainGun != None )
		MainGun.Ammo = MainGun.default.MaxAmmo;
	if( HandGun != None )
		HandGun.Ammo = HandGun.default.MaxAmmo;
}

exec function KillSam()
{
    ePawn.TakeDamage(2000, ePawn, vect(0,0,0), vect(0,0,0), vect(0,0,0), class'Crushed');
}

exec function KillTarget()
{
    m_AttackTarget.TakeDamage(2000, ePawn, vect(0,0,0), vect(0,0,0), vect(0,0,0), class'Crushed');
}

exec function KillAll(class<actor> aClass)
{
	local Actor A;

	if ( ClassIsChildOf(aClass, class'Pawn') )
	{
		KillAllPawns(class<Pawn>(aClass));
		return;
	}
	ForEach DynamicActors(class 'Actor', A)
		if ( ClassIsChildOf(A.class, aClass) )
			A.Destroy();
}

// Kill non-player pawns and their controllers
function KillAllPawns(class<Pawn> aClass)
{
	local Pawn P;

	ForEach DynamicActors(class'Pawn', P)
		if ( ClassIsChildOf(P.Class, aClass)
			&& !P.IsHumanControlled() )
		{
			//P.TakeDamage(0, None, Vect(0,0,0), Vect(0,0,0), Vect(0,0,0), class'EBurned');
			P.TakeDamage(P.Health, None, Vect(0,0,0), Vect(0,0,0), Vect(0,0,0), None);
			if ( P.Controller != None )
				P.Controller.Destroy();
			P.Destroy();
		}
}

exec function KillPawns()
{
	KillAllPawns(class'Pawn');
}

exec function Stealth()
{
	bDebugStealth = !bDebugStealth;
}
exec function ShowInput()
{
    bDebugInput = !bDebugInput;
}
exec function ToggleCamShot()
{
	bVideoMode = !bVideoMode;
}

exec function IM()
{
	local int i;

	Log("* IM ["$IManager.Interactions.Length$"]");
	for( i=0; i<IManager.Interactions.Length; i++ )
		Log("	"$IManager.Interactions[i].name@IManager.Interactions[i].iPriority);
}

exec function Mission( optional float i )
{
	EndMission(i==0, 4);
}

//
//	EEEEE   N   N   DDDD            DDDD     &&&     CCCC
//	EE      NN  N   DD DD           DD DD   && &&   CC
//	EEEE    N N N   DD  D           DD  D    &&&    CC
//	EE      N  NN   DD DD           DD DD   &  &&   CC
//	EEEEE   N   N   DDDD            DDDD     && &    CCCC
//

function NotifyFiring()
{
	local EWeapon w;
	w = EWeapon(ePawn.HandItem);
	if(w != None)
	{
		Level.RumbleVibrate(0.07, 0.75);
		if(w.IsA('ETwoHandedWeapon'))
			ePawn.Recoil(Vector(Rotation), w.RecoilStrength, w.RecoilAngle, w.RecoilStartAlpha, w.RecoilFadeIn, w.RecoilFadeOut);
	}
}

function NotifyStartCrouch();
function NotifyEndCrouch();

function vector GetFireStart()
{
	local vector offset;
	offset.X = m_camera.GetCurrentDistance();
	return ToWorld(offset);
}

//---------------------------------------[David Kalina - 18 Dec 2000]-----
//
// Description
//      Called in every frame so that the player can calculate the appropriate position
//      for the camera.
//
// Output
//          ViewActor :         The Camera's Target
//          CameraLocation :    Camera Location
//          CameraRotation :    Camera Orientation
//
//------------------------------------------------------------------------
event PlayerCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local ESniperGun		snipeGun;

	if ( (ViewTarget == None) || ViewTarget.bDeleteMe )
	{
		if ( (ePawn != None) && !ePawn.bDeleteMe )
			SetViewTarget(ePawn);
		else
			SetViewTarget(self);
	}

	ViewActor = ViewTarget;
	snipeGun = ESniperGun(ePawn.HandItem);

	if( m_lastLocationTick == Level.TimeSeconds || bLockedCamera )
	{
		CameraLocation = Location;
		CameraRotation = Rotation;
	}
	else
	{
		m_camera.CalcView(ViewTarget, CameraLocation, CameraRotation);
		m_lastLocationTick = Level.TimeSeconds;
	}

	if( snipeGun != None && snipeGun.bSniperMode && !bStopInput )
		CameraRotation = snipeGun.SniperNoisedRotation;
}

//---------------------------------------[ Alain Turcotte @ 25 avr. 2001 ]-
// Prototype		PlayerCalcEye
//
// Description		Calc view for all Type of goggles
//					Should be in EController coz EyeBoneName in EPawn
//------------------------------------------------------------------------
event PlayerCalcEye( out vector EyeLocation, out rotator EyeRotation )
{
	local vector X,Y,Z;
	EyeRotation = ePawn.GetBoneRotation(ePawn.EyeBoneName);
	GetAxes(EyeRotation, X,Y,Z);
	EyeRotation = Rotator(Y);
	EyeLocation = ePawn.GetBoneCoords(ePawn.EyeBoneName).Origin;
	EyeLocation += Y*ePawn.CollisionRadius/4;
}

function AdjustView(float DeltaTime );

// ***********************************************************************************************
// * Gun stuff
// ***********************************************************************************************

exec function Fire( optional float F )
{
	if( Level.Pauser != None || bStopInput )
		return;
	ProcessFire();
}

function ProcessFire()
{
	if(PlayerInput.bFireToDrawGun)
	{
// ATURCOTTE HACKY DRAW WEAPON
		if( bInGunTransition )
			return;
		// If nothing in hands but a gun is available, draw it on first fire click
		else if( ePawn.HandItem == None && ActiveGun != None )
			ProcessScope();
		else if( ePawn.HandItem == ActiveGun || !ePawn.HandItem.bIsProjectile )
			ePawn.HandItem.Use();
// ATURCOTTE HACKY DRAW WEAPON
		else
			DiscardPickup();
	}
	else
	{
		if( ePawn.HandItem == None || bInGunTransition )
			return;
		if( ePawn.HandItem == ActiveGun || !ePawn.HandItem.bIsProjectile )
			ePawn.HandItem.Use();
		else
			DiscardPickup();
	}
}

exec function AltFire( optional float F )
{
	if( Level.Pauser != None || bStopInput )
		return;
	ProcessAltFire();
}
function ProcessAltFire()
{
	if( ePawn.HandItem == None || bInGunTransition )
		return;
	if( ePawn.HandItem == ActiveGun )
		ActiveGun.AltFire();
}

exec function Scope()
{
	if( Level.Pauser != None || bStopInput )
		return;
	ProcessScope();
}
function ProcessScope();

function bool MayReload()
{
	return ActiveGun != None && ePawn.HandItem == ActiveGun && !bInGunTransition && !bLockedCamera;
}

exec function Interact()
{
	if( Level.Pauser != None || bStopInput )
		return;

	bInteraction = true;

	ProcessInteract();
}
function ProcessInteract();

//clauzon process the back to Wall alone
exec function BackToWall()
{
	if( Level.Pauser != None || bStopInput )
		return;

	ProcessBackToWall();
}

function ProcessBackToWall();

//clauzon process the Reload Gun alone
exec function ReloadGun()
{
	if( Level.Pauser != None || bStopInput )
		return;

	ProcessReloadGun();
}

function ProcessReloadGun()
{
	// Interact can call reload with a ActiveGun and no Interactions in the Mngr.
	if( MayReload() )
		ActiveGun.Reload();
}

function UsePalm()
{
	GotoState('s_UsingPalm');
}

function ReturnFromInteraction()
{
	GotoState('s_PlayerWalking');
}

function NotifyReloading()
{
	bInGunTransition = true;
	ePawn.PlayReload(true);
}

exec function SwitchROF()
{
	if( Level.Pauser != None || bStopInput )
		return;
	if( ePawn.HandItem != ActiveGun || bInGunTransition || bInTransition )
		return;
	if( ActiveGun != None )
	{
		if( ActiveGun.SwitchROF() )
			ePawn.PlaySound(Sound'FisherEquipement.Play_FisherSelectFireMode', SLOT_SFX);
	}
}
exec function SwitchHeadset( optional float i )
{
	if( Level.Pauser != None || bStopInput )
		return;
	ProcessHeadset(i);
}
function ProcessHeadSet( float i )
{
	// Quick switch goggle mode
	if( bInGunTransition )
		return;
	Goggle.SwitchMode(i);
}

exec function Jump( optional float F )
{
	if( Level.Pauser != None || bStopInput )
		return;
	bPressedJump = true;
}


function bool ToggleCinematic( EPattern CinematicPattern, bool bStart )
{
	// Make sure to never start a cinematic delayed
	if( iGameOverMsg != -1 )
		return false;

	//Log("ToggleCinematic"@bStart@bInCinematic);
	if( bStart && !bInCinematic )
	{
		EMainHUD(myHUD).SaveState();
		myHUD.GoToState('s_Cinematic');

		bInCinematic	= true;
		SetCameraMode(CinematicPattern, 5/*REN_DynLight*/);
		SetCameraFOV(CinematicPattern, DesiredFOV);
		backupRotation	= Rotation;

		CurrentPattern	= CinematicPattern;

		// Reset input (move to function if needed)
		ProcessToggleCinematic();

		m_camera.GoToState('s_Fixed');
	}
	else if( !bStart && bInCinematic )
	{
		bInCinematic	= false;
		PopCamera(CinematicPattern);
		SetRotation(backupRotation);

		CurrentPattern	= None;

		m_camera.GoToState('s_Following');
        myHUD.GotoState(EMainHUD(myHUD).RestoreState());
	}

	bStopInput = bInCinematic;
	return true;
}
function ProcessToggleCinematic()
{
	bFire = 0;
}

//------------------------------------------------------------------------
// Description
//		Called as soon as a mission is game over/failed. No state changes
//------------------------------------------------------------------------
function EndMission( bool bMissionComplete, int iFailureType )
{
	// Stop player input
	bStopInput = true;

	// Check if wouldn't be already in mission state
	// Or allow the same for timer
	if( iGameOverMsg != -1 && iGameOverMsg != iFailureType )
		return;

	// Extra stuff
	ProcessEndMission();

	// Set failure type
	iGameOverMsg = iFailureType;

	// Change hud state
	if( bMissionComplete )
		myHUD.GotoState('s_Mission', 'Complete');
	else
		myHUD.GoToState('s_Mission', 'Failed');
}
function ProcessEndMission();

//----------------------------------------[David Kalina - 9 Apr 2001]-----
//
// Description
//		Returns the position at which we are aiming.
//
//------------------------------------------------------------------------
function vector GetTargetPosition()
{
	return m_targetLocation;
}

//---------------------------------------[ Alain Turcotte @ 18 mai 2001 ]-
// Prototype		CheckForCrouch
//
// Description		Quick switch to Crouching / Uncrouching
//------------------------------------------------------------------------
function CheckForCrouch()
{
    if( bDuck > 0 )
    {
		ePawn.bWantsToCrouch = !ePawn.bWantsToCrouch;
		bDuck = 0;
    }
}

function bool SlowGunBack();

function bool CanInteract();
function bool CanAccessQuick();

exec function FullInv()
{
	EMainHUD(myHUD).FullInventory();
}

//------------------------------------------------------------------------
// Description
//		Catch all animEnd
//------------------------------------------------------------------------
event AnimEnd( int Channel )
{
	local float blendTime;
	//Log("AnimEnd"@Channel);

	// action channel ends
	if( Channel == EPawn.ACTIONCHANNEL )
	{
		if( !ePawn.IsAnimating(EPawn.ACTIONCHANNEL) )
		{
			//Log("AnimEnd Resetting Channel=3 alpha");
			if(SlowGunBack())
				blendTime = 0.25;
			else
				blendTime = 0.25;
			ePawn.AnimBlendToAlpha(EPawn.ACTIONCHANNEL,0,blendTime);
		}

		if( bInGunTransition )
		{
			//Log("AnimEnd EPawn.ACTIONCHANNEL .. not in scope transition anymore");
			bInGunTransition = false;
		}

		// If weapon in Reload state, will catch the AnimEnd
		if( ActiveGun != None && ePawn.HandItem == ActiveGun )
			ActiveGun.AnimEnd(69);
	}
	// Goggle stuff
	else if( Channel == EPawn.PERSONALITYCHANNEL )
	{
		//Log("* * * * AnimEnd on PERSONALITYCHANNEL");
		ePawn.AnimBlendToAlpha(EPawn.PERSONALITYCHANNEL,0,0.20);
	}
}

//------------------------------------------------------------------------
// Description
//		Called from animations to perform action synch with anims
//------------------------------------------------------------------------
event NotifyAction();

native(1162) final function CheckFallGrab();

//------------------------------------------------------------------------
// Description
//		Called just before a player falls from a high point
//------------------------------------------------------------------------
event MayFall()
{
	CheckFallGrab();
	GotoState('s_PlayerJumping');
}

//-------------------------------[Francois Schelling - 14 June 2001]-----
//
//	Call this function to Add Transmission to communication boxe
//-----------------------------------------------------------------------
function SendTransmissionMessage(string TextData, EchelonEnums.eTransmissionType eType)
{
	EMainHUD(myHUD).CommunicationBox.AddTransmission( self, eType, TextData, None);
}

//************************************************************************
//************************************************************************
//*********														   *******
//********					INVENTORY STUFF                         ******
//*********														   *******
//************************************************************************
//************************************************************************
function NotifyLostInventoryItem( EInventoryItem Item )
{
	if( Item == None )
        return;

	if( EWeapon(Item) == MainGun )
		MainGun = None;
	else if( EWeapon(Item) == HandGun )
		HandGun = None;

    if( EWeapon(Item) == ActiveGun )
        ActiveGun = None;
}

function bool CanSwitchToHandItem( EInventoryItem Item )
{
	return !bInGunTransition;
}

function bool CanSwitchGoggleManually();

function ChangeHeadObject( EInventoryItem HeadObj )
{
	//Log("* * * * CHangeHeadObject to="$HeadObj);
	Goggle.SwitchVisionMode(EAbstractGoggle(HeadObj).RendType);
}
event GoggleUp() // High
{
	Goggle.Up();
}
event GoggleDown() // Low
{
	Goggle.Down();
}

function DiscardPickup( optional bool bSheathItem, optional bool bNoVelocity )
{
	local vector DropVel;

	//Log("* * DiscardPickup HAND="$ePawn.HandItem$" Gun="$ActiveGun$" SEL="$ePawn.FullInventory.GetSelectedItem()@bSheathItem);
	if( ePawn.HandItem == None )
		return;

	DropVel = ePawn.Velocity;
	if( !bNoVelocity )
		DropVel += ePawn.ToWorldDir(Vect(50,50,0));

	// If it's a simple g.o. so item can not be added to inventory
	if( !ePawn.HandItem.IsA('EInventoryItem') )
	{
		ePawn.SetBoneRotation('B R Hand',Rot(-2000,-8000,0),,10,3,1);
		ePawn.HandItem.Throw(self, DropVel);
	}
	else if( bSheathItem )
		SelectWeapon(,bSheathItem);
}

//------------------------------------------------------------------------
// Description
//		Fall back on available weapon
//------------------------------------------------------------------------
function SelectWeapon( optional bool bForceHandGun, optional bool bChangeToNone )
{
	//Log("SelectWeapon ActiveGun["$ActiveGun$"] HandItem["$ePawn.HandItem$"]"@bChangeToNone);

	// Force HandGun
	if( bForceHandGun && HandGun != None )
		ePawn.FullInventory.SetSelectedItem(HandGun);

	// Fallback to currently active gun
	else if( ActiveGun != None )
		ePawn.FullInventory.SetSelectedItem(ActiveGun);

	// Fallback to any available weapon
	else if( MainGun != None )
		ePawn.FullInventory.SetSelectedItem(MainGun);
	else if( HandGun != None )
		ePawn.FullInventory.SetSelectedItem(HandGun);
	else if( bChangeToNone )
		ePawn.FullInventory.UnEquipItem(ePawn.FullInventory.GetSelectedItem());
}

//------------------------------------------------------------------------
// Description
//		Do the actual handItem change, for egameplayobject
//------------------------------------------------------------------------
function ChangeHandObject( EGameplayObject Obj, optional bool bSheathItem )
{
	//Log("* * * * CHangeHandObject to="$Obj$" HAND="$ePawn.HandItem$" SEL="$ePawn.FullInventory.GetSelectedItem());

	// Do nothing for maingun and handgun
	if( Obj != MainGun && Obj != HandGun )
	{
		// if ePawn.HandItem != None .. maybe should drop it
		DiscardPickup(bSheathItem);

		ePawn.HandItem = Obj;
		if( Obj != None )
			AttachObject(Obj, Obj.GetHandAttachTag());
	}
	// Only nonify inventory item if selecting a weapon
	else if( ePawn.HandItem != None && ePawn.HandItem.IsA('EInventoryItem') )
		ePawn.HandItem = None;

	// Set ActiveGun for any weapon coming in hands
	if( EWeapon(Obj) != None )
		ActiveGun = EWeapon(Obj);

	CheckHandObject();

	// Auto Switch to maingun
	if( Obj == None )
		SelectWeapon();
}

function CheckHandObject()
{
	if( EWeapon(ePawn.HandItem) != None )
	{
		// test case one or two handed weapon
		if( ePawn.HandItem.IsA('ETwoHandedWeapon') )
			ePawn.WeaponStance = 2;
		else
			ePawn.WeaponStance = 1;
	}
	else
		ePawn.WeaponStance = 0;

	//Log("@ calling switchanims"@ePawn.WeaponStance@epawn.handitem@activegun);

	// call to set animations for each different character
	ePawn.SwitchAnims();
}

//------------------------------------------------------------------------
//		Attach an object to a bone name
//------------------------------------------------------------------------
event AttachObject( EGameplayObject Obj, name AttachTag )
{
	if( Obj == None )
		return;

	//Log("Attaching "$Obj$" to bone "$AttachTag$" of "$ePawn);
	ePawn.AttachToBone(Obj, AttachTag);
	Obj.SetRelativeLocation(Vect(0,0,0));
	Obj.SetRelativeRotation(Rot(0,0,0));
}

//------------------------------------------------------------------------
// Description
//		For sheath/draw animation, play returned anim inverted
//------------------------------------------------------------------------
function name GetWeaponAnimation()
{
	if( /*ActiveGun == None ||*/ ActiveGun.IsA('EOneHandedWeapon') )
	{
		if( !ePawn.bIsCrouched )
			return 'DrawStSpBg1';
		else
			return 'DrawCrSpBg1';
	}
	else
	{
		if( !ePawn.bIsCrouched )
			return 'DrawStSpBg2';
		else
			return 'DrawCrSpBg2';
	}
}

//------------------------------------------------------------------------
//		Restore back weapon in pawn's hands
//------------------------------------------------------------------------
function DrawWeapon( optional Name AnimName, optional bool bFullSkel, optional bool bForceHandGun )
{
	local name BoneStart;
	//Log("* * * DrawWeapon "$ePawn.HandItem@ActiveGun);

	// Select a weapon to draw
	DiscardPickup();
	SelectWeapon(bForceHandGun);

	bDrawWeapon = true;

	// Give default anim
	if( AnimName == '' )
		AnimName = GetWeaponAnimation();

	//Log("bInGunTransition = true;");
	bInGunTransition = true;

	if ( ActiveGun == MainGun )
	{
		if(GetStateName() == 's_RappellingTargeting')
			ePawn.AddSoundRequest(Sound'FisherEquipement.Play_FN2000Aim', SLOT_SFX, 1.0f);
		else
			ePawn.PlaySound(Sound'FisherEquipement.Play_FN2000Aim', SLOT_SFX);
	}
	else
		ePawn.PlaySound(Sound'FisherEquipement.Play_FisherPistolAim', SLOT_SFX);


	if( bFullSkel )
		BoneStart = '';
	else
		BoneStart = ePawn.UpperBodyBoneName;
	ePawn.BlendAnimOverCurrent(AnimName, 1, BoneStart);
}

//------------------------------------------------------------------------
//		Send in hand weapon in pawn's back
//------------------------------------------------------------------------
function SheathWeapon( optional name AnimName, optional name BoneStart, optional bool bFullSkel )
{
	//Log("* * * SheathWeapon handitem["$ePawn.HandItem$"] activeGun["$ActiveGun$"] in state["$GetStateName()$"]");

	bDrawWeapon = false;

	if( ActiveGun == None )
		Log("Trying to Sheath no active gun ..."@ePawn.HandItem@ActiveGun);

	else if( ActiveGun != ePawn.FullInventory.GetSelectedItem() )
	{
		Log("ERROR: SheathWeapon called when ActiveGun not selected"@ActiveGun@ePawn.FullInventory.GetSelectedItem());
		//return; // so that we may call Sheath on a picked up main/hand gun
	}

	// Give default anim
	if( AnimName == '' )
		AnimName = GetWeaponAnimation();

	bInGunTransition = true;

	if ( ActiveGun == MainGun )
		ePawn.PlaySound(Sound'FisherEquipement.Play_FN2000AimBack', SLOT_SFX);
	else
		ePawn.PlaySound(Sound'FisherEquipement.Play_FisherPistolAimBack', SLOT_SFX);

	if(bFullSkel)
		BoneStart = '';
	else if( BoneStart == '' || ePawn.bIsCrouched )
		BoneStart = ePawn.UpperBodyBoneName;
	ePawn.BlendAnimOverCurrent(AnimName, 1, BoneStart,,,,true);
}

// called from anim notify
function NotifyWeapon()
{
	//Log("NotifyWeapon"@bDrawWeapon);
	if( bDrawWeapon )
		DoDraw();
	else
		DoSheath();
}

function DoSheath()
{
	//Log("DoSheath"@ePawn.HandItem@ActiveGun);
	// Grab release will call this .. do nothing if no weapon in hands
	if( ActiveGun == None )
		return;

	ActiveGun.InTargetingMode = false;

	if( ActiveGun == MainGun || ActiveGun == HandGun )
		ePawn.HandItem = None;

	AttachObject(ActiveGun, ActiveGun.AttachAwayTag);
	CheckHandObject();

	// Make sure gun is not in reload state here
	ActiveGun.EndEvent();
}

function DoDraw()
{
	//Log("DoDraw"@ePawn.HandItem@ActiveGun);
	// Grab start will call this .. do nothing if no weapon in hands
	if( ActiveGun == None )
		return;

	ActiveGun.InTargetingMode = true;

	ePawn.HandItem = ActiveGun;

	AttachObject(ActiveGun, 'WeaponBone');
	CheckHandObject();

	// Change camera?
	SwitchCameraMode();
}

function SwitchCameraMode();

function Reload1()
{
	ActiveGun.StepReload(1);
}
function Reload2()
{
	ActiveGun.StepReload(2);
}
function Reload3()
{
	ActiveGun.StepReload(3);
}
function Reload4()
{
	ActiveGun.StepReload(4);
}

// Toggle to prevent paying extra anim
function Trigger( actor Other, pawn EventInstigator, optional name InTag )
{
	SpecialWaitAnim = '';
}

//-----
// END
//-----

native(1163) final function bool TryHoisting(vector locationEnd, rotator orientationEnd, vector offsetStanding, vector offsetCrouch, name animStanding, name animCrouch);
native(1166) final function bool CheckBTW(optional bool reverse, optional bool test);
native(1178) final function bool ShouldReleaseNPC();

//************************************************************************
//************************************************************************
//*********														   *******
//********					      STATES                            ******
//*********														   *******
//************************************************************************
//************************************************************************

// ----------------------------------------------------------------------
// state s_Grab
// ----------------------------------------------------------------------
state s_Grab
{
	Ignores MayReload;

    function BeginState()
    {
		local vector EndPos, offset;
		if(!m_GrabTargeting)
		{
			m_AttackTarget.Controller.GoTostate('s_Grabbed');
			bInTransition = true;

			ePawn.UnCrouch(false);
			ePawn.bWantsToCrouch = false;

			offset = Normal(ePawn.Location - m_AttackTarget.Location);
			EndPos = m_AttackTarget.Location + offset * (m_AttackTarget.CollisionRadius + ePawn.CollisionRadius + 2.0);
			offset.Z = 0;

			ePawn.m_orientationStart	= ePawn.Rotation;
			ePawn.m_orientationEnd		= Rotator(-offset);
			ePawn.m_locationStart		= ePawn.Location;
			ePawn.m_locationEnd			= EndPos;

			m_AttackTarget.RandomizedAnimRate = 1.0;
		}

		ePawn.AddSoundRequest(Sound'FisherFoley.Play_FisherGrabNpc', SLOT_SFX, 0.1f);

		m_camera.SetMode(ECM_Grab);
		m_GrabTargeting = false;
		m_SMInTrans = false;
    }

    function EndState()
	{
		bNoLedgePush = false;
		if(!m_GrabTargeting)
		{
			if( ePawn.Health <= 0 )
			{
				KillPawnSpeed();
				m_AttackTarget.Controller.GotoState(,'FinishAlone');
			}
			else if( m_AttackTarget != None )
			{
				m_AttackTarget.RandomizedAnimRate = 0.97f + RandRange(0.0f, 0.06f);
				m_AttackTarget = None;
			}
		}
		m_SMInTrans = false;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		local bool bTalking;
		if( m_AttackTarget != None && m_AttackTarget.Interaction != None )
			bTalking = m_AttackTarget.Interaction.GetStateName() == 's_NpcTalking';

		// Case 1 : When not conversating, any of these interactions are available
		// Case 2 : Target interaction is always valid (talking or not talking)
		if( (!bTalking &&
			 (IntObj.class.name == 'ERetinalScannerInteraction'	 ||
			IntObj.class.name == 'ERetinalSafeInteraction'		||
			IntObj.class.name == 'EForceRetinalSafeInteraction'	||
			  IntObj.class.name == 'EForceComputerInteraction'))	||
			(IntObj.Owner == m_AttackTarget))
			return !bInTransition && ePawn.Physics != PHYS_Falling;
	}

	function bool CanInteract()
	{
		return ePawn.Physics != PHYS_Falling;
	}

	function SetPawnRotation(float rotSpeed)
	{
		local rotator newRot;
		local vector pushingDir;

		pushingDir = GetPushingDir();
		if(VSize(pushingDir) > 0.0)
		{
			newRot.Yaw = 32768 + Rotation.Yaw + Rotator(pushingDir).Yaw;
			ePawn.RotateTowardsRotator(newRot, rotSpeed / 0.5, 0.5);
		}
	}

	function ProcessFire()
	{
		if( !bInTransition && ePawn.Physics != PHYS_Falling)
			GoToState(, 'ReleaseKnock');
	}

	function ProcessScope()
	{
		if( bInTransition || HandGun == None || ePawn.Physics == PHYS_Falling)
			return;

		m_GrabTargeting = true;
		GotoState('s_GrabTargeting');
	}

	event MayFall()
	{
		if( !bInTransition && ShouldReleaseNPC())
		{
			CheckFallGrab();
			GotoState(, 'Falling');
		}
		// else Sam and NPC will fall together
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		return true;
	}

    function PlayerMove( float DeltaTime )
    {
        local vector X,Y,Z, NewAccel, targetDir;
		local float dist;

		if(m_SMInTrans)
		{
			m_AttackTarget.SetPhysics(PHYS_None);
			m_AttackTarget.Velocity = vect(0,0,0);
			m_AttackTarget.Acceleration = vect(0,0,0);
			m_AttackTarget.RotateTowardsRotator(ePawn.m_orientationEnd, 70000);
			targetDir.Z = ePawn.Location.Z - m_AttackTarget.Location.Z;
			dist = FMin(Abs(targetDir.Z), 90.0f * DeltaTime);
			if(dist > 0.001)
			{
				m_AttackTarget.Move(Normal(targetDir) * dist);
			}
		}

		// if Npc is on fire
		if( m_AttackTarget != None && m_AttackTarget.Base == ePawn && m_AttackTarget.BodyFlames.Length > 0 )
		{
			m_AttackTarget.AttenuateFire(0.8);
			ePawn.TakeDamage(0, None, Vect(0,0,0), Vect(0,0,0), Vect(0,0,0), class'EBurned');
		}

		// If npc dies while being grabbed
		if( m_AttackTarget != None && m_AttackTarget.GetStateName() != 's_Grabbed' )
		{
			GoToState(, 'ReleaseDead');
			bInTransition = true;
			m_AttackTarget = None;
			return;
		}

		if(ePawn.Physics == PHYS_Falling)
			return ;

		if(bInTransition)
			return;

		PawnLookAt();

        GetAxes(ePawn.Rotation,X,Y,Z);

		// Update acceleration.
		SetPawnAccel(-X, eGame.m_onGroundAccel);
		SetPawnRotation(40000);

		if(IsPushing())
			ePawn.GroundSpeed = m_speedGrab;
		else
			ePawn.GroundSpeed = 0.0;

		if(VSize(ePawn.Velocity) > 0)
		{
			ePawn.LoopAnim(ePawn.ABlendGrab.m_backward,,0.05);
			m_AttackTarget.LoopAnimOnly(m_AttackTarget.ABlendGrab.m_backward,,0.05);
		}
		else
		{
			ePawn.LoopAnimOnly(ePawn.AGrabWait,,0.2);
			m_AttackTarget.LoopAnimOnly(m_AttackTarget.AGrabWait,,0.2);
		}
	}

	function Trigger( actor Other, pawn EventInstigator, optional name InTag )
	{
		//Global.Trigger(Other, EventInstigator);
		if( Interaction == None )
			Log(self$" Interaction shouldn't be None in s_grab computer");
		Interaction.PostInteract(self);
	}

	event NotifyAction()
	{
		if(m_AttackTarget != None)
			m_AttackTarget.TakeDamage(m_AttackTarget.health / 2.0, ePawn, m_AttackTarget.Location, ePawn.Velocity, ePawn.Velocity, class'EKnocked', EPawn.P_Head);
	}

Falling:
	KillPawnSpeed();
	DoSheath();
	m_AttackTarget.Controller.GoTostate(,'Fall');
	GoTostate('s_PlayerJumping');

Begin:
	bNoLedgePush = true;
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Linear);
	m_SMInTrans = true;
	MoveToDestination(200.0, true);
	ePawn.SetRotation(ePawn.m_orientationEnd);
	m_AttackTarget.SetRotation(ePawn.m_orientationEnd);
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Walking);
	DiscardPickup();
	SelectWeapon(true);
	if( HandGun != None )
		ePawn.WeaponStance = 1;
	ePawn.SwitchAnims();
	bDrawWeapon = true;
	m_AttackTarget.PlayAnimOnly(m_AttackTarget.AGrabStart,,0.1);
	ePawn.PlayAnimOnly(ePawn.AGrabStart,,0.1);
	// Force uncrouch without collision
	m_AttackTarget.SetCollisionSize(m_AttackTarget.default.CollisionRadius, m_AttackTarget.default.CollisionHeight);
	m_AttackTarget.bWantsToCrouch = false;
	m_AttackTarget.PrePivot = vect(0,0,0);
	FinishAnim();
	m_AttackTarget.FinishGrabbing(ePawn);
	m_SMInTrans = false;
	bInTransition = false;
	bNoLedgePush = false;
	Stop;

ForceInterrogate:
	// Don't shake Npc if already shaking him
	if( ePawn.IsAnimating(EPawn.ACTIONCHANNEL) )
		Stop;
	ePawn.BlendAnimOverCurrent(ePawn.AGrabSqeeze,1,ePawn.UpperBodyBoneName,,0.05);
	m_AttackTarget.BlendAnimOverCurrent(m_AttackTarget.AGrabSqeeze,1,ePawn.UpperBodyBoneName,,0.05);
	Stop;

TurnRelease:
	bInTransition = true;
	KillPawnSpeed();
	m_camera.SetMode(ECM_Walking);
	// Animate pawns towards InteractLocation
	ePawn.LoopAnimOnly(ePawn.ABlendGrab.m_backward,,,true);
	m_AttackTarget.LoopAnimOnly(m_AttackTarget.ABlendGrab.m_forward,,, true);

	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);

	ePawn.SetPhysics(PHYS_Walking);
	JumpLabel = '';
	bInTransition = true;
	KillPawnSpeed();
	bDrawWeapon = false;
	ePawn.PlayAnimOnly(ePawn.AGrabRelease);

	// Remove interaction on Nicoladze, since no more grab or interaction/conversation to do on him
	m_AttackTarget.ResetInteraction();
	m_AttackTarget.Controller.GoTostate('s_Grabbed','Release');
	FinishAnim();

	// Force wait animation
	SpecialWaitAnim = 'HoldStAlNt0';

	bInTransition = false;
	GoToState('s_PlayerWalking');

ReleaseDead:
	bNoLedgePush = true;
	KillPawnSpeed();
	ePawn.WeaponStance = 0;
	ePawn.SwitchAnims();
	ePawn.PlayWait(0, 0.3);
	Sleep(0.3);
	ForceGunAway(true);
	bInTransition = false;
	bNoLedgePush = false;
	GoToState('s_PlayerWalking');

ReleaseKnock:
	bInTransition = true;
	bNoLedgePush = true;
	KillPawnSpeed();
	ePawn.AddSoundRequest(Sound'FisherFoley.Play_FisherKnockOut', SLOT_SFX, 0.5f);
	bDrawWeapon = false;
	ePawn.PlayAnimOnly(ePawn.AGrabReleaseKnock);
	m_AttackTarget.PlayAnimOnly(m_AttackTarget.AGrabReleaseKnock);
	FinishAnim();
	bInTransition = false;
	bNoLedgePush = false;
	GoToState('s_PlayerWalking');

Computer:
	bInTransition = true;
	KillPawnSpeed();
	m_camera.SetMode(ECM_Walking);

	// Animate pawns towards InteractLocation
	ePawn.LoopAnimOnly(ePawn.ABlendGrab.m_backward,,0.05, true);
	m_AttackTarget.LoopAnimOnly(m_AttackTarget.ABlendGrab.m_backward,,0.05, true);

	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);

	ePawn.SetPhysics(PHYS_Walking);

	ePawn.PlayAnimOnly(ePawn.AGrabRetinalStart, , 0.1);
	m_AttackTarget.PlayAnimOnly('GrabStPcBg0', , 0.1);
	FinishAnim();

	Interaction.Interact(m_AttackTarget.Controller);

	ePawn.LoopAnimOnly(ePawn.AGrabRetinalWait, , 0.1);
	m_AttackTarget.LoopAnimOnly('GrabStPcNt0', , 0.1);
	Stop;

ComputerEnd:
	ePawn.PlayAnimOnly(ePawn.AGrabRetinalEnd, , 0.1);
	m_AttackTarget.PlayAnimOnly('GrabStPcEd0', , 0.1);
	FinishAnim();

	bInTransition = false;
	m_camera.SetMode(ECM_Grab);
	Stop;

RetinalScan:
	m_camera.SetMode(ECM_HSphere);

	bInTransition = true;
	KillPawnSpeed();

	// Animate pawns towards InteractLocation
	ePawn.LoopAnimOnly(ePawn.ABlendGrab.m_backward,,0.05, true);
	m_AttackTarget.LoopAnimOnly(m_AttackTarget.ABlendGrab.m_backward,,0.05, true);

	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);

	ePawn.SetPhysics(PHYS_Walking);

	ePawn.PlayAnimOnly(ePawn.AGrabRetinalStart, , 0.1);
	m_AttackTarget.PlayAnimOnly(m_AttackTarget.AGrabRetinalStart, , 0.1);
	FinishAnim();

	// Interact after grab start animation
	Interaction.Interact(m_AttackTarget.Controller);

	ePawn.LoopAnimOnly(ePawn.AGrabRetinalWait, , 0.1);
	m_AttackTarget.LoopAnimOnly(m_AttackTarget.AGrabRetinalWait, , 0.1);
	Stop;

RetinalEnd:
	ePawn.PlayAnimOnly(ePawn.AGrabRetinalEnd, , 0.1);
	m_AttackTarget.PlayAnimOnly(m_AttackTarget.AGrabRetinalEnd, , 0.1);
	FinishAnim();

	bInTransition = false;
	m_camera.SetMode(ECM_Grab);

	if( JumpLabel != '' )
		Goto(JumpLabel);
	Stop;

FromFP:
}

state s_GrabTargeting
{
	Ignores MayReload;

	function BeginState()
	{
		m_useTarget = true;
		bInGunTransition = true;
		ePawn.WalkingRatio = 0;
		ePawn.SoundWalkingRatio = 0;
		ePawn.Acceleration = vect(0,0,0);
		m_camera.SetMode(ECM_GrabFP);

		// Prevents looping hold animation
		ePawn.AnimBlendToAlpha(ePawn.REFPOSECHANNEL,0,0.10);
	}

	function EndState()
	{
		m_useTarget = false;
		EMainHUD(myHud).NormalView();
		ePawn.AnimBlendToAlpha(ePawn.BLENDMOVEMENTCHANNEL,0,0.10);

		// If Sam dies, release Npc
		if( ePawn.Health <= 0 )
		{
			KillPawnSpeed();
			m_AttackTarget.Controller.GotoState(,'FinishAlone');
		}
	}

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = m_turnMul;
	}

	function ProcessScope()
	{
		if(ePawn.Physics != PHYS_Falling)
			GoToState( ,'UnAim');
	}

	function ProcessFire()
	{
		if(ePawn.Physics != PHYS_Falling)
			Global.ProcessFire();
	}

	event MayFall()
	{
		if( !bInTransition && ShouldReleaseNPC())
		{
			CheckFallGrab();
			GotoState('s_Grab', 'Falling');
		}
		// else Sam and NPC will fall together
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		return true;
	}

    function PlayerMove( float DeltaTime )
    {
        local vector X,Y,Z, NewAccel;
		local float pushingForce;

		pushingForce = GetPushingForce();

		// if Npc is on fire
		if( m_AttackTarget != None && m_AttackTarget.Base == ePawn && m_AttackTarget.BodyFlames.Length > 0 )
		{
			m_AttackTarget.AttenuateFire(0.8);
			ePawn.TakeDamage(0, None, Vect(0,0,0), Vect(0,0,0), Vect(0,0,0), class'EBurned');
		}

		// If npc dies while being grabbed
		if( m_AttackTarget != None && m_AttackTarget.GetStateName() != 's_Grabbed' )
		{
			GoToState('s_Grab', 'ReleaseDead');
			bInTransition = true;
			m_AttackTarget = None;
			return;
		}

		if(ePawn.Physics == PHYS_Falling)
			return ;

		if( bInGunTransition && (ePawn.Rotation.yaw&65535) == (Rotation.yaw&65535) )
			GotoState(,'Aim');

		if( bInTransition )
			return;

		GetAxes(Rotation,X,Y,Z);

		// Update acceleration.
		NewAccel            = aForward * X + aStrafe * Y;
		NewAccel.Z          = 0;
		ePawn.Acceleration  = NewAccel;
		ePawn.Acceleration *= eGame.m_onGroundAccel;

		ePawn.AimAt(AAHOH, Vector(Rotation), Vector(ePawn.Rotation), 0, 0, -30, 40);

		if(pushingForce > 0.3)
		{
			ePawn.GroundSpeed = pushingForce * m_speedGrabFP;
			ePawn.WalkingRatio = VSize(ePawn.Velocity / m_speedGrabFP);
			m_AttackTarget.WalkingRatio = ePawn.WalkingRatio;
			ePawn.PlayMove(ePawn.ABlendGrab, Rotation, ePawn.Velocity, 0.0, true, true);
			m_AttackTarget.PlayMove(m_AttackTarget.ABlendGrab, Rotation, ePawn.Velocity, 0.0,true,true);
		}
		else
		{
			ePawn.GroundSpeed = 0.0;
			ePawn.WalkingRatio = 0.0;
			ePawn.SoundWalkingRatio = 0;
			m_AttackTarget.WalkingRatio = 0.0;
			ePawn.LoopAnimOnly('grabstspnt1', , 0.2);
			m_AttackTarget.LoopAnimOnly(m_AttackTarget.AGrabWait, , 0.2);
		}
	}

	function NotifyFiring()
	{
		Level.RumbleVibrate(0.07, 0.75);
		ePawn.BlendAnimOverCurrent('grabstspfr1', 1, ePawn.UpperBodyBoneName);
	}

UnAim:
	GoToState('s_Grab', 'FromFP');

Aim:
	EMainHUD(myHud).Slave(ActiveGun);
	bInGunTransition = false;
	Stop;
}

// ----------------------------------------------------------------------
// state s_Carry
// ----------------------------------------------------------------------
state s_Carry
{
	Ignores Fire;

    function BeginState()
    {
		m_SMInTrans = false;
		DiscardPickup();
		bInTransition = true;
		m_camera.SetMode(ECM_CarryCr);
		ePawn.AddSoundRequest(Sound'FisherVoice.Play_Random_SamCarryBody', SLOT_SFX, 2.25f);
		ePawn.SoundWalkingRatio = 0.45;
    }

	function EndState()
	{
		bNoLedgePush = false;
		// If Sam dies, release Npc
		if( ePawn.Health <= 0 )
		{
			if( !ePawn.bIsCrouched )
				NpcSetup(ePawn.ToWorldDir(vect(-200,0,200)), 'CaryStAlQk0', true);
			else
				NpcSetup(ePawn.ToWorldDir(vect(-200,0,200)),'CaryCrAlQk0', true);
		}

		m_AttackTarget = None;
		ElapsedTime = 0.0f;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
		ePawn.SetPhysics(PHYS_Walking);
    }

	event MayFall()
	{
		if( !bInTransition && ShouldReleaseNPC())
		{
			CheckFallGrab();
			GotoState(, 'Falling');
		}
		// else Sam and NPC will fall together
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		return true;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		return false;
	}

	function ProcessScope()
	{
		if( !bInTransition && ePawn.Physics != PHYS_Falling)
			GotoState(, 'QuickDrop');
	}

	function ProcessInteract()
	{
		if( !bInTransition && ePawn.Physics != PHYS_Falling)
			GoToState(, 'Drop');
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);
		HandleBreathSounds(deltaTime);
	}

    function PlayerMove( float DeltaTime )
    {
        local vector X,Y,Z;
		local float tweenTime;
		local bool wasCrouch;

		if(m_SMInTrans)
			m_AttackTarget.RollPawn(0);

		// if Npc is on fire
		if( m_AttackTarget != None && m_AttackTarget.Base == ePawn && m_AttackTarget.BodyFlames.Length > 0 )
		{
			m_AttackTarget.AttenuateFire(0.8);
			ePawn.TakeDamage(0, None, Vect(0,0,0), Vect(0,0,0), Vect(0,0,0), class'EBurned');
		}

		if(bInTransition)
			return;

		GetAxes(ePawn.Rotation,X,Y,Z);

		// Update acceleration.
		SetPawnAccel(X, eGame.m_onGroundAccel);
		SetPawnRotation(40000);

		if( bDuck > 0 )
		{
			ePawn.bWantsToCrouch = !ePawn.bWantsToCrouch;
			bDuck = 0;
			if(VSize(ePawn.Velocity) == 0)
			{
				if(ePawn.bIsCrouched)
					GoToState(,'UnCrouch');
				else
					GoToState(,'Crouch');
				return;
			}
		}

		if(ePawn.bIsCrouched)
			m_camera.SetMode(ECM_CarryCr);
		else
			m_camera.SetMode(ECM_Carry);

		if(IsPushingGentle())
		{
			if(ePawn.bIsCrouched)
				ePawn.GroundSpeed = m_speedCarry * 0.8;
			else
				ePawn.GroundSpeed = m_speedCarry;
		}
		else
		{
			ePawn.GroundSpeed = 0.0;
		}

		wasCrouch = ePawn.GetAnimName() == 'CaryCrAlFd0' || ePawn.GetAnimName() == 'CaryCrAlNt0';
		if(!wasCrouch && ePawn.bIsCrouched || wasCrouch && !ePawn.bIsCrouched)
			tweenTime = 0.3;
		else
			tweenTime = 0.15;

		if(VSize(ePawn.Velocity) > 0)
		{
			if(ePawn.bIsCrouched)
				ePawn.LoopAnimOnly('CaryCrAlFd0', , tweenTime);
			else
				ePawn.LoopAnimOnly('CaryStAlFd0', , tweenTime);
		}
		else
		{
			if(ePawn.bIsCrouched)
				ePawn.LoopAnimOnly('CaryCrAlNt0', , tweenTime);
			else
				ePawn.LoopAnimOnly('CaryStAlNt0', , tweenTime);
		}
    }

	function NpcSetup( vector speed,  name AnimName, optional bool halfTurn )
	{
		local vector DropPos;
		local float randNum;
		bInTransition = true;
		KillPawnSpeed();
		ePawn.DetachFromBone(m_AttackTarget);
		m_AttackTarget.SetCollision(false, false, false);
		DropPos = ePawn.Location;
		if(ePawn.bIsCrouched)
			DropPos.Z += (ePawn.default.CollisionHeight - ePawn.CollisionHeight);
		m_AttackTarget.SetLocation(DropPos);
		m_AttackTarget.SetRotation(ePawn.Rotation);
		m_AttackTarget.SetCollision(true, false, false);
		m_AttackTarget.PlayAnimOnly(AnimName);
		if ( m_AttackTarget.Interaction != none )
			ENpcZoneInteraction(m_AttackTarget.Interaction).ResetInert();
		m_AttackTarget.SetBase(None);
		m_AttackTarget.SetPhysics(PHYS_Falling);
		m_AttackTarget.Velocity = speed;
		if(ePawn.Base != None)
			m_AttackTarget.Velocity += ePawn.Base.Velocity;
		m_AttackTarget.bCollideWorld = true;
		if( m_AttackTarget.Controller != None )
			m_AttackTarget.Controller.GoTostate('s_Unconscious');
		if( m_AttackTarget.Health > 0 )
			m_AttackTarget.GoTostate('s_Unconscious');
		else
			m_AttackTarget.GoTostate('s_Dying');

		m_AttackTarget.m_locationEnd = vect(0,0,0);
		if(halfTurn)
		{
			m_AttackTarget.ADeathNeutral = m_AttackTarget.ADeathLeftNtrl;
			m_AttackTarget.m_orientationEnd = rot(0, 32768, 0);
		}
		else
		{
			m_AttackTarget.ADeathNeutral = m_AttackTarget.ADeathForwardNtrl;
			m_AttackTarget.m_orientationEnd = rot(0, -16384, 0);
		}

	}

Falling:
	if(!ePawn.bIsCrouched)
		NpcSetup(ePawn.ToWorldDir(vect(-200,0,200)),'CaryStAlQk0', true);
	else
		NpcSetup(ePawn.ToWorldDir(vect(-200,0,200)),'CaryCrAlQk0', true);
	bInTransition = false;
	GoTostate('s_PlayerJumping');

QuickDrop:
	bNoLedgePush = true;
	if(!ePawn.bIsCrouched)
	{
		NpcSetup(ePawn.ToWorldDir(vect(-200,0,200)),'CaryStAlQk0', true);
		ePawn.PlayAnimOnly('CaryStAlQk0');
	}
	else
	{
		NpcSetup(ePawn.ToWorldDir(vect(-200,0,200)),'CaryCrAlQk0', true);
		ePawn.PlayAnimOnly('CaryCrAlQk0');
	}
	m_AttackTarget.bWasCarried = true;
	if(ActiveGun != None)
		Sleep(ePawn.GetAnimTime('CaryCrAlQk0') * 0.8);
	else
		FinishAnim();
	bInTransition = false;
	bNoLedgePush = false;
	// if ActiveGun, go in targeting mode automatically
	if( ActiveGun != None )
		OnGroundScope();
	else
		GotoState('s_PlayerWalking');
	Stop;

Drop:
	bInTransition = true;
	bNoLedgePush = true;
	KillPawnSpeed();
	m_camera.SetMode(ECM_CarryCr);
	if( !ePawn.bIsCrouched )
	{
		ePawn.PlayAnimOnly('CaryStAlBg0', 1.3, 0.1, true);
		FinishAnim();
	}
	NpcSetup(ePawn.ToWorldDir(vect(0,0,0)),'CaryCrAlDn0');
	m_AttackTarget.bWasCarried = true;

	// Make sur the NPC stays attached to Sam during DropDown anim. (for movers)
	m_AttackTarget.SetBase(ePawn);
	m_AttackTarget.SetPhysics(PHYS_None);
	m_AttackTarget.Velocity = vect(0,0,0);
	ePawn.PlayAnimOnly('CaryCrAlDn0');
	FinishAnim();
	m_AttackTarget.SetBase(None);
	m_AttackTarget.SetPhysics(PHYS_Falling);
	ePawn.SetPhysics(PHYS_Walking);
	if( !ePawn.bIsCrouched )
	{
		ePawn.PlayAnim(ePawn.AWaitCrouchIn, 1.3,,,true);
		FinishAnim();
	}
	bInTransition = false;
	bNoLedgePush = false;

	GotoState('s_playerwalking');

Crouch:
	bInTransition = true;
	ePawn.PlayAnimOnly('CaryStAlBg0', 1.3, , true);
	FinishAnim();
	bInTransition = false;
	m_camera.SetMode(ECM_CarryCr);
	Stop;

UnCrouch:
	bInTransition = true;
	ePawn.PlayAnimOnly('CaryStAlBg0', 1.3);
	FinishAnim();
	bInTransition = false;
	m_camera.SetMode(ECM_Carry);
	Stop;

Begin:
	bNoLedgePush = true;
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(ArrivalSpeedApprox(1.5), true);
	ePawn.SetRotation(ePawn.m_orientationEnd);
	ePawn.SetPhysics(PHYS_Walking);
	KillPawnSpeed();
	if(!ePawn.bIsCrouched)
	{
		ePawn.PlayAnimOnly('CaryCrAlUp0', 0.001, 0.3);
		Sleep(0.3);
	}
	if( m_AttackTarget.Controller != None )
		m_AttackTarget.Controller.GoTostate('s_Carried');
	m_AttackTarget.GoTostate('s_Carried');
	m_SMInTrans = true;
	m_AttackTarget.SetupNPCCarry(ePawn);
	ePawn.PlayAnimOnly('CaryCrAlUp0', , 0.1);
	FinishAnim(0);
	m_SMInTrans = false;

	// if grabbed npc is on fire, player will fire up
	NotifyBump(m_AttackTarget);

	m_AttackTarget.LoopAnimOnly('CaryStAlNt0', 0.0001, 0.0);
	m_AttackTarget.SetBase(None);	// Leave this line there. It is important
	ePawn.AttachToBone(m_AttackTarget, 'CarryBody');
	ePawn.LoopAnimOnly('CaryCrAlNt0', , 0.1);

	m_AttackTarget.SetRelativeRotation(rot(1796, -8345, 15840));
	m_AttackTarget.SetRelativeLocation(vect(17.152830, -0.340918, 0.698137));
	if(!ePawn.bIsCrouched)
	{
		ePawn.PlayAnimOnly('CaryStAlBg0', 1.3);
		FinishAnim();
		m_camera.SetMode(ECM_Carry);
	}
	else
		m_camera.SetMode(ECM_CarryCr);

	bInTransition = false;
	bNoLedgePush = false;
	Stop;
}

function bool OnGroundScope()
{
	if( ePawn.HandItem != None && ePawn.HandItem.bIsProjectile )
	{
		GotoState('s_Throw');
		return true;
	}

	//Log("ONGROUNDSCOPE"@ePawn.HandItem@ActiveGun@bInGunTransition);

	// Do Nothing if busy with weapon
	if( bInGunTransition )
		return false;

	// May draw weapon if nothing in hand
	if( ActiveGun != None && ePawn.HandItem == None )
		GotoState('s_FirstPersonTargeting');

	// Allow drawing other items than ActiveGun
	if( ePawn.HandItem != None && !ePawn.HandItem.Scope() )
	{
		// only aim if we have a gun
		if( MainGun != None || HandGun != None )
			GotoState('s_FirstPersonTargeting');
	}

	// Do Nothing if no weapon in inventory
	else if( ActiveGun == None )
		return false;

	return true;
}

// ----------------------------------------------------------------------
// state s_PlayerWalking
// ----------------------------------------------------------------------
state s_PlayerWalking
{
	Ignores MayReload;

    function BeginState()
    {
		if(ePawn.WeaponStance != 0 && !ePawn.IsAnimating(ePawn.ACTIONCHANNEL))
		{
			ForceGunAway(true);
			ePawn.WeaponStance = 0;
			ePawn.SwitchAnims();
			Log("ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR");
			Log("ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR");
			Log("ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR - ERROR");
		}
		NormalIKFade();
		ePawn.SetPhysics(PHYS_Walking);
		SetWalkingCameraMode(VSize(ePawn.Velocity));
		m_RunStartTime = 0.0;
    }

    function EndState()
    {
		ePawn.IdleTime=0;
		prevWalkingRatio = 0.0;
		ElapsedTime = 0.0;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathRun', SLOT_SFX );
			bIsPlaying = false;
		}
    }

	function SetPawnAccel(vector X, float accel)
	{
		local rotator targetRot;
		local vector NewAccel;

		if(IsPushing())
		{
			targetRot = Rotation + Rotator(GetPushingDir());
			targetRot.Pitch = 0;
			targetRot.Roll = 0;
			if(X dot Vector(targetRot) > 0.5)
			{
				NewAccel = X;
			}
		}
		NewAccel.Z = 0;
		if(!IsPushing() || !((VSize(NewAccel) == 0) && (VSize(ePawn.Velocity) > 0.0)))
		{
			ePawn.Acceleration  = NewAccel;
			ePawn.Acceleration *= accel;
		}
	}

	function SetPawnRotation(float rotSpeed)
	{
		local rotator newRot, deltaRot, prevRot;
		local vector pushingDir;
		local float speedRatio, turnSpeedRatio;

		pushingDir = GetPushingDir();
		if(VSize(pushingDir) > 0.0)
		{
			newRot.Yaw = Rotation.Yaw + Rotator(pushingDir).Yaw;
			prevRot = ePawn.Rotation;
			rotSpeed = FMin(rotSpeed, rotSpeed *  (1.0 - (VSize(ePawn.Velocity) / (m_speedRun * 4.0))));
			ePawn.RotateTowardsRotator(newRot, rotSpeed / 0.5, 0.5);
			deltaRot = ePawn.Rotation - prevRot;
			if(ePawn.Velocity dot Vector(ePawn.Rotation) > 0.0)
				ePawn.Velocity = ePawn.Velocity >> deltaRot;
			if(ePawn.Acceleration dot Vector(ePawn.Rotation) > 0.0)
				ePawn.Acceleration = ePawn.Acceleration >> deltaRot;
		}
		speedRatio = FClamp(VSize(ePawn.Velocity) / m_speedRun, 0.0, 1.0);
		deltaRot = Normalize(deltaRot);
		turnSpeedRatio = FClamp((float(deltaRot.Yaw) / Level.m_dt) / rotSpeed, -1.0, 1.0);
		ePawn.RollPawn(int(speedRatio * turnSpeedRatio * 2000.0));
	}

	function ProcessScope()
	{
		if( !bInTransition )
			OnGroundScope();
	}

	function ProcessFire()
	{
        local vector X,Y,Z, HitLocation, HitNormal, Start, End, NearPawnDir;
		local ePawn NearPawn;
		local Actor NearActor;
		local float AngleDir;

		if(PlayerInput.bFireToDrawGun)
		{
			GetAxes(ePawn.Rotation,X,Y,Z);

			Start = ePawn.Location;
			End = Start + (ePawn.CollisionRadius + 40.0f) * X;

			NearActor = ePawn.Trace(HitLocation, HitNormal, End, Start, , , , true);

			// Check if breakable stuff
			if(NearActor != None && NearActor.IsA('EBreakableGlass'))
			{
				m_targetObject = NearActor;
				GotoState('s_BreakObject');
				return;
			}

			// Check if it's a pawn
			NearPawn = EPawn(NearActor);
			if(NearPawn == None)
			{
				// check right
				Start += Y * 30.0;
				End += Y * 30.0;
				NearPawn = EPawn(ePawn.Trace(HitLocation, HitNormal, End, Start, , , , true));
			}
			if(NearPawn == None)
			{
				// check left
				Start -= Y * 60.0;
				End -= Y * 60.0;
				NearPawn = EPawn(ePawn.Trace(HitLocation, HitNormal, End, Start, , , , true));
			}
			if( NearPawn != None && !NearPawn.bIsDog && !NearPawn.bDyingDude) // bDyingDude is for special case (1_1_0 and 2_2_2)
			{
				NearPawnDir		= NearPawn.ToWorldDir(Vect(1,0,0));
				AngleDir		= NearPawnDir Dot Normal(ePawn.Location - NearPawn.Location);
				m_AttackTarget	= NearPawn;

				if(AngleDir < -0.3)
					JumpLabelPrivate = 'Back';
				else
					JumpLabelPrivate = 'Front';

				GotoState('s_ShortRangeAttack');
			}
			else
				Global.ProcessFire();
		}
		else
			Global.ProcessFire();
	}

	function ProcessBackToWall()
	{
		if(CheckBTW(, true))
		{
			bIntransition = true;
			GotoState('s_PlayerBTW', 'Entering');
		}
		else if(CheckBTW(true, true))
		{
			bIntransition = true;
			GotoState('s_PlayerBTW', 'EnteringBack');
		}
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);

		if ( ePawn.SoundWalkingRatio >= 0.65f && VSize(ePawn.Velocity) != 0.0f && !ePawn.bIsCrouched)
			ElapsedTime += deltaTime;
		else
			ElapsedTime = 0.0f;

		if ( ElapsedTime > 7.0f )
		{
			if (!bIsPlaying)
			{
				ePawn.PlaySound( Sound'FisherVoice.Play_Sq_FisherBreathRun', SLOT_SFX );
				bIsPlaying = true;
			}
		}
		else
		{
			if (bIsPlaying)
			{
				ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathRun', SLOT_SFX );
				bIsPlaying = false;
			}
		}
	}

    function PlayerMove( float DeltaTime )
    {
        local vector X,Y,Z, HitLocation, HitNormal, Start, End, NearPawnDir;
		local ePawn NearPawn;
		local Actor NearActor;
		local int PillTag;
		local float AngleDir;
		local float accelModif;

		if(m_camera.m_volSize != EVS_Minute)
			PawnLookAt();

		if( bInTransition )
			return;

		GetAxes(ePawn.Rotation,X,Y,Z);

		//clauzon the player can now change is speed level
		if(m_curWalkSpeed!=0)
		{
			accelModif = float(m_curWalkSpeed) / eGame.m_maxSpeedInterval;
			aForward *= accelModif;
			aStrafe  *= accelModif;
		}

		// Update acceleration.
		SetPawnAccel(X, eGame.m_onGroundAccel);
		SetPawnRotation(80000);

		// Check if player wants to roll
		//
		// Player must not be already crouched
		// Must be going forward (Over a certain speed?)
		//
		if( bDuck > 0 &&  IsPushingFull() && (VSize(ePawn.Velocity) > (m_speedRunCr - 1.0)))
		{
			//Log("playermove walking"@bIntransition);
			GotoState('s_Roll', 'FromWalking');
			return;
		}

		CheckForCrouch();
		ePawn.ResetZones();

	    SetGroundSpeed();
		PlayOnGround(0.4);

		if( m_camera.m_volSize == EVS_Minute || bInGunTransition )
			return;

		if( bFire == 1 && !PlayerInput.bFireToDrawGun)
		{
			bFire = 0;
			Start = ePawn.Location;
			End = Start + (ePawn.CollisionRadius + 40.0f) * X;

			NearActor = ePawn.Trace(HitLocation, HitNormal, End, Start, , , , true);

			// Check if breakable stuff
			if(NearActor != None && NearActor.IsA('EBreakableGlass'))
			{
				m_targetObject = NearActor;
				GotoState('s_BreakObject');
				return;
			}

			// Check if it's a pawn
			NearPawn = EPawn(NearActor);
			if(NearPawn == None)
			{
				// check right
				Start += Y * 30.0;
				End += Y * 30.0;
				NearPawn = EPawn(ePawn.Trace(HitLocation, HitNormal, End, Start, , , , true));
			}
			if(NearPawn == None)
			{
				// check left
				Start -= Y * 60.0;
				End -= Y * 60.0;
				NearPawn = EPawn(ePawn.Trace(HitLocation, HitNormal, End, Start, , , , true));
			}
			if( NearPawn != None && !NearPawn.bIsDog && !NearPawn.bDyingDude) // bDyingDude is for special case (1_1_0 and 2_2_2)
			{
				NearPawnDir		= NearPawn.ToWorldDir(Vect(1,0,0));
				AngleDir		= NearPawnDir Dot Normal(ePawn.Location - NearPawn.Location);
				m_AttackTarget	= NearPawn;

				if(AngleDir < -0.3)
					JumpLabelPrivate = 'Back';
				else
					JumpLabelPrivate = 'Front';

				GotoState('s_ShortRangeAttack');
			}
		}
    	else if( bPressedJump )
        {
			if(ePawn.bIsCrouched)
			{
				Start = ePawn.Location;
				Start.Z += ePawn.Default.CollisionHeight - ePawn.Default.CrouchHeight;
				End.X = ePawn.Default.CollisionRadius;
				End.Y = End.X;
				End.Z = ePawn.Default.CollisionHeight;
				if(ePawn.FastPointCheck(Start, End, true, true))
					GoToState(, 'jumpstart');
			}
			else
				GoToState(, 'jumpstart');
        }
    }

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function bool CanAccessQuick()
	{
		return !bInTransition && !bInGunTransition;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		return true;
	}

	function bool CanInteract()
	{
		return !bInGunTransition && !bInTransition;
	}

	function GEOUnCrouch()
	{
		KillPawnSpeed();
		ePawn.bWantsToCrouch = False;
		ePawn.UnCrouch(false);
		m_LPStartTime = Level.TimeSeconds;
	}

	event bool NarrowLadder(vector locationEnd, rotator orientationEnd, int NLType)
	{
		local vector HitLocation, HitNormal;
		// We hit a ladder with the arms zone. If we push long enough on it, we grab it
		if(IsPushingToward(-ePawn.m_geoNormal, m_towardAngle))
		{
			if(ePawn.bIsCrouched)
			{
				GEOUnCrouch();
			}
			else if(m_LPStartTime < (Level.TimeSeconds - eGame.m_grabbingDelay))
			{
				if(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None)
				{
					// We grab it
					GoToState('s_GrabbingNarrowLadder', 'FromGround');
					return true;
				}
			}
			else
				KillPawnSpeed();
		}
		else
			m_LPStartTime = 0.0;

		return false;
	}

	event bool TopNarrowLadder(vector locationEnd, rotator orientationEnd)
	{
		local vector HitLocation, HitNormal, destination, extent;
		// We hit a ladder with the feet zone. If we push long enough on it, we grab it
		if(IsPushingToward(ePawn.m_geoNormal, m_towardAngle))
		{
			if(ePawn.bIsCrouched)
			{
				GEOUnCrouch();
			}
			else if(m_LPStartTime < (Level.TimeSeconds - eGame.m_grabbingDelay))
			{
				extent = ePawn.GetExtent();
				destination = locationEnd + ((vect(1,0,0) >> orientationEnd) * (extent.X + extent.X + 7.0));
				destination.Z -= extent.Z;
				if(	ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, extent, , true) == None &&
					ePawn.FastPointCheck(destination, extent, true, true) &&
					ePawn.FastPointCheck(destination + vect(0,0,40.0), extent, true, true))
				{
					// We grab it
					GoToState('s_GrabbingNarrowLadder', 'FromTop');
					return true;
				}
			}
			else
				KillPawnSpeed();
		}
		else
			m_LPStartTime = 0.0;

		return false;
	}

	event bool Pole(vector locationEnd, rotator orientationEnd, int RType)
	{
		local vector HitLocation, HitNormal;
		// We hit a zone with the arms zone. If we push long enough on it, we grab it
		if(IsPushingTowardFront(m_towardAngle))
		{
			if(ePawn.bIsCrouched)
			{
				GEOUnCrouch();
			}
			else if(m_LPStartTime < (Level.TimeSeconds - eGame.m_grabbingDelay))
			{
				// We have a Pole in the arms zone.
				if(	(RType == 0 || RType == 2) &&
					(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None))
				{
					// We grab it
					GoToState('s_GrabbingPole', 'FromGround');
					return true;
				}
			}
			else
				KillPawnSpeed();
		}
		else
			m_LPStartTime = 0.0;

		return false;
	}

	event bool Pipe(vector locationEnd, rotator orientationEnd)
	{
		local vector HitLocation, HitNormal;
		// We hit a pipe with the arms zone. If we push long enough on it, we grab it
		if(IsPushingToward(-ePawn.m_geoNormal, m_towardAngle))
		{
			if(ePawn.bIsCrouched)
			{
				GEOUnCrouch();
			}
			else if(m_LPStartTime < (Level.TimeSeconds - eGame.m_grabbingDelay))
			{
				if(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None)
				{
					// We grab it
					GoToState('s_GrabbingPipe', 'FromGround');
					return true;
				}
			}
			else
				KillPawnSpeed();
		}
		else
			m_LPStartTime = 0.0;

		return false;
	}

	event bool TopPipe(vector locationEnd, rotator orientationEnd)
	{
		local vector HitLocation, HitNormal;
		// We hit a pipe with the feet zone. If we push long enough on it, we grab it
		if(IsPushingToward(ePawn.m_geoNormal, m_towardAngle))
		{
			if(ePawn.bIsCrouched)
			{
				GEOUnCrouch();
			}
			else if(m_LPStartTime < (Level.TimeSeconds - eGame.m_grabbingDelay))
			{
				if(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None)
				{
					// We grab it
					GoToState('s_GrabbingPipe', 'FromTop');
					return true;
				}
			}
			else
				KillPawnSpeed();
		}
		else
			m_LPStartTime = 0.0;

		return false;
	}

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		local vector normal, position;

		if(m_camera.m_volSize == EVS_Minute)
			return true;

		// Because of the StepUp function in UnPhysics.cpp
		position = ePawn.Location;
		position.Z = ePawn.OldZ;

		if(	IsPushingTowardFront(m_towardAngle) &&
			ePawn.CheckFence(normal, wall, position) &&
			((HitNormal dot Vector(ePawn.Rotation)) < -0.707))
		{
			if(ePawn.bIsCrouched)
			{
				ePawn.bWantsToCrouch = false;
				ePawn.UnCrouch(false);
				m_LPStartTime = Level.TimeSeconds;
			}
			else if(m_LPStartTime < (Level.TimeSeconds - eGame.m_grabbingDelay))
			{
				m_EnterDir = GetPushingDir();
				ePawn.m_validFence = true;
				ePawn.SetRotation(Rotator(-normal));
				GoToState('s_Fence', 'FromGround');
				ePawn.SetBase(wall, normal);
			}
		}
        return true;  // don't notify pawn
    }

begin:
	if( bInTransition )
	{
		LOG("ERROR: This shouldn't be happening anymore. Each states finish his own stuff before coming back to PlayerWalking");
		//FinishAnim();           // finish any transitions from other states
		m_LPStartTime = 0.0;
	}
	bInTransition = false;
	Stop;

jumpstart:
	bInTransition = true;
	bJump = 0;
	if(ePawn.PlayJumpStart())
		FinishAnim();
	else if( ePawn.bWantsToCrouch )
		Sleep(0.2);
	ePawn.DoJumping();
	m_NbJump = 1;
	GotoState('s_PlayerJumping');
}

native(1159) final function SetWalkingCameraMode(float speed);
native(1160) final function SetGroundSpeed();

function PlayOnGround(float waitTween)
{
	local float speed, blendRate;
	local Rotator deltaRot;
	local name CurrentAnimSeq;
	local float CurrentFrame, CurrentRate;

	ePawn.GetAnimParams(0, CurrentAnimSeq, CurrentFrame, CurrentRate);

	//Log("PlayOnGround"@waitTween@speed);

	ePawn.SetPhysics(PHYS_Walking);
	speed = VSize(ePawn.Velocity);

	// For breaking
	if(speed < eGame.m_PlayerJoggingThreshold)
		m_RunStartTime = Level.TimeSeconds;

	// Reset time when moving, turning or when you're frozen
	if( speed > 0 || aTurn != 0 || bStopInput )
	{
		ePawn.IdleTime=0;
	}
	// If is 0, reset to current time to begin idle timer
	else if( ePawn.IdleTime == 0 )
	{
		ePawn.IdleTime = Level.TimeSeconds;
	}

	// Clean ref pose
	ePawn.AnimBlendParams(EPawn.REFPOSECHANNEL,0,0,0);

	if(m_camera.m_volSize == EVS_Minute)
	{
		if(speed > 0.0)
		{
			ePawn.SoundWalkingRatio = 0.5f;
			ePawn.AnimBlendToAlpha(1,0,0.10);
			ePawn.LoopAnim('VentCrAlFd0', 1.0, 0.3);
		}
		else
		{
			ePawn.PlayWait(0, waitTween);
		}
		SetWalkingCameraMode(speed);
		return;
	}

	if(	speed > 100.0 ||
		speed > 0.0 && speed < 100.0 && VSize(ePawn.Acceleration) > 0.0 )
	{
		if( ePawn.IsJumpingForward() )	// If we're jumping forward
		{
			if( ePawn.IsAnimating() )	// Do nothing, wait for anim to finish
			{
				if(CurrentFrame < 0.59) // Hit head on something, cancel landing to prevent slide
					waitTween = 0.2;
				else
					return;	// normal running landing
			}
			else						// tween in fast since last jump forward frame is first run frame
				waitTween = 0.01;
		}

		if( ePawn.bIsCrouched )
		{
			blendRate = (1.0 - ePawn.WalkingRatio) + ((ePawn.GetAnimTime(ePawn.AWalkCrouch) / ePawn.GetAnimTime(ePawn.AJoggCrouch)) * ePawn.WalkingRatio);
			ePawn.LoopAnim(ePawn.AWalkCrouch, blendRate, waitTween * 0.5);
			ePawn.SynchAnim(ePawn.AJoggCrouch, 1, 0.2, FClamp(ePawn.WalkingRatio, 0.0, 0.99));
		}
		else
		{
			blendRate = (1.0 - ePawn.WalkingRatio) + ((ePawn.GetAnimTime(ePawn.AWalk) / ePawn.GetAnimTime(ePawn.AJogg)) * ePawn.WalkingRatio);
			ePawn.LoopAnim(ePawn.AWalk, blendRate, waitTween * 0.5);
			ePawn.SynchAnim(ePawn.AJogg, 1, 0.2, FClamp(ePawn.WalkingRatio, 0.0, 0.99));
		}
	}
    else // we ain't movin'
    {
		if (prevWalkingRatio == 1.0f)
		{   //Play a sound when Sam goes from full run to a complete stop
			ePawn.Bark_Type = BARK_AboutToDie;
			ePawn.PlaySound(ePawn.Sounds_Move, SLOT_SFX);
		}
		prevWalkingRatio = 0.0f;

		deltaRot = ePawn.Rotation - ePawn.m_prevRot;
		deltaRot = Normalize(deltaRot);
		ePawn.PlayWait(deltaRot.Yaw / Level.m_dT, 0.2f);
    }

	SetWalkingCameraMode(speed);
}



function PlayOnGroundSniping( bool bMoving, float Speed, optional bool bBlendRefPose )
{
	local ePawn.SAnimBlend AnimSet;

	ePawn.SetPhysics(PHYS_Walking);

	//
	// Lower body movement/wait
	//
    if( bMoving )
	{
		ePawn.GroundSpeed = Speed;
		if(ePawn.bIsCrouched)
			AnimSet = ePawn.ABlendSnipingCrouch;
		else
			AnimSet = ePawn.ABlendSniping;

		ePawn.PlayMove(AnimSet, Rotation, ePawn.Velocity, 0.3, true, true);
	}
    else
	{
		ePawn.GroundSpeed = 0.0;
		ePawn.PlayWait(0.0f, 0.2);
	}

	//
	// Upper body pose
	//
	if( !bBlendRefPose )
		return;

	// Loop extra wait animation on Channel REFPOSECHANNEL. To be able to have different weapon with same base leg movement
    ePawn.AnimBlendParams(EPawn.REFPOSECHANNEL, 1, 0.0, 0.0, ePawn.UpperBodyBoneName);
	if( ePawn.bIsCrouched )
		ePawn.LoopAnim(ePawn.AWaitCrouch, 1, 0.2, EPawn.REFPOSECHANNEL);
	else
		ePawn.LoopAnim(ePawn.AWait, 1, 0.2, EPawn.REFPOSECHANNEL);
}

// ----------------------------------------------------------------------
// state s_Roll
// ----------------------------------------------------------------------
state s_Roll
{
	Ignores Fire, ProcessHeadSet;

	function BeginState()
	{
		ePawn.bWantsToCrouch	= true;
		m_rollScope				= false;
		m_rollGetUp				= false;
		m_rollCrouch = ePawn.bIsCrouched;
	}

	function EndState()
	{
		ePawn.bWantsToCrouch = !m_rollGetUp;
		bDuck = 0;
	}

	function SetPawnRotation(float rotSpeed)
	{
		local rotator newRot;
		local vector pushingDir;

		pushingDir = GetPushingDir();
		if(VSize(pushingDir) > 0.0)
		{
			newRot.Yaw = Rotation.Yaw + Rotator(pushingDir).Yaw;
			if((Vector(newRot) dot Vector(ePawn.Rotation)) > 0.0)
			{
				ePawn.RotateTowardsRotator(newRot, rotSpeed / 0.5, 0.5);
			}
		}
	}

	function PlayerMove( Float DeltaTime )
	{
		local rotator prevRot, deltaRot;

		// Check to get out crouched/uncrouched
		if( bInTransition && bDuck > 0 )
		{
			m_rollGetUp = !m_rollGetUp;
			bDuck = 0;
		}

		if( !bInTransition && bDuck == 0 )
		{
			if(m_rollCrouch)
				m_rollGetUp = true;
			GotoState('s_PlayerWalking');
			prevWalkingRatio = ePawn.WalkingRatio;
			return;
		}

		prevRot				= ePawn.Rotation;
		SetPawnRotation(15000);
		deltaRot			= ePawn.Rotation - prevRot;
		ePawn.Velocity		= ePawn.Velocity >> deltaRot;
		ePawn.Acceleration	= ePawn.Acceleration >> deltaRot;
	}

	event NotifyAction()
	{
		if(!IsPushingGentle())
			KillPawnSpeed();
	}

	function Timer()
	{
		bInTransition = true;
		bDuck = 0;
		GotoState(, 'DoRoll');
	}

	function ProcessScope()
	{
		m_rollScope = true;
	}

FromWalking:
	SetTimer(0.2f,false);
	Stop;

DoRoll:
	m_camera.SetMode(ECM_WalkingCr);
	ePawn.bWantsToCrouch = true;
	ePawn.GroundSpeed = eGame.m_RollSpeed;
	ePawn.PlayRoll();
	FinishAnim();

	bInTransition = false;
	if( m_rollScope )
	{
		OnGroundScope();
	}
	else
	{
		if( m_rollGetUp )
			ePawn.bWantsToCrouch = false;
		ePawn.LoopAnim(ePawn.AWalkCrouch, 1.0, 0.0);
		ePawn.SynchAnim(ePawn.AJoggCrouch, 1, 0.0, 1.0);
		prevWalkingRatio = 1.0;
		ePawn.WalkingRatio = prevWalkingRatio;
		ePawn.SoundWalkingRatio = prevWalkingRatio;
		GotoState('s_PlayerWalking');
	}
}

// ----------------------------------------------------------------------
// state PlayerFlying
// ----------------------------------------------------------------------
state PlayerFlying
{
	Ignores Bump;

	function PlayerMove( float DeltaTime )
	{
		local vector X,Y,Z;

		GetAxes(Rotation,X,Y,Z);
		Pawn.Acceleration = (aForward*X + aStrafe*Y) * 30000;

		ePawn.bVisibilityCalculated = true;
		ePawn.VisibilityFactor = 0.0f;

		if (Pawn.Acceleration == vect(0,0,0))
			Pawn.Velocity = vect(0,0,0);

		ePawn.PlayWait(0);
	}

	function BeginState()
	{
		Pawn.SetPhysics(PHYS_Flying);
		m_camera.SetMode(ECM_FSphere);
	}
}

// ----------------------------------------------------------------------
// Use this for any situation where Player should use Palm
// ----------------------------------------------------------------------
state s_UsingPalm
{
	Ignores AltFire, Fire, ProcessHeadSet;

	function BeginState()
	{
		bLockedCamera = true;
		JumpLabelPrivate = '';
		KillPawnSpeed();
		ePawn.PlayWait(0.0, 0.1);
	}

	function EndState()
	{
		bLockedCamera = false;
		EMainHUD(myHud).NormalView();
	}

	function bool OnGroundScope()
	{
		JumpLabelPrivate = 'BackToTargeting';
		return Global.OnGroundScope();
	}

BackToTargeting:
	Stop;

Begin:
	// wait for end of weapon sheathing
	FinishAnim(EPawn.ACTIONCHANNEL);

	if( JumpLabelPrivate != '' )
	{
		OnGroundScope();
		Stop;
	}

	ePawn.AnimBlendParams(EPawn.ACTIONCHANNEL, 1,,,ePawn.UpperBodyBoneName);
	if( !ePawn.bIsCrouched )
		ePawn.LoopAnim('palmstalnt0',,0.8, EPawn.ACTIONCHANNEL);
	else
		ePawn.LoopAnim('palmcralnt0',,0.8, EPawn.ACTIONCHANNEL);
}

function float GetGroundSpeed()
{
	if( ePawn.bIsCrouched )
		return m_speedWalkFPCr;
	else
		return m_speedWalkFP;
}

// ----------------------------------------------------------------------
// state s_Targeting (BASE STATE)
// ----------------------------------------------------------------------
state s_Targeting
{
	function BeginState()
	{
		FastIKFade();

		ePawn.SetPhysics(PHYS_Walking);
		m_useTarget	= true;
	}

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = m_turnMul;
	}

	function EndState()
	{
		m_useTarget	= false;
		if( EMainHUD(myHud).hud_master == ePawn.HandItem )
			EMainHUD(myHud).NormalView();

		if( !bInTransition )
			SheathWeapon();
	}

	// Mission failed to bring back in 3rd person
	function ProcessEndMission()
	{
		bFire = 0; // Force this so that ProcessScope may be valid
		ProcessScope();
	}

	// Make sure we don't lose weapon while using it
	function NotifyLostInventoryItem( EInventoryItem Item )
	{
		// Force release fire
		bFire = 0;
		if( Item != None && Item == ePawn.HandItem )
			ProcessScope();
		// Reset things
		Global.NotifyLostInventoryItem(Item);
	}

	function SwitchCameraMode()
	{
		if(ePawn.bIsCrouched)
			m_camera.SetMode(ECM_FirstPersonCr);
		else
			m_camera.SetMode(ECM_FirstPerson);
	}

    function PlayerMove( float DeltaTime )
    {
        local vector X,Y,Z, NewAccel;
		local float pushingForce;

		pushingForce = GetPushingForce();
        GetAxes(Rotation,X,Y,Z);

		if( bLockedCamera )
			KillPawnSpeed();
		else
		{
			// Update acceleration.
			NewAccel            = aForward*X + aStrafe*Y;
			NewAccel.Z          = 0;
			ePawn.Acceleration  = NewAccel;
			ePawn.Acceleration *= eGame.m_onGroundAccel;

			ePawn.WalkingRatio = VSize(ePawn.Velocity / GetGroundSpeed());

			CheckForCrouch();

			ePawn.AimAt(AAVERT, Vector(Rotation), Vector(ePawn.Rotation), 0, 0, -90, 90);
		}

		PlayOnGroundSniping((pushingForce > 0.3), pushingForce * GetGroundSpeed(), true);
	}

	function bool CanAccessQuick()
	{
		return ePawn.FullInventory.GetSelectedItem() == MainGun && !bInTransition && !bInGunTransition && !bLockedCamera;
	}

	function bool CanSwitchToHandItem( EInventoryItem Item )
	{
		return Item.IsA('ESecondaryAmmo');
	}
}

// ----------------------------------------------------------------------
// state s_FirstPersonTargeting
// ----------------------------------------------------------------------
state s_FirstPersonTargeting extends s_Targeting
{
	function BeginState()
	{
		Super.BeginState();

		if( !bInTransition )
			DrawWeapon();

		ePawn.SoundWalkingRatio = 0.45f;
	}

	function EndState()
	{
		// Force releasing triggger when getting out.
		bFire = 0;
		Super.EndState();
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		if( IntObj.class.name == 'EDoorInteraction' )
			return true;
	}

	function bool CanInteract()
	{
		return !bInTransition && !bInGunTransition;
	}

	// Prevent gun shot when in air camera
	function ProcessFire()
	{
		if( !bLockedCamera )
			Global.ProcessFire();
	}

	function ProcessAltFire()
	{
		if( !bLockedCamera )
			Global.ProcessAltFire();
	}

	function NotifyFiring()
	{
		if( ePawn.WeaponStance == 1 )
		{
			Level.RumbleVibrate(0.07, 0.75);
			if( !ePawn.bIsCrouched )
				ePawn.BlendAnimOverCurrent('WaitStSpFr1', 1, ePawn.UpperBodyBoneName);
			else
				ePawn.BlendAnimOverCurrent('WaitCrSpFr1', 1, ePawn.UpperBodyBoneName);
		}
		else
			Global.NotifyFiring();
	}

	function ProcessScope()
	{
		if( bLockedCamera )
			return;

		if( !bInGunTransition && !ePawn.PressingFire() )
			GotoState('s_PlayerWalking');
	}

	// from cam
	function bool OnGroundScope()
	{
		EMainHUD(myHud).Slave(ActiveGun);
		return false;
	}

	function NotifyStartCrouch()
	{
		m_camera.SetMode(ECM_FirstPersonCr);
	}

	function NotifyEndCrouch()
	{
		m_camera.SetMode(ECM_FirstPerson);
	}

    function PlayerMove( float DeltaTime )
    {
		Super.PlayerMove(DeltaTime);

		if( bMustZoomIn && ActiveGun == MainGun && !bInGunTransition && !bLockedCamera )
		{
			bInTransition = true; // prevent the SheathWeapon
			GotoState('s_PlayerSniping');
		}
	}

Begin:
	// will not happen from Sniper mode
	// DrawWeapon in BeginState to prevent a PlayerMove with bInGunTransition==false
	if( !bInTransition )
		FinishAnim(EPawn.ACTIONCHANNEL);

	SwitchCameraMode();
	EMainHUD(myHud).Slave(ActiveGun);
	bInTransition = false;

	// If mission failed happened during draw weapon anim
	// send after anim is over.
	if( iGameOverMsg != -1 )
		ProcessScope();
}

// ----------------------------------------------------------------------
// state s_PlayerSniping
// ----------------------------------------------------------------------
state s_PlayerSniping extends s_Targeting
{
	function BeginState()
	{
		Super.BeginState();
		ePawn.PlaySound(Sound'FisherEquipement.Play_FN2000Zoom', SLOT_SFX);
	}

	function EndState()
	{
		Super.EndState();

		ZoomLevel	= 0.0f;
		SetCameraFOV(self, DesiredFov);			// restore Fov that may have changed in sniping mode
		ESniperGun(ActiveGun).SetSniperMode(false);
		bHideSam	= false;
	}

	function float GetSniperPosVar()
	{
		if(ePawn.bIsCrouched)
			return 1.2;
		else
			return 1.0;
	}

	function ReduceCameraSpeed(out float turnMul)
	{
		if(m_camera.m_camMode == ECM_FirstPerson)	// turn fast like OverShoulder while reloading
			turnMul = m_turnMul;
		else if(!m_holdingBreath)
			turnMul = (ESniperGun(ActiveGun).GetZoom() / DesiredFOV) * 0.5;
		else
			turnMul = (ESniperGun(ActiveGun).GetZoom() / DesiredFOV) * 0.25;
	}

	function SwitchCameraMode()
	{
		m_camera.SetMode(ECM_Sniping);
	}

	// Reload in Sniper mode stuff
	function NotifyReloading()
	{
		ZoomLevel			= 0.0f;
		SetCameraFOV(self, DesiredFov);			// restore Fov that may have changed in sniping mode
		ESniperGun(ActiveGun).SetSniperMode(false);
		bHideSam		= false;

		m_camera.SetMode(ECM_FirstPerson);
		Global.NotifyReloading();
		GotoState(,'Reload');
	}

	function ProcessScope()
	{
		if( !bInGunTransition )
			GotoState('s_PlayerWalking');
	}

	function ProcessHeadSet( float i )
	{
		if( ePawn.HandItem == None || bInGunTransition || bInTransition )
			return;
		Global.ProcessHeadSet(i);
	}

    function PlayerMove( float DeltaTime )
    {
		// No update on ZoomLevel when reloading
		if( bHideSam )
		{
			// Update this value for motion blur
			ZoomLevel=FMax(Abs(aTurn),Abs(aLookUp));
			ZoomLevel=FClamp(ZoomLevel,0.0f,1.0f);
			ZoomLevel*=0.7f;
		}

		if( bMustZoomOut && !bInGunTransition )
		{
			bInTransition = true; // prevent the DrawWeapon
			GotoState('s_FirstPersonTargeting');
		}
		else if( !m_holdingBreath )
   			Super.PlayerMove(DeltaTime);
		else
			KillPawnSpeed();
	}

	function float GetGroundSpeed()
	{
		return m_speedWalkSniping;
	}

Reload:
	FinishAnim(EPawn.ACTIONCHANNEL);
Begin:
	SwitchCameraMode();
	bHideSam = true;
	EMainHUD(myHud).Slave(ActiveGun);
	ESniperGun(ActiveGun).SetSniperMode(true);
	bInTransition = false;
}

// ----------------------------------------------------------------------
// state s_CameraJammerTargeting
// ----------------------------------------------------------------------
state s_CameraJammerTargeting extends s_Targeting
{
	Ignores NotifyWeapon;

	function BeginState()
	{
		Super.BeginState();
		bInTransition = true;
	}

	function EndState()
	{
		bInTransition = true; // Prevents s_Targeting::EndState()
		CheckHandObject();
		Super.EndState();
		bInTransition = false;
	}

	function ProcessFire()
	{
		if( !bInTransition )
			Global.ProcessFire();
	}

	function ProcessScope()
	{
		if( !bInTransition )
			ePawn.HandItem.Scope();
	}

	// Force one hand animation even if ActiveGun != None
	function name GetWeaponAnimation()
	{
		if( !ePawn.bIsCrouched )
			return 'DrawStSpBg1';
		else
			return 'DrawCrSpBg1';
	}

	function NotifyStartCrouch()
	{
		m_camera.SetMode(ECM_FirstPersonCr);
	}
	function NotifyEndCrouch()
	{
		m_camera.SetMode(ECM_FirstPerson);
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

Begin:
	// Get camera out in hands
	ePawn.BlendAnimOverCurrent(GetWeaponAnimation(), 1, ePawn.UpperBodyBoneName);
	// Force hand stance
	ePawn.WeaponStance = 1;
	ePawn.SwitchAnims();
	SwitchCameraMode();
	FinishAnim(EPawn.ACTIONCHANNEL);
	EMainHUD(myHud).Slave(ePawn.HandItem);
	bInTransition = false;
}

// ----------------------------------------------------------------------
// state s_LaserMicTargeting
// ----------------------------------------------------------------------
state s_LaserMicTargeting extends s_Targeting
{
	Ignores ProcessHeadSet;

	event bool CanSaveGame()
	{
		return false;
	}

	function BeginState()
	{
		Super.BeginState();
		bHideSam = true;
	}

	function EndState()
	{
		bInTransition = true;
		Super.EndState();
		bHideSam = false;

		// Stop microiing
		ePawn.HandItem.GotoState('s_Selected');
	}

	function ProcessScope()
	{
		if( !bInTransition )
			ePawn.HandItem.Scope();
	}

	function SwitchCameraMode()
	{
		m_camera.SetMode(ECM_Sniping);
	}

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = 0.3f; //(FovAngle from Laser mic / DesiredFOV) * 0.5;
	}

Begin:
	SwitchCameraMode();
	EMainHUD(myHud).Slave(ePawn.HandItem);
}

function bool CheckInAirHoist(vector locationEnd, rotator orientationEnd, name hoistSt, name hoistCr, vector hoistStOffset, vector hoistCrOffset)
{
	local vector HitLocation, HitNormal;

	m_HoistSt = hoistSt;
	m_HoistCr = hoistCr;
	m_HoistStOffset = hoistStOffset;
	m_HoistCrOffset = hoistCrOffset;

	if(	!bInTransition &&
		(m_LastLedgeFenceTime + 0.2 < Level.TimeSeconds) &&
		ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, true, ePawn.GetExtent(), , true) == None &&
		TryHoisting(locationEnd, orientationEnd, m_HoistStOffset, m_HoistCrOffset, m_HoistSt, m_HoistCr))
	{
		KillPawnSpeed();
		ePawn.SetPhysics(PHYS_Linear);
		GoToState(, 'LedgeHoist');
		return true;
	}
	return false;
}

native(1175) final function bool CheckWallJump(vector X, vector Y, vector Z);

// ----------------------------------------------------------------------
// state s_PlayerJumping
// ----------------------------------------------------------------------
state s_PlayerJumping
{
	Ignores Fire;

	function BeginState()
	{
		ePawn.m_climbingUpperHand = CHNONE;
		NormalIKFade();
		ePawn.SetPhysics(PHYS_Falling);
		bInTransition = false;
		m_camera.SetMode(ECM_Walking);
		ePawn.SoundWalkingRatio = 1.0;
	}

	function EndState()
	{
		m_NbJump = 0;
		m_LPStartTime = 0;
		m_LastLedgeFenceTime = 0;
		m_LastHOHTime = 0;
		m_LastNLPipeTime = 0;
		m_LastZipLineTime = 0;
		m_LPSlideStartTime = 0;
		m_FallGrabDir = vect(0,0,0);
	}

    function PlayerMove(float deltaTime)
    {
        local vector X,Y,Z, NewAccel, HitNormal;
		local rotator flatRot;
		local actor wall;

		if(bInTransition)
		{
			if(ePawn.m_climbingUpperHand != CHNONE)
			{
				ePawn.Acceleration = DampVec(ePawn.Acceleration, ePawn.m_locationEnd, VSize(ePawn.m_locationEnd) / ePawn.GetAnimTime('spltstalrt0'), deltaTime);
				ePawn.RotateTowardsRotator(ePawn.m_orientationEnd, 40000, 0.9);
			}
			return;
		}

		PawnLookAt();

		if(VSize(m_FallGrabDir) > 0.0 && ePawn.m_climbingUpperHand == CHNONE)
		{
			ePawn.Velocity.X = 0.0;
			ePawn.Velocity.Y = 0.0;
			ePawn.Acceleration.X = 0.0;
			ePawn.Acceleration.Y = 0.0;
			ePawn.RotateTowardsRotator(Rotator(m_FallGrabDir), 80000, 0.9);
		}

		flatRot = ePawn.Rotation;
		flatRot.Roll = 0;
		GetAxes(flatRot, X, Y, Z);

		// Update acceleration.
		SetPawnAccel(X, eGame.m_inAirAccel);

		// No turning when FallGrab, WallJump or SplitJump
		if(ePawn.m_climbingUpperHand == CHNONE && m_NbJump < 2)
			SetPawnRotation(20000);

		CheckForCrouch();
		ePawn.SetNormalZones(); // No Crouching zones in air

		// Look for a fence in front
		if(	ePawn.Velocity.Z < eGame.m_FMaxGrabbingSpeed &&
			ePawn.Velocity.Z > eGame.m_FMinGrabbingSpeed &&
			(m_LastLedgeFenceTime + 0.3 < Level.TimeSeconds))
		{
			if(ePawn.CheckFence(HitNormal, wall, ePawn.Location))
			{
				ePawn.m_validFence = true;
				ePawn.SetRotation(Rotator(-HitNormal));
				if ( wall.SurfaceType == SURFACE_FenceMetal )
					ePawn.PlaySound(Sound'FisherFoley.Play_Random_FisherFenceVib', SLOT_SFX);
				else if ( wall.SurfaceType == SURFACE_FenceVine )
					ePawn.PlaySound(Sound'FisherFoley.Play_Random_VineVibration', SLOT_SFX);
				ePawn.MakeNoise(500.0, NOISE_HeavyFootstep);
				GoToState('s_Fence', 'FromAir');
				ePawn.SetBase(wall, HitNormal);
				return;
			}
		}

		if(CheckWallJump(X, Y, Z))
			return;
		ePawn.PlayInAir();
    }

	event bool FeetLedge(vector locationEnd, rotator orientationEnd)
	{
		local name standing, crouch;
		local name CurrentAnimSeq;

		CurrentAnimSeq = ePawn.GetAnimName();

		if(CurrentAnimSeq == ePawn.AJumpForwardL)
		{
			standing = 'ledgstalftl';
			crouch = 'ledgcralftl';
		}
		else
		{
			standing = 'ledgstalftr';
			crouch = 'ledgcralftr';
		}
		return !bInTransition && !bInGunTransition && (m_FallGrabDir == vect(0,0,0)) &&
			   CheckInAirHoist(locationEnd, orientationEnd, standing, crouch, ePawn.m_HoistFeetOffset, ePawn.m_HoistFeetCrOffset);
	}

	event bool WaistLedge(vector locationEnd, rotator orientationEnd)
	{
		return !bInTransition && !bInGunTransition &&
			   CheckInAirHoist(locationEnd, orientationEnd, 'ledgstalqk0', 'ledgcralqk0', ePawn.m_HoistWaistOffset, ePawn.m_HoistWaistCrOffset);
	}

	event bool Ledge(vector locationEnd, rotator orientationEnd, int LType)
	{
		local vector handsPosition, handsSize, HitLocation, HitNormal;

		// We have a ledge in the arms zone.
		if(	!bInTransition &&
			(m_LastLedgeFenceTime + eGame.m_grabbingDelay < Level.TimeSeconds) &&	// dont grab twice too fast
			ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, true, ePawn.GetExtent(), , true) == None)
		{
			// Test the hands so they are not "in" an object
			handsSize.X = ePawn.CollisionRadius * 0.5;
			handsSize.Y = handsSize.X;
			handsSize.Z = (ePawn.m_LedgeGrabArmsZone.Z - ePawn.CollisionHeight) * 0.5;
			handsPosition.X = handsSize.X;
			handsPosition.Y = 0;
			handsPosition.Z = ePawn.CollisionHeight + handsSize.Z;
			handsPosition = locationEnd + (handsPosition >> orientationEnd);
			if(ePawn.FastPointCheck(handsPosition, handsSize, true, true))
			{
				GoToState('s_GrabbingLedge', 'Above');
				return true;
			}
		}
		return false;
	}

	event bool HandOverHand(vector locationEnd, rotator orientationEnd)
	{
		local vector HitLocation, HitNormal;
		// We have a hoh in the arms zone.
		if(	!bInTransition &&
			(ePawn.CollisionHeight == ePawn.default.CollisionHeight) &&
			(m_LastHOHTime + eGame.m_grabbingDelay < Level.TimeSeconds) &&	// dont grab twice too fast
			(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None))
		{
			GoToState('s_GrabbingHandOverHand', 'FromAir');
			return true;
		}
		return false;
	}

	event bool NarrowLadder(vector locationEnd, rotator orientationEnd, int NLType)
	{
		local vector HitLocation, HitNormal;
		// We have a narrowladder in the arms zone.
		if(	!bInTransition &&
			(m_LastNLPipeTime + eGame.m_grabbingDelay < Level.TimeSeconds) &&	// dont grab twice too fast
			(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None))
		{
			if(NLType == 0)
			{
				GoToState('s_GrabbingNarrowLadder', 'FromAir');
			}
			else if(NLType == 1)
			{
				GoToState('s_GrabbingNarrowLadder', 'FromUnder');
			}
			return true;
		}
		return false;
	}

	event bool Pipe(vector locationEnd, rotator orientationEnd)
	{
		local vector HitLocation, HitNormal;
		// We have a Pipe in the arms zone.
		if(	!bInTransition &&
			(m_LastNLPipeTime + eGame.m_grabbingDelay < Level.TimeSeconds) &&	// dont grab twice too fast
			(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None))
		{
			GoToState('s_GrabbingPipe', 'FromAir');
			return true;
		}
		return false;
	}

	event bool Pole(vector locationEnd, rotator orientationEnd, int RType)
	{
		local vector HitLocation, HitNormal;
		// We have a Pole in the arms zone.
		if(	(RType == 0 || RType == 2) &&
			!bInTransition &&
			(m_LastNLPipeTime + eGame.m_grabbingDelay < Level.TimeSeconds) &&	// dont grab twice too fast
			(ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, false, ePawn.GetExtent(), , true) == None))
		{
			GoToState('s_GrabbingPole', 'FromAir');
			return true;
		}
		return false;
	}

	event bool ZipLine(vector locationEnd, rotator orientationEnd, int ZLType)
	{
		local vector HitLocation, HitNormal;

		// We have a ZipLine in the arms zone.
		if(	!bInTransition &&
			((m_LastZipLineTime * 2.0) + eGame.m_grabbingDelay < Level.TimeSeconds) &&	// dont grab twice too fast
			ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, true, ePawn.GetExtent(), , true) == None)
		{
			GoToState('s_GrabbingZipLine', 'FromAir');
			return true;
		}
		return false;
	}

	// Exit Condition for this state
    event bool NotifyLanded(vector HitNormal, Actor HitActor)
    {
		local float damage;
		local EPawn other;

		m_LPStartTime = 0.0;
		m_NbJump = 0;
		m_LastLedgeFenceTime = 0;			// Reset all geo timers just to be safe
		m_LastHOHTime = 0;				//
		m_LastNLPipeTime = 0;				//
		m_LastZipLineTime = 0;			//

		/* ********************************************
		Damage processing

		Quick log:

		From 400cm .. Normal=-1118, Jump=-1222, Grab=-802
		From 600cm .. Normal=-1344, Jump=-1423, Grab=-1092
		From 800cm .. Normal=-1544, Jump=-1629, Grab=
		*********************************************** */

		other = EPawn(HitActor);
			if(abs(ePawn.Velocity.Z) >= eGame.MinBeforeDamage)
			{
				damage = (Abs(ePawn.Velocity.Z) - eGame.MinBeforeDamage) / (eGame.MaxBeforeDeath-eGame.MinBeforeDamage) * ePawn.default.Health;
			if(other != None)
				damage /= eGame.NPCCushionDivider;

				ePawn.TakeDamage(damage, ePawn, ePawn.Location, HitNormal, ePawn.Velocity, class'Crushed');
				if( CurrentVolume == None || !CurrentVolume.bLiquid )
				{
					ePawn.PlaySound(Sound'FisherVoice.Play_Random_FisherHitGeometry', SLOT_SFX);
					ePawn.PlaySound(Sound'FisherFoley.Play_FisherAnkleCreak', SLOT_SFX);
				ePawn.MakeNoise(200.0, NOISE_LightFootstep);
			}
				}

		if(other != None)
			other.TakeDamage(other.Health / 2, ePawn, other.Location, HitNormal, ePawn.Velocity, class'EKnocked', EPawn.P_Head);

				if( ePawn.Health <= 0 )
					return true;

		//Log("Notify Landed");

		// Handle landing animation here to prevent losing velocity values
		if( ShouldRoll() )
		{
			bInTransition = true;
			GotoState('s_Roll', 'DoRoll');
			return true;
		}

		bInTransition = true;
		if( ePawn.PlayLanding() )
			GotoState(,'Land');
		else
			GotoState(,'FinishNoLand');

        return true;           // do not pass notification to pawn
    }

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		if(bInTransition && ePawn.Physics == PHYS_Linear)
		{
			ePawn.Acceleration = 0.5 * ((ePawn.Acceleration dot HitNormal) * HitNormal * (-2.0) + ePawn.Acceleration);
			ePawn.Velocity = ePawn.Acceleration;
			return true;
		}
		return false;
	}

	function bool ShouldRoll()
	{
		local float forward_vel;
		forward_vel = ePawn.ToLocalDir(ePawn.Velocity).X;
		//Log("ePawn.Velocity"@ePawn.Velocity@"=="@VSize(ePawn.Velocity));
		//Log("ePawn.Acceleration"@ePawn.Acceleration@"=="@VSize(ePawn.Acceleration));

		return VSize(ePawn.Acceleration) > 0 &&
			   ePawn.bWantsToCrouch &&
			   forward_vel > 200.0 &&
			   ePawn.Velocity.Z < -400.0 &&
			   IsPushingGentle();
	}

LedgeHoist:
	bInTransition = true;
	m_LastLedgeFenceTime = Level.TimeSeconds;
	if(ePawn.bWantsToCrouch)
		ePawn.PlayAnimOnly(m_HoistCr, , 0.2);
	else
		ePawn.PlayAnimOnly(m_HoistSt, , 0.2);
	hoistTargetRot = ePawn.m_orientationEnd;
	ePawn.m_orientationEnd = ePawn.m_orientationStart;
	MoveToDestination(ArrivalSpeedApprox(2.0));
	bInTransition = false;
	if(TryHoisting(ePawn.Location, hoistTargetRot, m_HoistStOffset, m_HoistCrOffset, m_HoistSt, m_HoistCr))
		GoToState('s_HoistingAboveLedge', 'NoPlay');
	ePawn.SetPhysics(PHYS_Falling);
	Stop;

PushSide:
	bInTransition = true;
	ePawn.SetPhysics(PHYS_Linear);
	if(ePawn.m_climbingUpperHand == CHRIGHT)
		ePawn.PlayAnimOnly('spltstalrt0', 1.0, 0.05);
	else
		ePawn.PlayAnimOnly('spltstallt0', 1.0, 0.05);
	FinishAnim(0);
	ePawn.SetPhysics(PHYS_Falling);
	ePawn.Velocity = ePawn.Acceleration;
	ePawn.LoopAnimOnly(ePawn.AFall, , 0.3);
	ePawn.m_climbingUpperHand = CHNONE;
	bInTransition = false;
	ePawn.SetRotation(ePawn.m_orientationEnd);
	Stop;

SlipSide:
	bInTransition = true;
	ePawn.SetPhysics(PHYS_None);
	if(ePawn.m_climbingUpperHand == CHRIGHT)
		ePawn.PlayAnimOnly('spltstalrt0', 1.0, 0.05);
	else
		ePawn.PlayAnimOnly('spltstallt0', 1.0, 0.05);
	Sleep(ePawn.GetAnimTime('spltstallt0') * 0.4);
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Falling);
	ePawn.LoopAnimOnly(ePawn.AFall, , 0.2);
	ePawn.m_climbingUpperHand = CHNONE;
	bInTransition = false;
	Stop;

Land:
	ePawn.m_climbingUpperHand = CHNONE;
	KillPawnSpeed();
	FinishAnim();
	Goto('Finish');
FinishNoLand:
	ePawn.m_climbingUpperHand = CHNONE;
	PlayOnGround(0.1);
	Goto('Finish');
Finish:
	bInTransition = false;
	if( IsPushingGentle() )
	{
		prevWalkingRatio = 1.0;
		ePawn.WalkingRatio = prevWalkingRatio;
		ePawn.SoundWalkingRatio = prevWalkingRatio;
	}
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_Throw
// ----------------------------------------------------------------------
state s_Throw
{
	function BeginState()
	{
		ePawn.HandItem.Use();
		m_ThrowSpeed = m_ThrowMinSpeed;
		FastIKFade();
		bInTransition = false;
		bThrowTargeting = true;
		ePawn.WeaponStance = 1;
		ePawn.SwitchAnims();
		m_camera.SetMode(ECM_Throw);
	}

	function EndState()
	{
		NormalIKFade();
		CheckHandObject();
		bThrowTargeting = false;
		ePawn.PlayAnim('throStAlEdH',0,0,EPawn.ACTIONCHANNEL);
		ePawn.AnimBlendToAlpha(EPawn.ACTIONCHANNEL, 0.0, 0.2);
	}

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = m_turnMul;
	}

	event MayFall()
	{
		GotoState('s_PlayerWalking');
	}

	function ProcessFire()
	{
		if(!bInTransition)
			GotoState(, 'Throw');
	}

	function ProcessScope()
	{
		if(!bInTransition)
			GotoState('s_PlayerWalking');
	}

	function PlayerMove( float DeltaTime )
	{
        local vector X,Y,Z, NewAccel;
		local float pushingForce;

		pushingForce = GetPushingForce();

		ePawn.AimAt(AAVERT, Vector(Rotation), Vector(ePawn.Rotation), 0, 0, -60, 80);

		if(bAltFire != 0)
			m_ThrowSpeed = DampVec(m_ThrowSpeed, m_ThrowMaxSpeed, m_ThrowVarSpeed, DeltaTime);
		else
			m_ThrowSpeed = DampVec(m_ThrowSpeed, m_ThrowMinSpeed, m_ThrowVarSpeed, DeltaTime);

		if(bInTransition)
			return;

        GetAxes(Rotation,X,Y,Z);

		NewAccel            = aForward*X + aStrafe*Y;
		NewAccel.Z          = 0;
		ePawn.Acceleration  = NewAccel;
		ePawn.Acceleration *= eGame.m_onGroundAccel;

		ePawn.WalkingRatio = VSize(ePawn.Velocity / GetGroundSpeed());

		CheckForCrouch();

		bThrowTargeting = true;

		PlayOnGroundSniping((pushingForce > 0.3), pushingForce * GetGroundSpeed(), false);

		ePawn.AnimBlendParams(ePawn.ACTIONCHANNEL, 1, 0.0, 0.0, ePawn.UpperBodyBoneName);

		if(ePawn.bIsCrouched)
			ePawn.LoopAnim('throCrAlNtH', 1, 0.3, ePawn.ACTIONCHANNEL);
		else
			ePawn.LoopAnim('throStAlNtH', 1, 0.3, ePawn.ACTIONCHANNEL);
	}

	event NotifyAction()
	{
		ePawn.HandItem.Throw(self, (m_ThrowSpeed >> Rotation));
	}

Throw:
	bThrowTargeting = false;
	bInTransition = true;
	KillPawnSpeed();
	if(ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('throCrAlEdH', 1.0 + 0.5 * ((VSize(m_ThrowSpeed) - VSize(m_ThrowMinSpeed)) / (VSize(m_ThrowMaxSpeed) - VSize(m_ThrowMinSpeed))),0.05);
	else
		ePawn.PlayAnimOnly('throStAlEdH', 1.0 + 0.5 * ((VSize(m_ThrowSpeed) - VSize(m_ThrowMinSpeed)) / (VSize(m_ThrowMaxSpeed) - VSize(m_ThrowMinSpeed))),0.05);
	FinishAnim();
	bInTransition = false;
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_InteractWithObject (BASE STATE)
// ----------------------------------------------------------------------
state s_InteractWithObject
{
	function BeginState()
	{
        NormalIKFade();

		KillPawnSpeed();

		if(ePawn.bIsCrouched)
			m_camera.SetMode(ECM_WalkingCr);
		else
			m_camera.SetMode(ECM_Walking);
	}

	function EndState()
	{
		bNoLedgePush = false;
		if( Interaction != None )
			Interaction.PostInteract(self);
	}

	function ProcessFire();

	event NotifyAction()
	{
		// Might happen in open door for a backward anim at the end doesn't need to call this
		if( Interaction != None )
			Interaction.Interact(self);
	}

    event bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
    {
        Super.KeyEvent(Key, Action, Delta);

		// Prevent
        if( Interaction != None && Level.Pauser == None )
            Interaction.KeyEvent(Key, Action, Delta);

        return false;
    }

	event bool NotifyLanded(vector HitNormal, actor Wall)
	{
		// We cannot reach destination without collision... give up
		GoToState('s_PlayerWalking');
		return true;
	}

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		// We cannot reach destination without collision... give up
		GoToState('s_PlayerWalking');
		return true;
	}
}

state s_Conversation extends s_InteractWithObject
{
	function BeginState()
	{
		Super.BeginState();
		if( !ePawn.bIsCrouched )
			ePawn.LoopAnimOnly(ePawn.AWait,,0.2);
		else
			ePawn.LoopAnimOnly(ePawn.AWaitCrouch,,0.2);
	}
	function EndState();

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function bool CanInteract()
	{
		return true;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		// Allow only current conversation
		return IntObj.class.name == 'ENpcZoneInteraction' &&
			   IntObj.GetStateName() == 's_NpcTalking' &&
			   ENpcZoneInteraction(IntObj).Epc == self;
	}

	function PlayerMove( float DeltaTime )
	{
		// Always look toward Npc while talking
		if( Interaction != None )
			ePawn.RotateTowardsRotator(Rotator(Interaction.Owner.Location - ePawn.Location));
	}
}

// ----------------------------------------------------------------------
// state s_Turret
// ----------------------------------------------------------------------
state s_Turret extends s_InteractWithObject
{
	event bool CanSaveGame()
	{
		return false;
	}

	function bool CanAccessQuick()
	{
		return false;
	}

    function bool KeyEvent( string Key, EInputAction Action, FLOAT Delta )
	{
		if( !bInTransition )
			return Super.KeyEvent(Key, Action, Delta);
	}

Begin:
	// Move pawn over object
	bInTransition = true;
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Walking);

	// Check if needs to crouch
	if( !ePawn.bIsCrouched )
	{
		ePawn.bWantsToCrouch = true;
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn, ,0.1);
		FinishAnim();
	}

	// Begin
	ePawn.PlayAnimOnly('kbrdcralbg0',,0.05);
	FinishAnim();
	bInTransition = false;

	// Pop interface
	EMainHUD(myHud).hud_master = EGameplayObject(Interaction.Owner);
	FakeMouseToggle(true);
	EMainHUD(myHud).hud_master.BeginEvent();

	ePawn.PlaySound(Sound'Electronic.Play_Sq_ComputerKeyBoard', SLOT_SFX);
	ePawn.AddSoundRequest(Sound'Electronic.Stop_Sq_ComputerKeyBoard', SLOT_SFX, 2.0f);

	// Loop
	ePawn.LoopAnimOnly('kbrdcralnt0',,0.05);
	Stop;

End:
	// Remove interface
	EMainHUD(myHud).hud_master.EndEvent();
	FakeMouseToggle(false);
	EMainHUD(myHud).hud_master = None;

	ePawn.PlaySound(Sound'Electronic.Stop_Sq_ComputerKeyBoard', SLOT_SFX);

	ePawn.PlayAnimOnly('kbrdcralbg0',,0.1,true);
	FinishAnim();

	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_PickUp
// ----------------------------------------------------------------------
state s_PickUp extends s_InteractWithObject
{
	event bool NotifyHitWall( Vector hitNormal, Actor Wall )
	{
		if( !bInTransition )
			GotoState(,'Interrupted');
		return true;
	}
	event bool NotifyLanded(vector HitNormal, actor Wall)
	{
		return NotifyHitWall(HitNormal, Wall);
	}

Begin:
	// Move pawn over object
	bNoLedgePush = true;
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Walking);

Interrupted:
	bInTransition = true;

	// Drop handItem within cylinder with no extra velocity
	DiscardPickup(,true);

	if( JumpLabel == '' )
		Log("ERROR : JumpLabel should be defined in PickUp interaction");
	Goto(JumpLabel);

PickupHigh:
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('PickStAlFd0', ,0.1);
	else
		ePawn.PlayAnimOnly('PickCrAlFd0', ,0.1);
	goto('Finish');

PickupLow:
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('PickStAlUp0', ,0.1);
	else
		ePawn.PlayAnimOnly('PickCrAlUp0', ,0.1);
	goto('Finish');

Finish:
	FinishAnim();
	bInTransition = false;
	GotoState('s_PlayerWalking');
}

state s_PushNPCOffChair
{
	Ignores AltFire, Fire, ProcessHeadSet;

	function PlayerMove(float DeltaTime)
	{
		ePawn.RotateTowardsRotator(Rotator(Normal(m_AttackTarget.Location - ePawn.Location)));
	}

Begin:
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Walking);
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('PickStAlFd0', 1.5, 0.1, true);
	else
		ePawn.PlayAnimOnly('PickCrAlFd0', 1.5, 0.1, true);
	FinishAnim();
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_KeyPadInteract
// ----------------------------------------------------------------------
state s_KeyPadInteract extends s_InteractWithObject
{
	event bool CanSaveGame()
	{
		return false;
	}

	function BeginState()
	{
		ePawn.SetPhysics(PHYS_None);
		ePawn.PlayWait(0.0, 0.1);

		Super.BeginState();
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function bool CanAccessQuick()
	{
		return false;
	}
}

// ----------------------------------------------------------------------
// state s_Computer
// ----------------------------------------------------------------------
state s_Computer extends s_InteractWithObject
{
	Ignores EndState;

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		//Disable('NotifyHitWall');
		//bInTransition = false;
		//GoToState(,'DoInteract');
		return true;
	}
	event bool NotifyLanded(vector HitNormal, actor Wall)
	{
		return NotifyHitWall(HitNormal, Wall);
	}

	function PlayerMove( float DeltaTime )
	{
		// if !bInTransition, means collision interrupted, make it go to rotation
		if( !bInTransition && Interaction != None )
			ePawn.RotateTowardsRotator(Rotator(Interaction.Owner.Location - ePawn.Location));
	}

Begin:
	//bInTransition = true;
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.PlayMoveTo(Rotation);
	MoveToDestination(300,true);

DoInteract:
	if( ePawn.bIsCrouched )
	{
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn, 2,,true);
		FinishAnim();
	}
	ePawn.PlayAnimOnly('kbrdstalbg0',,0.1);
	FinishAnim();

	Interaction.Interact(self);

	ePawn.LoopAnimOnly('kbrdstalnt0');
	Sleep(1.5);

	Interaction.PostInteract(self);
	ePawn.PlayAnimOnly('kbrdstalbg0',,,true);
	FinishAnim();

	if( ePawn.bIsCrouched )
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn);
	// No FinishAnim to be able to start moving

	bInTransition = false;
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_OpticCable
// ----------------------------------------------------------------------
state s_OpticCable extends s_InteractWithObject
{
	Ignores ProcessHeadset;

	function EndState()
	{
		Exit();
		Super.EndState();
	}
	function Exit()
	{
		EMainHUD(myHud).NormalView();
		bLockedCamera	= false;
		// If exiting because of death, remove control
		if( ePawn.HandItem.GetStateName() != 's_Selected' )
		{
			ePawn.HandItem.GotoState('s_Selected');
			SetRotation(backupRotation);
		}
	}

	function ProcessFire()
	{
		if( !bInTransition )
			Global.ProcessFire();
	}

begin:
	bInTransition = true;
	if( !ePawn.bIsCrouched )
	{
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn, , 0.1);
		FinishAnim();
	}
	ePawn.LoopAnimOnly(ePawn.AWaitCrouch, , 0.1);

	ePawn.BlendAnimOverCurrent('LockStAlNt0',1,ePawn.UpperBodyBoneName,1,0.2);
	Sleep(ePawn.GetAnimtime('LockStAlNt0') * 0.5);
	backupRotation	= Rotation;
	bLockedCamera	= true;
	bInTransition	= false;
	ePawn.HandItem.GotoState('s_Sneaking');
	Stop;

End:
	bInTransition	= true;
	SetRotation(backupRotation);
	Exit();

	ePawn.BlendAnimOverCurrent('LockStAlNt0',1,ePawn.UpperBodyBoneName,1,0.2);
	Sleep(ePawn.GetAnimtime('LockStAlNt0') * 0.5);
	if( !ePawn.bIsCrouched )
	{
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn,,,true);
		FinishAnim();
	}
	bInTransition = false;
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_PickLock
// ----------------------------------------------------------------------
state s_PickLock extends s_InteractWithObject
{
	Ignores NotifyHitWall; // Make sure pick lock always work (often with large door frame)

	function PlayerMove( float DeltaTime )
	{
		// Give Axis to interaction, move camera
		Interaction.ProcessAxis(self, aForward, aStrafe, aTurn, aLookUp);
	}

	function ProcessFire()
	{
		if( Interaction != None )
			Interaction.PostInteract(self);
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function ProcessToggleCinematic()
	{
		// Abort and remove lock pick interface
		Global.ProcessToggleCinematic();
		GotoState('s_PlayerWalking');
	}

begin:
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.PlayMoveTo(Rotation);
	MoveToDestination(300, true);
	ePawn.PlayWait(0.0, 0.1);
	ePawn.SetPhysics(PHYS_Walking);
	ePawn.LoopAnimOnly('LockStAlNt0',,0.2);
	Interaction.InitInteract(self);
}

state s_BreakLock extends s_InteractWithObject
{
	// Don't interact(thus spending a lock breaker)
	// if destination can't be reached
	function MoveToDestinationFailed()
	{
		GoToState(,'Aborted');
	}

Begin:
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.PlayMoveTo(Rotation);
	MoveToDestination(200, true);

Aborted:
	ePawn.PlayWait(0.0, 0.1);
	ePawn.SetPhysics(PHYS_Walking);

	ePawn.PlayAnimOnly('LockStAlNt0',,0.2);
	FinishAnim();

	Interaction.Interact(self);

	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_PlaceWallMine
// ----------------------------------------------------------------------
state s_PlaceWallMine extends s_InteractWithObject
{
	function NotifyAction()
	{
		// Unhide item in hand from animation
		ePawn.HandItem.bHidden = !bInTransition;
	}

Begin:
	// Make sure to not call any obsolete interaction
	Interaction = None;

	bNoLedgePush = true;
	ePawn.SetPhysics(PHYS_None);

	ePawn.PlaySound(Sound'FisherEquipement.Play_WallMineFix', SLOT_SFX);

	// Should begin to test whether or not there's a valid surface
	ePawn.HandItem.GotoState('s_PawnPlacement');

	// Begin
	bInTransition = true;
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('minestalbg0',,0.1);
	else
		ePawn.PlayAnimOnly('minecralbg0',,0.1);
	FinishAnim();
	ePawn.HandItem.GotoState('s_PawnPlacement','QuickCheck');

	// Loop
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('minestalnt0');
	else
		ePawn.PlayAnimOnly('minecralnt0');
	FinishAnim();
	ePawn.HandItem.GotoState('s_PawnPlacement','PlaceOnWall');

	// End
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('minestaled0');
	else
		ePawn.PlayAnimOnly('minecraled0');
	Goto('Finish');

Aborted:
	bInTransition = false;
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('minestalbg0',,0.1, true, ePawn.GetAnimName()=='minestalbg0');
	else
		ePawn.PlayAnimOnly('minecralbg0',,0.1, true, ePawn.GetAnimName()=='minecralbg0');

Finish:
	FinishAnim();
	bInTransition = false;
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_DisableWallMine
// ----------------------------------------------------------------------
state s_DisableWallMine extends s_InteractWithObject
{
	Ignores NotifyAction;

	function BeginState()
	{
		bNoLedgePush = true;
		bInTransition = true;
		Super.BeginState();
	}

	function MoveToDestinationFailed()
	{
		GoToState(,'Aborted');
	}

	function PlayerMove( float DelaTime )
	{
		Super.PlayerMove(DelaTime);

		if( bInTransition || Interaction == None )
			return;

		// Check if wallmine wouldn't be on a moving surface
		if( Interaction.Owner.Location != m_HoistStOffset )
			GotoState(,'Aborted');
	}

Begin:
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Walking);

	// Keep track of wallmine distance
	m_HoistStOffset = Interaction.Owner.Location;
	bInTransition = false;

	if( JumpLabel == '' )
		Log("ERROR : JumpLabel should be defined in DisableWallMine interaction");
	Goto(JumpLabel);

DefuseCrouch:
	// Check if needs to crouch
	if( !ePawn.bIsCrouched )
	{
		ePawn.bWantsToCrouch = true;
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn, ,0.1);
		FinishAnim();
	}
	Goto('Defuse');

DefuseStand:
	// Check if needs to uncrouch
	if( ePawn.bIsCrouched )
	{
		ePawn.bWantsToCrouch = false;
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn,,0.1,true);
		FinishAnim();
	}
	Goto('Defuse');

Defuse:
	// Begin
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('minestaled0',,0.1, true);
	else
		ePawn.PlayAnimOnly('minecraled0',,0.1, true);
	FinishAnim();

	Interaction.Owner.PlaySound(Sound'FisherEquipement.Play_WallMineRemove', SLOT_SFX);

	// Cycle
	if( !ePawn.bIsCrouched )
		ePawn.LoopAnimOnly('minestalnt0');
	else
		ePawn.LoopAnimOnly('minecralnt0');
	Sleep(2.0);
	bInTransition = true;
	Interaction.Interact(self);
	Stop;

SheathMine:
	// End
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('minestalbg0',,, true);
	else
		ePawn.PlayAnimOnly('minecralbg0',,, true);
	FinishAnim();

Aborted:
	JumpLabel = '';
	bInTransition = false;
	GotoState('s_PlayerWalking');
}


// ----------------------------------------------------------------------
// state s_RetinalScanner
// ----------------------------------------------------------------------
state s_RetinalScanner extends s_InteractWithObject
{
begin:
	Interaction.SetInteractLocation(ePawn);
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(200, true);
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_Walking);
	ePawn.PlayAnimOnly(ePawn.ARetinalScanBegin,,0.2);
	FinishAnim();
	Interaction.Interact(self);
	ePawn.LoopAnimOnly(ePawn.ARetinalScan,,0.0);
	Stop;

RetinalEnd:
	ePawn.PlayAnimOnly(ePawn.ARetinalScanEnd,,0.0);
	FinishAnim();
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_OpenDoor
// ----------------------------------------------------------------------
state s_OpenDoor extends s_InteractWithObject
{
	function EndState()
	{
		Super.EndState();
		Interaction = None;
	}

	function bool NotifyBump( Actor Other, optional int Pill )
	{
		if( Other.bIsPawn )
			MoveToDestinationFailed();
		return false;
	}

	// s_OpenDoor should only complet if Sam reached destination.
	function MoveToDestinationFailed()
	{
		bInTransition = false;
		// Remove interaction to prevent calling PostInteract on it
		Interaction = None;
		GoToState('s_PlayerWalking');
	}

LockedRt:
	ePawn.AddSoundRequest(Sound'Door.Play_DoorJam', SLOT_SFX, 0.4f );
	JumpLabelPrivate = 'JmpLockedRt';
	Goto('DoDoor');
LockedLt:
	ePawn.AddSoundRequest(Sound'Door.Play_DoorJam', SLOT_SFX, 0.4f );
	JumpLabelPrivate = 'JmpLockedLt';
	Goto('DoDoor');
UnLockedRt:
	JumpLabelPrivate = 'JmpUnLockedRt';
	Goto('DoDoor');
UnLockedLt:
	JumpLabelPrivate = 'JmpUnLockedLt';
	Goto('DoDoor');

DoDoor:
	bInTransition = true;
	Interaction.SetInteractLocation( EPawn );
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.PlayMoveTo(Rotation);
	MoveToDestination(300, true);

	if( ePawn.bIsCrouched )
	{
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn, 2,,true);
		FinishAnim();
	}
	Goto(JumpLabelPrivate);

JmpLockedLt:
	ePawn.PlayAnimOnly('DoorStAlBgl');
	FinishAnim();
	ePawn.PlayAnimOnly('DoorStAlBgl',,, true);
	Goto('EndState');

JmpLockedRt:
	ePawn.PlayAnimOnly('DoorStAlBgr');
	FinishAnim();
	NotifyAction();
	ePawn.PlayAnimOnly('DoorStAlBgr',,, true);

	Goto('EndState');

JmpUnLockedLt:
	ePawn.PlayAnimOnly('DoorStAlBgl');
	FinishAnim();
	NotifyAction();
	ePawn.PlayAnimOnly('DoorStAlEdl');
	Goto('EndState');

JmpUnLockedRt:
	ePawn.PlayAnimOnly('DoorStAlBgr');
	FinishAnim();
	NotifyAction();
	ePawn.PlayAnimOnly('DoorStAlEdr');

	Goto('EndState');

EndState:
	JumpLabelPrivate = '';
	FinishAnim();
	if( ePawn.bIsCrouched )
		ePawn.PlayAnimOnly(ePawn.AWaitCrouchIn);
	// No FinishAnim to be able to start moving

	ePawn.SetPhysics(PHYS_Walking);
	bInTransition = false;

GetOut:
	// Check if coming from first person
	if( JumpLabel == 'BackToFirstPerson' )
	{
		JumpLabel = '';
		bInTransition = true;
		GotoState('s_FirstPersonTargeting');
	}
	else
		GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_OpenDoor
// ----------------------------------------------------------------------
state s_OpenDoorStealth extends s_OpenDoor
{
	function BeginState()
	{
		m_SMInTrans = false;
		Super.BeginState();
		bInTransition = true;
	}

	// No call to parent in case we want to finish opening door.
	function EndState()
	{
		if(m_SMInTrans)
		{
			m_camera.StopFixeTarget();
			bInTransition = false;
		}
		m_GrabTargeting = false;
		// Hack .. prevent having the door blocked for Npc in case we couldn't do the Interact() call
		EDoorMover(Interaction.Owner.Owner).bInUse = false;
		// Hack .. prevent having the Interaction set to something when we get out
		Interaction = None;
	}

	// Own processing in EndState
	function MoveToDestinationFailed()
	{
		bInTransition = false;
		GoToState('s_PlayerWalking');
	}

	// Abort if mover can not make it to 1st key, called from ESwingingDoor
	function EndEvent()
	{
		// Use this to prevent NotifyAction of being called if door movement is cancelled
		m_GrabTargeting = true;
		Interaction.KeyEvent("Interrupted", IST_Press, 1);
	}

	function NotifyAction()
	{
		if( !m_GrabTargeting )
			Super.NotifyAction();
	}

	function PlayerMove( float DeltaTime )
	{
		if( bInTransition ) return;

		// Pushing forward open door
		if( aForward == 0 )
		{
			Interaction.KeyEvent("Neutral", IST_Hold, 1);
		}
		else if( aForward > eGame.m_fullForce )
		{
			Interaction.KeyEvent("Forward", IST_Press, 1);
		}
		// Pushing back close door
		else if( aForward < -eGame.m_fullForce )
		{
			Interaction.KeyEvent("Backward", IST_Press, 1);
		}
	}

BeginStealth:
	bInTransition = true;
	Interaction.SetInteractLocation( EPawn );
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.PlayMoveTo(Rotation);
	MoveToDestination(300, true);

	// force uncrouch
	ePawn.bWantsToCrouch = false;

	// Play turn on Door
	if( m_BTWSide )
		ePawn.PlayAnimOnly('doorstpkbgr',,0.2);
	else
		ePawn.PlayAnimOnly('doorstpkbgl',,0.2);

	// Finish turn
	FinishAnim();
	if( m_BTWSide )
		m_camera.SetMode(ECM_DoorPeekRight);
	else
		m_camera.SetMode(ECM_DoorPeekLeft);

	// Loop wait on door
	if( m_BTWSide )
		ePawn.LoopAnimOnly('doorstpkntr');
	else
		ePawn.LoopAnimOnly('doorstpkntl');

	bInTransition = false;
	Stop;

// Push door open
OpenDoor:
	bIntransition = true;
	m_SMInTrans = true;
	m_camera.SetMode(ECM_Walking);
	m_camera.SetFixeTarget(ePawn.ToWorld(2*ePawn.CollisionRadius*Vect(1,0,0)), (2*ePawn.CollisionRadius) / ePawn.GetAnimTime('doorstpkedr'));
	if( m_BTWSide )
		ePawn.PlayAnimOnly('doorstpkedr',,,);
	else
		ePawn.PlayAnimOnly('doorstpkedl',,,);
	FinishAnim();
	ePawn.SetLocation(ePawn.ToWorld(2*ePawn.CollisionRadius*Vect(1,0,0)));
	ePawn.PlayAnimOnly(ePawn.AWait);
	m_camera.StopFixeTarget();
	m_SMInTrans = false;
	bInTransition = false;
	ePawn.SetPhysics(PHYS_Walking);
	Goto('GetOut');

// Interrupted while opening (bump object / bump pawn)
InterruptOpening:
	bIntransition = true;
	m_camera.StopFixeTarget();
	m_camera.SetMode(ECM_Walking);
	if( m_BTWSide )
		ePawn.PlayAnimOnly('doorstpkedr',,,true,true);
	else
		ePawn.PlayAnimOnly('doorstpkedl',,,true,true);
	FinishAnim();
	bInTransition = false;
	// This one will continue at frame and play inverted, not the next one.
	m_GrabTargeting = false;

InterruptStealth:
CloseDoor:
	bInTransition = true;
	m_camera.SetMode(ECM_Walking);
	if( m_BTWSide )
		ePawn.PlayAnimOnly('doorstpkbgr',,0.05, true, m_GrabTargeting);
	else
		ePawn.PlayAnimOnly('doorstpkbgl',,0.05, true, m_GrabTargeting);
	FinishAnim();
	bInTransition = false;
	ePawn.SetPhysics(PHYS_Walking);
	Goto('GetOut');
}

// ----------------------------------------------------------------------
// state s_OpenWindow
// ----------------------------------------------------------------------
state s_OpenWindow extends s_InteractWithObject
{
	function BeginState()
	{
		Super.BeginState();

		Interaction.SetInteractLocation(ePawn);
		ePawn.SetPhysics(PHYS_Linear);
		ePawn.PlayMoveTo(Rotation);
	}

High:
	MoveToDestination(300, true);
	ePawn.PlayAnimOnly('windstalup0',,0.2);
	Goto('EndState');

Low:
	ePawn.bWantsToCrouch = true; // will be crouched after animation
	MoveToDestination(300, true);
	ePawn.PlayAnimOnly('windcralup0',,0.2);
	Goto('EndState');

Trap:
	MoveToDestination(200, true);
	if( !ePawn.bIsCrouched )
		ePawn.PlayAnimOnly('trapstalup0', ,0.1);
	else
		ePawn.PlayAnimOnly('trapcralup0', ,0.1);
	Goto('EndState');

EndState:
	FinishAnim();
	GotoState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_BreakObject
// ----------------------------------------------------------------------
state s_BreakObject
{
	Ignores Fire;

	function BeginState()
	{
		if( m_targetObject == None )
			GotoState('s_PlayerWalking');
		KillPawnSpeed();
	}

	function EndState()
	{
		m_targetObject = None;
		JumpLabelPrivate = '';
	}

	function PlayerMove( float DeltaTime )
	{
		PawnLookAt();
	}

	event NotifyAction()
	{
		local vector HitNormal;
		HitNormal = Vector(ePawn.Rotation);
		m_targetObject.TakeDamage(100, ePawn, m_targetObject.Location, HitNormal, HitNormal * 500, None, 69);
	}

Begin:
	ePawn.PlayAnimOnly('AtakStAlFd0',,0.2);
	Goto('End');
End:
	FinishAnim();
	GotoState('s_PlayerWalking');
}

state s_ShortRangeAttack extends s_BreakObject
{
	function BeginState()
	{
		m_targetObject = m_AttackTarget;
		Super.BeginState();
	}

	function PlayerMove( float DeltaTime )
	{
		ePawn.RotateTowardsRotator(Rotator(m_AttackTarget.Location - ePawn.Location));
		m_AttackTarget.Velocity = vect(0,0,0);
		ePawn.Velocity = vect(0,0,0);

		Super.PlayerMove(DeltaTime);
	}

	event NotifyAction()
	{
		local vector normal;
		local bool AttackingSomething;
		normal = Vector(ePawn.Rotation);

		if ( EAIController(m_AttackTarget.Controller) != None )
			AttackingSomething = ((EAIController(m_AttackTarget.Controller).m_pGoalList.GetCurrent().m_GoalType == GOAL_Attack) && ((EAIController(m_AttackTarget.Controller).m_pGoalList.GetCurrent().GoalTarget) != (EchelonGameInfo(Level.Game).pPlayer.Pawn)));
		else
			AttackingSomething = false;

		// Check JumpLabelPrivate for front/back attack
		if( JumpLabelPrivate == 'front' && m_AttackTarget.bHostile && !EAIPawn(m_AttackTarget).AI.Pattern.IsInState('Hide') && !AttackingSomething )
			m_AttackTarget.TakeDamage(m_AttackTarget.InitialHealth/2, ePawn, m_AttackTarget.Location, -normal, normal, class'EKnocked');
		else
			m_AttackTarget.TakeDamage(m_AttackTarget.InitialHealth/2, ePawn, m_AttackTarget.Location, -normal, normal, class'EKnocked', EPawn.P_Head);
	}
}

state s_PlayerBTWBase
{
	function BeginState()
	{
		ePawn.bBatmanHack = true;
		bVisibilityFromCylinder = true;
		ePawn.SoundWalkingRatio = 0.5f;
	}

	function EndState()
	{
		ePawn.bBatmanHack = false;
		bVisibilityFromCylinder = false;
	}

	function ProcessBackToWall()
	{
		if(!bIntransition && CheckBTWRelease())
		{
			JumpLabel = '';
			GotoState(, 'Release');
		}
	}

	function ProcessScope()
	{
		if(!bIntransition && CheckBTWRelease() && ActiveGun != None)
		{
			JumpLabel = 'Scope';
			GotoState(, 'Release');
		}
	}
}

function StickBTW()
{
	local vector horiNorm;

		horiNorm = -m_WallHitNormal;
		horiNorm.Z = 0;
	horiNorm = Normal(horiNorm);

	if(!ePawn.m_LegdePushing && (horiNorm dot Vector(ePawn.Rotation)) > 0.0)
	{
		ePawn.bBatmanHack = false;
		ePawn.SetRotation(InterpolateRotator(0.5, Rotator(horiNorm), ePawn.Rotation));
		ePawn.Move(-horiNorm*0.1);
		ePawn.Move(horiNorm);
		ePawn.bBatmanHack = true;
	}
}

state s_PlayerBTW extends s_PlayerBTWBase
{
	Ignores Fire;

    function PlayerMove(float deltaTime)
    {
		StickBTW();

		if(bIntransition || CheckForCrouchBTW())
			return;

		if(!CheckBTW() && CheckBTWRelease())
		{
			GotoState(, 'Release');
		}
		else if(Abs(aStrafe) > eGame.m_gentleForce)
		{
			if(aStrafe > eGame.m_gentleForce)
				m_BTWSide = true;
			else
				m_BTWSide = false;

			if(CheckBTWEyes() > 0)
				GotoState('s_PlayerBTWPeek');
			else if(CheckBTWSide())
				GotoState('s_PlayerBTWMove');
		}
		else
		{
			ePawn.PlayBTW('BackCrAlNt0', '', 'BackStAlNt0', '', false, 1.0, 0.0);
		}
    }

ToggleCrouch:
	bIntransition = true;
	m_camera.SetMode(ECM_BTW);
	KillPawnSpeed();
	ePawn.PlayBTW('BackStAlDn0', '', 'BackStAlDn0', '', false, 1.0, 0.0, true);
	FinishAnim(0);
	bIntransition = false;
	Stop;

Release:
	bIntransition = true;
	SetBTWCamera(ECM_WalkingCr, ECM_WalkingCr, ECM_Walking, ECM_Walking);
	if(JumpLabel == 'Scope')
		ePawn.PlayBTW('BackCrAlEd0', '', 'BackStAlEd0', '', false, 1.5, 0.0);
	else
		ePawn.PlayBTW('BackCrAlEd0', '', 'BackStAlEd0', '', false, 1.0, 0.0);
	FinishAnim(0);
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	if(ePawn.bIsCrouched)
		ePawn.PlayAnimOnly(ePawn.AWaitCrouch);
	else
		ePawn.PlayAnimOnly(ePawn.AWait);
	bIntransition = false;
	if(JumpLabel == 'Scope')
		OnGroundScope();
	else
		GotoState('s_PlayerWalking');
	Stop;

Entering:
	m_camera.SetMode(ECM_BTW);
	KillPawnSpeed();
	ePawn.PlaySound(Sound'FisherFoley.Play_FisherBackToWallGear', SLOT_SFX);
	ePawn.PlayBTW('BackCrAlBg0', '', 'BackStAlBg0', '', false, 1.0, 0.0);
	FinishAnim(0);
	bIntransition = false;
	Stop;

EnteringBack:
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	m_camera.SetMode(ECM_BTW);
	KillPawnSpeed();
	FlipCameraLocalTarget();
	ePawn.PlaySound(Sound'FisherFoley.Play_FisherBackToWallGear', SLOT_SFX);
	ePawn.PlayBTW('BackCrAlEd0', '', 'BackStAlEd0', '', false, 1.0, 0.0, true, true);
	FinishAnim(0);
	bIntransition = false;
	Stop;
}

state s_PlayerBTWMove extends s_PlayerBTWBase
{
	Ignores Fire;

	function BeginState()
	{
		m_camera.SetMode(ECM_BTW);
		SetBTWAcceleration(1.0);
		ePawn.PlayBTW('BackCrAlFdL', 'BackCrAlFdR', 'BackStAlFdL', 'BackStAlFdR', m_BTWSide, 1.0, 0.3);
		Super.BeginState();
	}

    function PlayerMove(float deltaTime)
    {
		StickBTW();

		if(ePawn.bIsCrouched)
			ePawn.GroundSpeed = m_speedBTWCr;
		else
			ePawn.GroundSpeed = m_speedBTW;

		if(bIntransition)
		{
			if(CheckBTWEyes() > 0)
				KillPawnSpeed();
			return;
		}

		if(CheckForCrouchBTW())
			return;

		if(!CheckBTW() && CheckBTWRelease())
		{
			GotoState(, 'Release');
		}
		else if( (m_BTWSide && (aStrafe < -eGame.m_gentleForce)) ||
				(!m_BTWSide && (aStrafe > eGame.m_gentleForce)))
		{
			m_BTWSide = !m_BTWSide;
			m_camera.SetMode(ECM_BTW);
			SetBTWAcceleration(0.2);
			ePawn.PlayBTW('BackCrAlFdL', 'BackCrAlFdR', 'BackStAlFdL', 'BackStAlFdR', m_BTWSide, 1.0, 0.4);
		}
		else if(((Abs(aStrafe) < eGame.m_gentleForce) && (VSize(ePawn.Velocity) == 0.0)) ||
				!CheckBTWSide())
		{
			GotoState(, 'GoBack');
		}
		else if(CheckBTWEyes() > 0)
		{
			KillPawnSpeed();
			GotoState('s_PlayerBTWPeek');
		}
		else
		{
			if(Abs(aStrafe) < eGame.m_gentleForce)
				SetBTWAcceleration(0.0);
			else
				SetBTWAcceleration(1.0);

			ePawn.PlayBTW('BackCrAlFdL', 'BackCrAlFdR', 'BackStAlFdL', 'BackStAlFdR', m_BTWSide, 1.0, 0.0);
		}
    }

ToggleCrouch:
	m_camera.SetMode(ECM_BTW);
	ePawn.PlayBTW('BackCrAlFdL', 'BackCrAlFdR', 'BackStAlFdL', 'BackStAlFdR', m_BTWSide, 1.0, 0.35);
	Stop;

Release:
	bIntransition = true;
	SetBTWCamera(ECM_WalkingCr, ECM_WalkingCr, ECM_Walking, ECM_Walking);
	SetBTWAcceleration(0.0);
	if(JumpLabel == 'Scope')
		ePawn.PlayBTW('BackCrAlOfL', 'BackCrAlOfR', 'BackStAlOfL', 'BackStAlOfR', m_BTWSide, 1.5, 0.1);
	else
		ePawn.PlayBTW('BackCrAlOfL', 'BackCrAlOfR', 'BackStAlOfL', 'BackStAlOfR', m_BTWSide, 1.0, 0.1);
	FinishAnim(0);
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	if(ePawn.bIsCrouched)
		ePawn.PlayAnimOnly(ePawn.AWaitCrouch);
	else
		ePawn.PlayAnimOnly(ePawn.AWait);
	bIntransition = false;
	if(JumpLabel == 'Scope')
		OnGroundScope();
	else
		GotoState('s_PlayerWalking');

GoBack:
	bIntransition = true;
	m_camera.SetMode(ECM_BTW);
	KillPawnSpeed();
	ePawn.PlayBTW('BackCrAlNt0', 'BackCrAlNt0', 'BackStAlNt0', 'BackStAlNt0', m_BTWSide, 1.0, 0.3);
	Sleep(0.3);
	bIntransition = false;
	GotoState('s_PlayerBTW');
}

state s_PlayerBTWPeek extends s_PlayerBTWBase
{
	function ProcessScope()
	{
		if( CheckBTWEyes() == 2 && CheckBTWRelease() && HandGun != None )
			GotoState('s_PlayerBTWTargeting', 'Entering');
	}

	function BeginState()
	{
		KillPawnSpeed();
		SetBTWCamera(ECM_BTWPeakLeft, ECM_BTWPeakRight, ECM_BTWPeakLeft, ECM_BTWPeakRight);
		ePawn.PlayBTW('BackCrPkNtL', 'BackCrPkNtR', 'BackStPkNtL', 'BackStPkNtR', m_BTWSide, 1.0, 0.3);
		Super.Beginstate();
	}

    function PlayerMove(float deltaTime)
    {
		if(bIntransition)
			return;

		if(CheckForCrouchBTW())
			return;

		if(!CheckBTW() && CheckBTWRelease())
		{
			GotoState(, 'Release');
		}
		else if((m_BTWSide && (aStrafe < eGame.m_gentleForce)) ||
				(!m_BTWSide && (aStrafe > -eGame.m_gentleForce)))
		{
			m_camera.SetMode(ECM_BTW);
			ePawn.PlayBTW('BackCrAlNt0', 'BackCrAlNt0', 'BackStAlNt0', 'BackStAlNt0', m_BTWSide, 1.0, 0.3);
			GotoState('s_PlayerBTW');
		}
		else
		{
			ePawn.PlayBTW('BackCrPkNtL', 'BackCrPkNtR', 'BackStPkNtL', 'BackStPkNtR', m_BTWSide, 1.0, 0.0);
		}
    }

Release:
	bIntransition = true;
	m_camera.SetMode(ECM_BTW);
	ePawn.PlayBTW('BackCrAlNt0', 'BackCrAlNt0', 'BackStAlNt0', 'BackStAlNt0', m_BTWSide, 1.0, 0.2);
	Sleep(0.2);
	GotoState('s_PlayerBTW', 'Release');
	bIntransition = false;

ToggleCrouch:
	bIntransition = true;
	SetBTWCamera(ECM_BTWPeakLeft, ECM_BTWPeakRight, ECM_BTWPeakLeft, ECM_BTWPeakRight);
	ePawn.PlayBTW('BackStPkDnL', 'BackStPkDnR', 'BackStPkDnL', 'BackStPkDnR', m_BTWSide, 1.0, 0.0, true);
	FinishAnim(0);
	bIntransition = false;
	Stop;
}

state s_PlayerBTWTargeting extends s_PlayerBTWBase
{
	function BeginState()
	{
		ePawn.WalkingRatio = 0;
		ePawn.SoundWalkingRatio = 0;
		bIntransition = true;
		m_useTarget = true;
		m_SMInTrans = false;
		Super.BeginState();
	}

	function bool SlowGunBack()
	{
		return true;
	}

	function ProcessBackToWall()
	{
		Global.ProcessBackToWall();
	}

	function ProcessReloadGun()
	{
		Global.ProcessReloadGun();
	}

	function NotifyReloading()
	{
		bInGunTransition = true;
		ePawn.PlayReload(true, !m_BTWSide);
	}

	function EndState()
	{
		m_useTarget			= false;
		Super.EndState();

		// If gun is still in hands, remove view
		if( EMainHUD(myHud).hud_master == ActiveGun )
			EMainHUD(myHud).NormalView();
	}

	function ProcessScope()
	{
		if( !bIntransition && !bInGunTransition )
			GotoState(, 'GoBack');
	}

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = m_turnMul;
	}

	function SwitchCameraMode()
	{
		SetBTWCamera(ECM_BTWLeftFP, ECM_BTWRightFP, ECM_BTWLeftFP, ECM_BTWRightFP);
	}

    function PlayerMove(float deltaTime)
    {
		if(!m_SMInTrans && !bInGunTransition)
		{
			if(m_BTWSide)
				ePawn.AimAt(AABTW, Normal(m_targetLocation - Location), Vector(ePawn.Rotation), -65, 10, -70, 70);
			else
				ePawn.AimAt(AAHOH, Normal(m_targetLocation - Location), Vector(ePawn.Rotation), -10, 65, -70, 70);
		}

		if(bIntransition || bInGunTransition || CheckForCrouchBTW())
			return;

		if(!CheckBTW() && CheckBTWRelease())
		{
			JumpLabelPrivate = 'Release';
			ProcessScope();
		}
		else if((m_BTWSide && (aStrafe < eGame.m_gentleForce)) ||
				(!m_BTWSide && (aStrafe > -eGame.m_gentleForce)) ||
				(CheckBTWEyes() < 2))
		{
			ProcessScope();
		}
		else
		{
			ePawn.PlayBTW('BackCrSpNtL', 'BackCrSpNtR', 'BackStSpNtL', 'BackStSpNtR', m_BTWSide, 1.0, 0.0);
		}
    }

	function name GetWeaponAnimation()
	{
		if( ePawn.bIsCrouched )
		{
			if( m_BTWSide )
				return 'BackCrSpBgR';
			else
				return 'BackCrSpBgL';
		}
		else
		{
			if( m_BTWSide )
				return 'BackStSpBgR';
			else
				return 'BackStSpBgL';
		}
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function NotifyFiring()
	{
		Level.RumbleVibrate(0.07, 0.75);
		if(m_BTWSide)
			ePawn.BlendAnimOverCurrent('BackStSpFrR', 1, ePawn.UpperBodyBoneName);
		else
			ePawn.BlendAnimOverCurrent('BackStSpFrL', 1, ePawn.UpperBodyBoneName);
	}

ToggleCrouch:
	bIntransition = true;
	SetBTWCamera(ECM_BTWLeftFP, ECM_BTWRightFP, ECM_BTWLeftFP, ECM_BTWRightFP);
	ePawn.PlayBTW('BackStSpDnL', 'BackStSpDnR', 'BackStSpDnL', 'BackStSpDnR', m_BTWSide, 1.0, 0.0, true);
	FinishAnim();
	bIntransition = false;
	Stop;

GoBack:
	bIntransition = true;
	m_SMInTrans = true;
	SetBTWCamera(ECM_BTWPeakLeft, ECM_BTWPeakRight, ECM_BTWPeakLeft, ECM_BTWPeakRight);
	EMainHUD(myHud).NormalView();
	SheathWeapon();
	FinishAnim(ePawn.ACTIONCHANNEL);
	m_SMInTrans = false;
	bIntransition = false;
	GotoState('s_PlayerBTWPeek', JumpLabelPrivate);

Entering:
	m_SMInTrans = true;
	JumpLabelPrivate = '';
	KillPawnSpeed();
	DrawWeapon(,true,true);
	FinishAnim(ePawn.ACTIONCHANNEL);
	EMainHUD(myHud).Slave(ActiveGun);
	m_SMInTrans = false;
	bIntransition = false;
	ePawn.PlayBTW('BackCrSpNtL', 'BackCrSpNtR', 'BackStSpNtL', 'BackStSpNtR', m_BTWSide, 1.0, 0.0);
	Stop;
}

native(1170) final function bool CheckForCrouchBTW();

function SetBTWAcceleration(float alpha)
{
	if(!m_BTWSide)
		alpha = -alpha;
	ePawn.Acceleration = ePawn.ToWorldDir(vect(0, 1, 0)) * eGame.m_onGroundAccel * alpha;
}

function SetBTWCamera(ECamMode camModeCrL, ECamMode camModeCrR, ECamMode camModeStL, ECamMode camModeStR)
{
	if(ePawn.bIsCrouched)
	{
		if(m_BTWSide)
			m_camera.SetMode(camModeCrR);
		else
			m_camera.SetMode(camModeCrL);
	}
	else
	{
		if(m_BTWSide)
			m_camera.SetMode(camModeStR);
		else
			m_camera.SetMode(camModeStL);
	}
}

native(1167) final function bool CheckBTWRelease();
native(1168) final function int CheckBTWEyes();
native(1169) final function bool CheckBTWSide();

function int CheckRappellingFeet( out actor HitActor, out vector HitLocation, out vector HitNormal )
{
	local Vector	X,Y,Z, traceStart, traceEnd;

	GetAxes(ePawn.Rotation, X,Y,Z);

	traceStart  = ePawn.Location;
	traceStart += ePawn.CollisionRadius * X;
	traceEnd    = traceStart + 25.0 * X;

	hitActor = ePawn.Trace(HitLocation, HitNormal, TraceEnd, TraceStart);
	if( HitActor == None )
		return 0;
	else if( HitActor.IsA('EBreakableGlass') )
		return 1;
	else
		return 2;
}

// ----------------------------------------------------------------------
// state s_Rappelling
// ----------------------------------------------------------------------
state s_Rappelling
{
	Ignores Fire;

    function BeginState()
    {
		DiscardPickup();
		bInTransition = true;
		ePawn.bWantsToCrouch = false;
		m_camera.SetMode(ECM_Rapel);
    }

	function EndState()
	{
		ePawn.bKeepNPCAlive = false;
		RapelRopeType = 0;
		if(bInTransition)
			m_camera.StopFixeTarget();
	}

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = m_turnMul;
	}

	function PlayerMove( float DeltaTime )
	{
		local vector NewAccel;

		PawnLookAt();

		if( bIntransition || bInGunTransition )
			return;

		ePawn.SetPhysics(PHYS_Linear);

		// Do pulsed jump
		if( bPressedJump )
		{
			GotoState('s_RappellingFall');
			return;
		}

		if( aForward != 0 )
		{
			NewAccel = aForward/Abs(aForward) * 50.0 * Vect(0,0,1);

			ePawn.m_locationStart	= ePawn.Location;
			ePawn.m_locationEnd		= ePawn.ToWorld(NewAccel);
			ePawn.m_orientationStart= ePawn.Rotation;
			ePawn.m_orientationEnd	= ePawn.Rotation;

			// If there is no wall in front of top cylinder, don't move up
			if( AForward > 0 )
			{
				// Check to see if under ledge
				if( IsUnderLedge() )
				{
					// If something under his feet
					if( HasObjectInFront() )
						ePawn.LoopAnimOnly('raplstaldn0',,0.1, true, true);
					else
					{
						GotoState(,'EnterWindow');
						return;
					}
				}
				// Play OverTheEdge anim
				else
				{
					ePawn.Acceleration = Vect(0,0,0);
					GotoState(,'OverEdge');
					return;
				}
			}
			else
			{
				// Make sure rappelling height is respected. m_Elongation is in World coordinate.
				if( HasObjectInFront(true) && ePawn.Location.z > m_Elongation )
					ePawn.LoopAnimOnly('raplstaldn0',,0.1,, true);
				else
				{
					ePawn.Acceleration = Vect(0,0,0);
					ePawn.LoopAnimOnly(ePawn.ARappelWait,,0.1);
					return;
				}
			}

			ePawn.Acceleration = NewAccel;
		}
		else
		{
			ePawn.Acceleration = Vect(0,0,0);
			ePawn.LoopAnimOnly(ePawn.ARappelWait,,0.1);
		}
	}

	function bool IsUnderLedge()
	{
		local vector CylinderHighPoint;
		CylinderHighPoint = ePawn.ToWorld(ePawn.CollisionHeight*Vect(0,0,1));
		return CylinderHighPoint.z < m_HoistStOffset.Z;
	}

	function bool HasObjectInFront( optional bool bMovingDown )
	{
		local vector HitLocation, HitNormal, TraceStart1, TraceStart2, TraceEnd1, TraceEnd2, X,Y,Z;
		GetAxes(ePawn.Rotation,X,Y,Z);

		TraceStart1  = ePawn.Location;
		TraceStart1 += ePawn.CollisionRadius * X;
		if( bMovingDown )
		{
			TraceStart1 -= ePawn.CollisionHeight * 0.5f * Z;
			TraceStart2  = TraceStart1 - 20.0 * Z;
		}
		else
			TraceStart2  = TraceStart1 + 20.0 * Z;

		TraceEnd1    = TraceStart1 + 25.0 * X;
		TraceEnd2	 = TraceStart2 + 25.0 * X;

		return ePawn.Trace(HitLocation, HitNormal, TraceEnd1, TraceStart1) != None ||
			   ePawn.Trace(HitLocation, HitNormal, TraceEnd2, TraceStart2) != None;
	}

	function bool CanAccessQuick()
	{
		return !bInTransition && !bInGunTransition;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		if( IntObj.class.name == 'EWindowInteraction' )
			return true;
	}

	function bool CanInteract()
	{
		return true;
	}

	event NotifyAction()
	{
		if( Interaction != None )
			Interaction.Interact(self);
		if(bInGunTransition)
			RapelRopeType = 1;
	}

	function ProcessScope()
	{
		if( bInTransition || bInGunTransition || ActiveGun == None )
			return;
		bInGunTransition = true;
		GotoState('s_RappellingTargeting');
	}

	event bool NotifyLanded(vector HitNormal, actor Wall)
	{
		// We cannot reach destination without collision... give up
		GoToState('s_PlayerWalking');
		return true;
	}

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		// We cannot reach destination without collision... give up
		GoToState('s_PlayerWalking');
		return true;
	}

FromTop:
	bInTransition = true;
	ePawn.bKeepNPCAlive = true;

	// Move pawn in good spot to play anim
	ePawn.SetPhysics(PHYS_Linear);
	Interaction.SetInteractLocation(ePawn);
	ePawn.PlayMoveTo(Rotation);
	MoveToDestination(ArrivalSpeedApprox(1.0),true);
	bMoveToWalking = false;
	RapelRopeType = 1;

	ePawn.m_locationEnd = ePawn.ToWorld(Vect(-72.9f,0.0,-180.f));
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.Location) / ePawn.GetAnimTime('raplstalbg0'));

	ePawn.AddSoundRequest(Sound'FisherFoley.Play_RappelInstall', SLOT_SFX, 0.25f);
	ePawn.PlayAnimOnly('raplstalbg0');
	FinishAnim();

	ePawn.SetLocation(ePawn.m_locationEnd);
	m_camera.StopFixeTarget();

	// Notify interaction and make object collideable again
	Interaction.PostInteract(self);

	ePawn.LoopAnimOnly(ePawn.ARappelWait);
	ePawn.bKeepNPCAlive = false;
	Goto('Begin');

OverEdge:
	bInTransition = true;
	ePawn.bKeepNPCAlive = true;

	ePawn.m_locationEnd = ePawn.ToWorld(Vect(72.9f,0.0,180.f));
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.Location) / ePawn.GetAnimTime('raplstalbg0'));
	ePawn.SetLocation(ePawn.m_locationEnd);

	ePawn.AddSoundRequest(Sound'FisherFoley.Play_RappelInstall', SLOT_SFX, 2.5f);
	ePawn.PlayAnimOnly('raplstalbg0',,,true);
	FinishAnim();

	m_camera.StopFixeTarget();

	ePawn.bKeepNPCAlive = false;
	bInTransition = false;
	RapelRopeType = 0;
	GotoState('s_PlayerWalking');

OpenWindow:
	bIntransition = true;
	ePawn.Acceleration = Vect(0,0,0);
	epawn.PlayAnimOnly('raplStAlOp0',,0.1);
	FinishAnim();
	Goto('Begin');

EnterWindow:
	bIntransition = true;
	ePawn.bWantsToCrouch = true; // will be crouched after animation
	ePawn.Acceleration = Vect(0,0,0);
	ePawn.m_locationEnd = ePawn.ToWorld(Vect(40.4f, 0, 78.1f));
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.Location) / ePawn.GetAnimTime('raplstalbg0'));

	epawn.PlayAnimOnly('raplstalin0',,0.1);
	FinishAnim();

	ePawn.SetLocation(ePawn.m_locationEnd);
	m_camera.StopFixeTarget();

	ePawn.PlayAnimOnly(ePawn.AWaitCrouch);
	bInTransition = false;
	GotoState('s_PlayerWalking');

Begin:
	RapelRopeType = 1;
	bInTransition = false;
	Stop;

FromTargeting:
	RapelRopeType = 2;
	bInTransition = false;
}

// ----------------------------------------------------------------------
// state s_RappellingTargeting
// ----------------------------------------------------------------------
state s_RappellingTargeting
{
	function BeginState()
	{
		m_useTarget = true;
		ePawn.WalkingRatio = 0;
		ePawn.SoundWalkingRatio = 0;
		ePawn.Acceleration = Vect(0,0,0);

		if( !bInTransition )
			DrawWeapon();
	}

	function EndState()
	{
		RapelRopeType = 0;
		m_useTarget = false;

		EMainHUD(myHud).NormalView();

		if( !bInTransition )
			SheathWeapon();
	}

	function ProcessScope()
	{
		if( !bInTransition && !bInGunTransition )
			GoToState(,'PutGunBack');
	}

	function ProcessHeadSet( float i )
	{
		if( bInTransition )
			return;
		Global.ProcessHeadSet(i);
	}

	function UsePalm()
	{
		if( !bInTransition && !bInGunTransition )
			GotoState(,'BeginUsingPalm');
	}
	// Called from AirCamera for both split and rapel targeting)
	function bool OnGroundScope()
	{
		bInTransition = false;
		DrawWeapon();
		GotoState(,'Begin');
		return true;
	}

	event NotifyAction()
	{
		RapelRopeType = 2;
	}

	function SwitchCameraMode()
	{
		m_camera.SetMode(ECM_RapelFP);
	}

	function PlayerMove( float DeltaTime )
	{
		if( bIntransition )
			return;

		ePawn.LoopAnimOnly(StateWaitLoopingAnim(),,0.2);
		// Make body follow targeting angle
		ePawn.AimAt(AAFULL, Vector(Rotation), Vector(ePawn.Rotation), -90, 90, -90, 90);

		// Check under feet
		CheckFeet();

		if( bPressedJump && ActiveGun == MainGun && !bInGunTransition )
		{
			bInTransition = true;
			GotoState(,'BeginSniping');
		}
	}

	function CheckFeet()
	{
		local actor HitActor;
		local vector HitLocation, HitNormal;

		if( CheckRappellingFeet(HitActor, HitLocation, HitNormal) == 0 )
			GotoState(,'WindowBrokeUnderMyFeet');
	}

	function name GetWeaponAnimation()
	{
		if( ActiveGun.IsA('EOneHandedWeapon') )
			return 'RaplStSpBg1';
		else
			return 'RaplStSpBg2';
	}

	function name StateWaitLoopingAnim()
	{
		return ePawn.ARappelWait;
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function bool CanSwitchToHandItem( EInventoryItem Item )
	{
		return Item.IsA('ESecondaryAmmo');
	}

	function bool CanAccessQuick()
	{
		return ePawn.FullInventory.GetSelectedItem() == MainGun && !bInTransition && !bInGunTransition;
	}

	function NotifyFiring()
	{
		if( ePawn.WeaponStance == 1 )
		{
			Level.RumbleVibrate(0.07, 0.75);
			ePawn.BlendAnimOverCurrent('raplstspfr1', 1, ePawn.UpperBodyBoneName);
		}
		else
			Global.NotifyFiring();
	}

BeginSniping:
	GotoState('s_RappellingSniping');

PutGunBack:
	RapelRopeType = 2;
	ePawn.AddSoundRequest(Sound'FisherFoley.Play_RappelSecure', SLOT_SFX, 1.0f);
	ePawn.LoopAnimOnly('raplstalnt0',,0.2);
	GotoState('s_Rappelling', 'FromTargeting');

BeginUsingPalm:
	bInTransition = true;

	SheathWeapon();
	FinishAnim(EPawn.ACTIONCHANNEL);

	// set animation
	ePawn.AnimBlendParams(EPawn.ACTIONCHANNEL, 1,,,ePawn.UpperBodyBoneName);
	ePawn.LoopAnim('palmstalnt0',,0.8, EPawn.ACTIONCHANNEL);
	Stop;

WindowBrokeUnderMyFeet:
	ForceGunAway(true);
	ePawn.WeaponStance = 0;
	ePawn.SwitchAnims();
	GotoState('s_RappellingFall', 'ThroughWindow');

Begin:
	// will not happen from Sniper mode
	if( !bInTransition )
	{
		RapelRopeType = 1;
		ePawn.AddSoundRequest(Sound'FisherFoley.Play_RappelSecure', SLOT_SFX, 0.25f);
		FinishAnim(EPawn.ACTIONCHANNEL);
	}
	else
		RapelRopeType = 2;
	SwitchCameraMode();
	EMainHUD(myHud).Slave(ActiveGun);
	bIntransition = false;
}

// ----------------------------------------------------------------------
// state s_RappellingSniping
// ----------------------------------------------------------------------
state s_RappellingSniping extends s_PlayerSniping
{
	Ignores SwitchCameraMode;

	function BeginState()
	{
		Super.BeginState();
		// Overwrites switch to PHYS_Walking
		ePawn.SetPhysics(PHYS_Linear);
		JumpLabelPrivate = '';
		RapelRopeType = 2;
	}

	function EndState()
	{
		Super.EndState();
		RapelRopeType = 0;
	}

	function float GetSniperPosVar()
	{
		return 0.7;
	}

	function ProcessScope()
	{
		if( bInTransition || bInGunTransition )
			return;
		JumpLabelPrivate = 'PutGunBack';
		GotoState(,'BackToFirstPerson');
	}

	// Reload in Sniper mode stuff
	function NotifyReloading()
	{
		ZoomLevel = 0.0f;
		SetCameraFOV(self, DesiredFov);			// restore Fov that may have changed in sniping mode
		ESniperGun(ActiveGun).SetSniperMode(false);
		bHideSam = false;

		m_camera.SetMode(ECM_RapelFP);
		Global.NotifyReloading();
		GotoState(,'Reload');
	}

    function PlayerMove( float DeltaTime )
    {
		// Prevents glitch when coming back in targeting mode
		ePawn.AimAt(AAFULL, Vector(Rotation), Vector(ePawn.Rotation), -90, 90, -90, 90);

		CheckFeet();

		if( bPressedJump &&			// Go into sniper mode
			!bInGunTransition &&	// No drawing or sheating gun
			JumpLabelPrivate == '' )// Scope wasn't just pushed
		{
			bInTransition = true;
			GotoState(,'BackToFirstPerson');
		}
	}

	function CheckFeet()
	{
		local actor HitActor;
		local vector HitLocation, HitNormal;

		if( CheckRappellingFeet(HitActor, HitLocation, HitNormal) == 0 )
		{
			JumpLabelPrivate = 'WindowBrokeUnderMyFeet';
			GotoState(,'BackToFirstPerson');
		}
	}

BackToFirstPerson:
	GotoState('s_RappellingTargeting', JumpLabelPrivate);
}

// ----------------------------------------------------------------------
// state s_RappellingFall
// ----------------------------------------------------------------------
state s_RappellingFall
{
	Ignores MayFall;

	function BeginState()
	{
		RapelRopeType = 1;
		m_LPStartTime = -1;
	}

	function EndState()
	{
		RapelRopeType = 0;
	}

	function CheckWallLand()
	{
		local actor HitActor;
		local vector HitLocation, HitNormal;

		switch( CheckRappellingFeet(HitActor, HitLocation, HitNormal) )
		{
		// Hitting glass
		case 1:
			HitActor.TakeDamage(0, ePawn, HitLocation, HitNormal, HitLocation-ePawn.Location, None, 69);
		// Going through
		case 0:
			GotoState(,'ThroughWindow');
			break;
		// Solid land
		case 2:
			GotoState(,'End');
			break;
		}
	}

	function PlayerMove( float DeltaTime )
	{
		if( bInTransition )
			return;

		if( m_LPStartTime != -1 && Level.TimeSeconds - m_LPStartTime > 0.2f )
			CheckWallLand();

		// Jump almost horizontal if not holding down
		if( AForward == 0 )
			epawn.velocity.Z = 0;

		// Kill speed while doing the jump cycle
		if( ePawn.Location.z <= m_Elongation )
			KillPawnSpeed();
	}

	function AnimEnd( int Channel )
	{
		// If the ending anim is the jump air one
		// If not pushing down to do a longer jump
		if( Channel == 0 && !bInTransition )
		{
			// When cycle finishes, abort if height is limited or player not pushing down
			if( AForward >= 0 || ePawn.Location.z <= m_Elongation )
				CheckWallLand();

			// Begin fall timer
			m_LPStartTime = Level.TimeSeconds;
		}

		Global.AnimEnd(Channel);
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		if( HitActor != None && !HitActor.bIsMover )
			GotoState(,'Land');
		return true;
	}

begin:
	bInTransition = true;
	ePawn.SetPhysics(PHYS_None);
	ePawn.PlayAnimOnly('raplstaljb0',,0.2);
	FinishAnim();

	ePawn.PlaySound(Sound'FisherFoley.Play_RappelSlide', SLOT_SFX);
	if( ePawn.Location.z > m_Elongation )
		ePawn.SetPhysics(PHYS_Falling);
	ePawn.PlayAnimOnly('raplstaljc0');
	bInTransition = false;
	Stop;

End:
	bInTransition = true;
	ePawn.SetPhysics(PHYS_None);
	ePawn.PlayAnimOnly('raplstalje0');
	FinishAnim();
	bInTransition = false;
	GotoState('s_Rappelling');

ThroughWindow:
	bInTransition = true;
	ePawn.SetPhysics(PHYS_None);
	ePawn.PlayAnimOnly('RaplStAlEw0');
	FinishAnim();
	ePawn.SetLocation(ePawn.ToWorld(vect(100.0,0,0)));
	ePawn.Velocity += Vect(600,0,-300);
	bInTransition = false;
	GotoState('s_PlayerJumping');

Land:
	bInTransition = true;
	ePawn.SetPhysics(PHYS_None);
	ePawn.PlayLanding();
	FinishAnim();
	bInTransition = false;
	GoToState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_Split
// ----------------------------------------------------------------------
state s_Split
{
	ignores Fire;

	function BeginState()
	{
		m_camera.SetMode(ECM_SplitJump);
		ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.Default.CollisionHeight * 0.35);
	}

	function EndState()
	{
		if(bInTransition)
			m_camera.StopFixeTarget();
		ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.Default.CollisionHeight);
	}

	function bool CanSwitchGoggleManually()
	{
		return true;
	}

	function bool CanAccessQuick()
	{
		return true;
	}

	function ProcessScope()
	{
		if( bInTransition || bInGunTransition || ActiveGun == None )
			return;
		bInGunTransition = true;
		GotoState('s_SplitTargeting');
	}

    function PlayerMove(float deltaTime)
    {
		local vector testPos, moveDir, testExt;

		PawnLookAt();

		if(bInTransition)
		{
			testPos = DampVec(ePawn.Location, ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.m_locationStart) / ePawn.GetAnimTime('spltstalbgr'), deltaTime);
			if(ePawn.FastPointCheck(testPos, ePawn.GetExtent(), true, true))
				ePawn.SetLocation(testPos);
			else
				GoToState('s_PlayerJumping');
			return;
		}

		if(bPressedJump)
		{
			testExt.X = ePawn.CollisionRadius;
			testExt.Y = ePawn.CollisionRadius;
			testExt.Z = ePawn.default.CollisionHeight;
			if(!ePawn.FastPointCheck(ePawn.Location, testExt, true, true, true))
			{
				moveDir = vect(0,0,-1);
				moveDir *= ePawn.CollisionHeight;
				ePawn.Move(moveDir);
			}
			m_camera.SetMode(ECM_Walking);
			GoToState('s_PlayerJumping');
		}

		ePawn.LoopAnimOnly(ePawn.ASplitWait, , 0.1);
    }

Split:
	bInTransition = true;
	m_camera.SetFixeTarget(ePawn.m_locationEnd + vect(0,0,20.0), VSize(ePawn.m_locationEnd - ePawn.m_locationStart) / (ePawn.GetAnimTime('spltstalbgr') * 0.5));
	KillPawnSpeed();
	ePawn.SetPhysics(PHYS_None);
	if(ePawn.m_climbingUpperHand == CHRIGHT)
		ePawn.PlayAnimOnly('spltstalbgr', , 0.1);
	else
		ePawn.PlayAnimOnly('spltstalbgl', , 0.1);
	FinishAnim(0);
	ePawn.SetLocation(ePawn.Location + vect(0,0,20.0));
	ePawn.LoopAnimOnly(ePawn.ASplitWait);
	m_camera.StopFixeTarget();
	bInTransition = false;
}

// ----------------------------------------------------------------------
// state s_SplitTargeting
// ----------------------------------------------------------------------
state s_SplitTargeting extends s_RappellingTargeting
{
	function BeginState()
	{
		ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.Default.CollisionHeight * 0.35);
		Super.BeginState();
	}

	function EndState()
	{
		m_useTarget = false;
		ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.Default.CollisionHeight);

		EMainHUD(myHud).NormalView();

		if( !bInTransition )
			SheathWeapon();
	}

	function CheckFeet();

	function float GetSniperPosVar()
	{
		return 0.5;
	}

	function SwitchCameraMode()
	{
		m_camera.SetMode(ECM_SplitJumpFP);
	}

	function name GetWeaponAnimation()
	{
		if( ActiveGun.IsA('EOneHandedWeapon') )
			return 'SpltStSpBg1';
		else
			return 'SpltStSpBg2';
	}

	function name StateWaitLoopingAnim()
	{
		return ePawn.ASplitWait;
	}

	function NotifyFiring()
	{
		if( ePawn.WeaponStance == 1 )
		{
			Level.RumbleVibrate(0.07, 0.75);
			ePawn.BlendAnimOverCurrent('SpltStSpFr1', 1, ePawn.UpperBodyBoneName);
		}
		else
			Global.NotifyFiring();
	}

BeginSniping:
	GotoState('s_SplitSniping');

PutGunBack:
	GotoState('s_Split');

Begin:
	// will not happen from Sniper mode
	if( !bInTransition )
		FinishAnim(EPawn.ACTIONCHANNEL);

	SwitchCameraMode();
	EMainHUD(myHud).Slave(ActiveGun);
	bIntransition = false;
}

// ----------------------------------------------------------------------
// state s_SplitSniping
// ----------------------------------------------------------------------
state s_SplitSniping extends s_RappellingSniping
{
	function BeginState()
	{
		Super.BeginState();
		RapelRopeType = 0;
		ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.Default.CollisionHeight * 0.35);
	}

	function EndState()
	{
		Super.EndState();
		ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.Default.CollisionHeight);
	}

	function CheckFeet();

BackToFirstPerson:
	GotoState('s_SplitTargeting', JumpLabelPrivate);
}

// ----------------------------------------------------------------------
// state s_Fence
// ----------------------------------------------------------------------
state s_Fence
{
	ignores Fire;

    function BeginState()
    {
		m_didCrossFence = false;
		m_GECanGoForward = true;
		m_SMInTrans = false;
		DiscardPickup();
		ForceGunAway();
		ePawn.GroundSpeed = 0.0;
		m_camera.SetMode(ECM_FSphere);
		bInTransition = false;
		ePawn.SetPhysics(PHYS_Fence);
		ePawn.SetFenceZones();
    }

    function EndState()
    {
		ePawn.bKeepNPCAlive = false;
		if(bInTransition)
		{
			bInTransition = false;
			m_camera.StopFixeTarget();
		}
		ePawn.ResetZones();
		m_LastLedgeFenceTime = Level.TimeSeconds;
    }

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		// We got a floor so get out of the fence
		if(!bInTransition)
			GoToState(, 'OutBottom');
		return true;
	}

	event bool FenceTop(vector locationEnd, rotator orientationEnd)
	{
		local vector testOffset, testPos;

		if(!bInTransition && aForward > eGame.m_gentleForce)
		{
			if(m_didCrossFence)
			{
				m_GECanGoForward = false;
				return false;
			}

			if(ePawn.FastPointCheck(locationEnd, ePawn.GetExtent(), true, true))
			{
				testOffset.X = (ePawn.CollisionRadius * 2.0) + 4.0;
				testPos = locationEnd + ePawn.ToWorldDir(testOffset);
				if(ePawn.FastPointCheck(testPos, ePawn.GetExtent(), true, true))
				{
					GoToState(, 'OverTop');
					return true;
				}
			}
		}
		return false;
	}

	event bool Ledge(vector locationEnd, rotator orientationEnd, int LType)
	{
		local vector HitLocation, HitNormal;

		// We have a ledge in the arms zone.
		if(	!bInTransition &&
			ePawn.Trace(HitLocation, HitNormal, locationEnd, ePawn.Location, true, ePawn.GetExtent(), , true) == None)
		{
			GoToState('s_GrabbingLedge', 'Above');
			return true;
		}
		return false;
	}

    function PlayerMove(float deltaTime)
    {
        local vector X,Y,Z, Xv,Yv,Zv, NewAccel;

        if(bInTransition)
		{
			if(m_SMInTrans && IsPushingFull() && ((m_EnterDir dot GetPushingDir()) < -0.7))
			{
				m_SMInTrans = false;
				GotoState(, 'OutFast');
			}
			return;
		}

		if(m_didCrossFence && aForward < eGame.m_gentleForce)
		{
			m_GECanGoForward = true;
			m_didCrossFence = false;
		}

		PawnLookAt();

		GetAxes(ePawn.Rotation,X,Y,Z);
		GetAxes(Rotation,Xv,Yv,Zv);

		Xv.Z = 0.0;

		if(m_GECanGoForward)
			NewAccel = aForward * Z;
		if((X dot Xv) > 0.0)
			NewAccel += aStrafe * Y;
		else
			NewAccel -= aStrafe * Y;
		ePawn.Acceleration   = NewAccel;
		ePawn.Acceleration *= eGame.m_onGroundAccel;

		if(bPressedJump || !ePawn.m_validFence)
		{
			ePawn.Velocity = X * -200.0;
			if ( ePawn.Base.SurfaceType == SURFACE_FenceMetal )
				ePawn.PlaySound(Sound'FisherFoley.Play_Random_FisherFenceVib', SLOT_SFX);
			else if ( ePawn.Base.SurfaceType == SURFACE_FenceVine )
				ePawn.PlaySound(Sound'FisherFoley.Play_Random_VineVibration', SLOT_SFX);
			ePawn.MakeNoise(500.0, NOISE_HeavyFootstep);
			JumpRelease();
			return;
		}
		else if(IsPushingGentle())
		{
			ePawn.GroundSpeed = DampFloat(ePawn.GroundSpeed, m_speedFence, 150.0, Level.m_dT);
		}
		else
			ePawn.GroundSpeed = 0.0;

		if(VSize(ePawn.Velocity) > 0)
			ePawn.PlayFenceBlend();
		else
			ePawn.LoopAnimOnly('fencstalnt0', , 0.2);
    }

	function bool IsElectrocutingFence()
	{
		return EElectrifiedFence(ePawn.Base) != None && EElectrifiedFence(ePawn.Base).Electrified;
	}

OutFast:
	ePawn.PlayAnimOnly('FencStAlIO0', , , true, true);
	FinishAnim();
	bIntransition = false;
	GoToState('s_PlayerWalking');

OutBottom:
	bInTransition = true;
	KillPawnSpeed();
	ePawn.PlayAnimOnly('FencStAlIO0', , 0.1, true);
	FinishAnim();
	bInTransition = false;
	GoToState('s_PlayerWalking');

FromGround:
	bInTransition = true;
	KillPawnSpeed();
	ePawn.PlayAnimOnly('FencStAlIO0', , 0.1);
	m_SMInTrans = true;
	FinishAnim();
	m_SMInTrans = false;
	bInTransition = false;
	if( IsElectrocutingFence() )
		Goto('Electrocuted');
	Stop;

FromAir:
	bInTransition = true;
	KillPawnSpeed();
	ePawn.PlayAnim('FencStAlBg0', , 0.1);
	FinishAnim();
	bInTransition = false;
	if( IsElectrocutingFence() )
		Goto('Electrocuted');
	Stop;

OverTop:
	bInTransition = true;
	ePawn.bKeepNPCAlive = true;
	ePawn.SetPhysics(PHYS_Linear);
	MoveToDestination(ArrivalSpeedApprox(1.0));
	ePawn.m_locationEnd.X = (ePawn.CollisionRadius * 2.0) + 4.0;
	ePawn.m_locationEnd.Y = 0.0;
	ePawn.m_locationEnd.Z = 0.0;
	ePawn.m_locationEnd = ePawn.ToWorld(ePawn.m_locationEnd);
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
	{
		m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.Location) / ePawn.GetAnimTime('fencstalco0'));
		ePawn.PlayAnimOnly('fencstalco0');
		FinishAnim();
		m_camera.StopFixeTarget();
		ePawn.SetLocation(ePawn.m_locationEnd);
		ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
		FlipCameraLocalTarget();
		ePawn.PlayAnimOnly('fencstalnt0');
		m_didCrossFence = true;
	}
	ePawn.SetPhysics(PHYS_Fence);
	ePawn.bKeepNPCAlive = false;
	bInTransition = false;
	Stop;

Electrocuted:
	bInTransition = true;

	// Push Pawn away from fence
	ePawn.AddAmbientDamage(EElectrifiedFence(ePawn.Base).Damage);
	ePawn.PlayAnimOnly('FencStAlXx0',,0.1);
	FinishAnim();
	ePawn.AddAmbientDamage(-EElectrifiedFence(ePawn.Base).Damage);

	ePawn.Velocity += ePawn.ToWorldDir(Vect(-250,0,25));
	bInTransition = false;
	GoToState('s_PlayerJumping');
}

state s_GrabbingGE
{
	Ignores ProcessHeadSet;

	function BeginState()
	{
		m_LPStartTime = 0.0;
		m_EnterDir = GetPushingDir();
		aForward = 0.0;
		aStrafe = 0.0;
		DiscardPickup();
		ForceGunAway();
	}

	event MoveToDestinationFailed()
	{
		GoToState('s_PlayerWalking');
	}
}

// ----------------------------------------------------------------------
// state s_GrabbingLedge
// ----------------------------------------------------------------------

state s_GrabbingLedge extends s_GrabbingGE
{
	ignores Fire;

	function EndState()
	{
		m_LastLedgeFenceTime = Level.TimeSeconds;
	}

Above:
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.PlayAnimOnly('LedgStAlBg0', , ((1.0 - (FClamp(Abs(ePawn.Velocity.Z), 150.0, 800.0) / 800.0)) * 0.1) + 0.05);
	MoveToDestination(ArrivalSpeedApprox(1.5));
	m_camera.SetMode(ECM_HSphere);
	ePawn.AddSoundRequest(Sound'FisherFoley.Play_Switch_FisherGrab', SLOT_SFX, 0.05);
	FinishAnim();
	ePawn.LoopAnimOnly('ledgstalnt0', , 0.1);
	GoToState('s_Ledge');
}

// ----------------------------------------------------------------------
// state s_Ledge
// ----------------------------------------------------------------------
state s_Ledge
{
	ignores Fire;
    function BeginState()
    {
		m_camera.SetMode(ECM_HSphere);
        ePawn.SetLedgeGrabZones();
    }

	function EndState()
    {
		ePawn.bKeepNPCAlive = false;
		if(bInTransition)
		{
			m_camera.StopFixeTarget();
			bInTransition = false;
		}
        ePawn.ResetZones();
		KillPawnSpeed();
		m_LastLedgeFenceTime = Level.TimeSeconds;
		ElapsedTime = 0.0f;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
    }

	function bool CanAccessQuick()
	{
		return !bInTransition;
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		GoToState('s_PlayerWalking');
		return true;
	}
	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		return true;
	}

	event NoGE()
	{
		// BAD!!! No more ledge...
		GoToState('s_PlayerJumping');
	}

	event bool Ledge(vector locationEnd, rotator orientationEnd, int LType)
	{
		local vector checkLocation, extent, deltaMove;
		local bool pushingSide;

		if(bInTransition)
			return false;

		m_LedgeTurnRight = (LType & 16) != 0;
		pushingSide = ((aStrafe > eGame.m_fullForce && m_LedgeTurnRight) || (aStrafe < -eGame.m_fullForce && !m_LedgeTurnRight));
		deltaMove = locationEnd - ePawn.Location;

		// Try to reposition on the ledge if the gap is big enough
		m_GECanGoLeft = (LType & 1) != 0;
		m_GECanGoRight = (LType & 2) != 0;
		if(!m_GECanGoLeft  && !m_GECanGoRight)
			GoToState('s_PlayerJumping');
		else if((LType & 4) != 0 && pushingSide)
				GoToState(, 'TurnIn');
		else if((LType & 8) != 0 && pushingSide)
		{
			extent = ePawn.GetExtent();
			if(m_LedgeTurnRight)
				checkLocation = ePawn.ToWorldDir(vect(72.0,  72.0, 0.0));
			else
				checkLocation = ePawn.ToWorldDir(vect(72.0, -72.0, 0.0));
			checkLocation += locationEnd;
			if(ePawn.FastPointCheck(checkLocation, extent, true, true))
			{
				GoToState(, 'TurnOut');
				return true;
			}
		}
		else if(VSize(deltaMove) > 0.1 && (LType & 8) == 0)
		{
			// Relocate
			ePawn.Move(deltaMove);
			if(VSize(ePawn.Location - locationEnd) > 1.0)
				GoToState('s_PlayerWalking');

			// Reoriente
			ePawn.SetRotation(orientationEnd);
		}
		return false;
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);

		HandleBreathSounds(deltaTime);
	}

	// Can only move sideway
    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z, localSpeed;
		local vector extent, checkLocationUp, checkLocationDown;
		local float halfAdjustHeight;
		local vector HitLocation, HitNormal;

		if(bInTransition)
			return;

		PawnLookAt();

        GetAxes(ePawn.Rotation,X,Y,Z);

		if(aForward < eGame.m_backwardFull || bPressedJump)
		{
			ePawn.SetPhysics(PHYS_Falling);
			GoToState('s_PlayerJumping');
		}
		else if((aForward > eGame.m_forwardFull) &&
				m_GECanHoist &&
				TryHoisting(ePawn.Location, ePawn.Rotation, ePawn.m_HoistOffset, ePawn.m_HoistCrOffset, 'ledgstalup0', 'ledgcralup0'))
		{
			hoistTargetRot = ePawn.Rotation;
			GoToState('s_HoistingAboveLedge');
		}
		else
		{
			if(aStrafe < eGame.m_leftGentle && m_GECanGoLeft)
			{
				ePawn.Acceleration = Y * -eGame.m_LGShimmySpeed;
			}
			else
			{
				if(aStrafe > eGame.m_rightGentle && m_GECanGoRight)
					ePawn.Acceleration = Y * eGame.m_LGShimmySpeed;
				else
					ePawn.Acceleration = vect(0, 0, 0);
			}

			localSpeed = ePawn.ToLocalDir(ePawn.Velocity);

			if(localSpeed.Y < -eGame.m_LGSpeedThreshold)
			{
				ePawn.LoopAnimOnly('ledgstallt0');
			}
			else
			{
				if(localSpeed.Y > eGame.m_LGSpeedThreshold)
					ePawn.LoopAnimOnly('ledgstalrt0');
				else
					ePawn.LoopAnimOnly('ledgstalnt0', , 0.2);
			}
		}
    }

TurnIn:
	bInTransition = true;
	KillPawnSpeed();
	if(m_LedgeTurnRight)
	{
		ePawn.PlayAnimOnly('LedgStAlTiR', , 0.15);
		FinishAnim(0);
		ePawn.SetRotation(ePawn.Rotation + rot(0, 16384, 0));
		MirrorCameraLocalTarget(false);
	}
	else
	{
		ePawn.PlayAnimOnly('LedgStAlTiL', , 0.15);
		FinishAnim(0);
		ePawn.SetRotation(ePawn.Rotation + rot(0, -16384, 0));
		MirrorCameraLocalTarget(true);
	}
	ePawn.LoopAnimOnly('ledgstalnt0');
	bInTransition = false;
	Stop;

TurnOut:
	bInTransition = true;
	ePawn.bKeepNPCAlive = true;
	MoveToDestination(100);
	KillPawnSpeed();
	if(m_LedgeTurnRight)
	{
		m_camera.SetFixeTarget(ePawn.ToWorld(vect(0.0, 72.0, 0.0)), 144.0 / ePawn.GetAnimTime('LedgStAlToR'), ePawn.ToWorld(vect(72.0, 72.0, 0.0)));
		ePawn.PlayAnimOnly('LedgStAlToR', , 0.15);
		FinishAnim(0);
		ePawn.SetLocation(ePawn.ToWorld(vect(72.0, 72.0, 0.0)));
		ePawn.SetRotation(ePawn.Rotation + rot(0, -16384, 0));
		m_camera.StopFixeTarget();
		MirrorCameraLocalTarget(true);
	}
	else
	{
		m_camera.SetFixeTarget(ePawn.ToWorld(vect(0.0, -72.0, 0.0)), 144.0 / ePawn.GetAnimTime('LedgStAlToL'), ePawn.ToWorld(vect(72.0, -72.0, 0.0)));
		ePawn.PlayAnimOnly('LedgStAlToL', , 0.15);
		FinishAnim(0);
		ePawn.SetLocation(ePawn.ToWorld(vect(72.0, -72.0, 0.0)));
		ePawn.SetRotation(ePawn.Rotation + rot(0, 16384, 0));
		m_camera.StopFixeTarget();
		MirrorCameraLocalTarget(false);
	}
	ePawn.LoopAnimOnly('ledgstalnt0');
	ePawn.bKeepNPCAlive = false;
	bInTransition = false;
	Stop;

Begin:
}

// ----------------------------------------------------------------------
// state s_HoistingAboveLedge
// ----------------------------------------------------------------------
state s_HoistingAboveLedge
{
	Ignores Fire, ProcessHeadSet;

    function BeginState()
    {
		local vector camTarget;
		m_HoistDeltaTime = ePawn.GetAnimTime(m_HoistingAnim);
		m_HoistingDelta = (ePawn.m_locationEnd.Z - ePawn.Location.Z) - m_HoistAnimOffset.Z;
		camTarget = ePawn.m_locationEnd;
		if(ePawn.bWantsToCrouch)
			camTarget.Z -= (ePawn.CollisionHeight - ePawn.CrouchHeight);
		m_camera.SetFixeTarget(camTarget, VSize(camTarget - ePawn.Location) / m_HoistDeltaTime);
		bInTransition = true;
		ePawn.bKeepNPCAlive = true;
		KillPawnSpeed();
		ePawn.bCanCrouch = false;
		SetWalkingCameraMode(0.0);
    }

    function EndState()
    {
		m_camera.StopFixeTarget();
		bInTransition = false;
		ePawn.bKeepNPCAlive = false;
		ePawn.bCanCrouch = true;
    }

    function PlayerMove( float DeltaTime )
	{
		local vector delta;
		delta.Z = (DeltaTime / m_HoistDeltaTime) * m_HoistingDelta;
		ePawn.Move(delta);
		ePawn.RotateTowardsRotator(hoistTargetRot, 80000);
	}

begin:
    ePawn.PlayAnimOnly(m_HoistingAnim, , 0.15);
NoPlay:
	FinishAnim();
	ePawn.SetRotation(hoistTargetRot);
	if(ePawn.bWantsToCrouch)
		ePawn.Crouch(false);
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
		ePawn.SetLocation(ePawn.m_locationEnd);
	else
	{
		ePawn.SetLocation(ePawn.m_locationEnd + vect(0,0,15));
		ePawn.Move(vect(0,0,-15));
	}

	ePawn.PlayLedgeHoistingEnd();
	GoToState('s_PlayerWalking');
}

// ----------------------------------------------------------------------
// state s_GrabbingHandOverHand
// ----------------------------------------------------------------------

state s_GrabbingHandOverHand extends s_GrabbingGE
{
	ignores Fire;

	function BeginState()
	{
		bInTransition = false;
		ePawn.SetPhysics(PHYS_Linear);
		ePawn.Acceleration = vect(0.0, 0.0, 0.0);
		m_camera.SetMode(ECM_HOH);
		Super.BeginState();
	}

	function EndState()
	{
		m_LastHOHTime = Level.TimeSeconds;
		if(bInTransition)
		{
			m_camera.StopFixeTarget();
			bInTransition = false;
		}
	}

FromPipe:
	bInTransition = true;
	m_camera.SetFixeTarget(ePawn.ToWorld(vect(0.0, 0.0, -22.0)), 22.0 / ePawn.GetAnimTime('Pipvstalbhr'));
	if(ePawn.m_climbingUpperHand == CHRIGHT)
		ePawn.PlayAnimOnly('Pipvstalbhr');
	else
		ePawn.PlayAnimOnly('Pipvstalbhl');
	FinishAnim();
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	ePawn.PlayAnimOnly('piphstalnt0');
	ePawn.Move(vect(0.0, 0.0, -22.0));
	m_camera.StopFixeTarget();
	bInTransition = false;
	GoToState('s_HandOverHand');

FromAir:
	ePawn.PlayAnimOnly('PipHStAlbg0');
	MoveToDestination(ArrivalSpeedApprox(1.0));
	FinishAnim(0);
	GoToState('s_HandOverHand');
}

// ----------------------------------------------------------------------
// state s_HandOverHand
// ----------------------------------------------------------------------

state s_HandOverHand
{
    function BeginState()
    {
		KillPawnSpeed();
		m_camera.SetMode(ECM_HOH);
		ePawn.SetHandOverHandZones();
    }

    function EndState()
    {
        ePawn.ResetZones();
		KillPawnSpeed();
		m_camera.SetMode(ECM_Walking);
		m_LastHOHTime = Level.TimeSeconds;
		ElapsedTime = 0.0f;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
    }

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		if(m_PipeInFeet)
		{
			GoToState('s_GrabbingPipe', 'FromHOH');
		}
		return true;
	}

	event NoGE()
	{
		GoToState('s_PlayerJumping');
	}

	event bool ZipLine(vector locationEnd, rotator orientationEnd, int ZLType)
	{
		GoToState('s_GrabbingZipLine', 'FromAir');
		return true;
	}

	event bool HandOverHand(vector locationEnd, rotator orientationEnd)
	{
		if(!bInTransition && VSize(locationEnd - ePawn.Location) > 0.01)
		{
			ePawn.SetLocation(locationEnd);
			ePawn.SetRotation(orientationEnd);
		}
		return false;
	}

	function PawnLookAt()
	{
		local rotator lookAtRotPawn, lookAtRotEPC;

		lookAtRotPawn = ePawn.Rotation;
		lookAtRotPawn.Pitch = 0;
		lookAtRotEPC = Rotation;
		lookAtRotEPC.Pitch = 0;
		if(Vector(lookAtRotPawn) dot Vector(lookAtRotEPC) > -0.3)
			ePawn.LookAt(LANORMAL, Vector(Rotation), Vector(ePawn.Rotation), -50, 70, -70, 70);
	}

	function ProcessScope()
	{
		local vector checkPos, extent;

		if( bInTransition || HandGun == None )
			return;

		checkPos = ePawn.ToWorld(vect(60.0, 10.0, 40.0));
		extent = vect(8.0, 8.0, 8.0);
		if(ePawn.FastPointCheck(checkPos, extent, true, true))
		GotoState('s_HOHTargeting', 'TakeGunOut');
	}

	function bool CanAccessQuick()
	{
		return !bInTransition;
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);

		HandleBreathSounds(deltaTime);
	}

    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z;
		local vector localSpeed, hohDir, gap, testOffset;

		if(bInTransition)
			return;

		PawnLookAt();

        GetAxes(ePawn.Rotation, X, Y, Z);

		if(bPressedJump)
		{
			GoToState(,'FallDown');
		}
		else if(bDuck > 0)
		{
			bDuck = 0;
			hohDir = Normal(ePawn.m_geoTopPoint - ePawn.m_geoBottomPoint);
			if((hohDir dot X)  > 0.0)
			{
				// looking torward m_geoTopPoint
				gap = ePawn.m_geoTopPoint - ePawn.Location;
			}
			else
			{
				// looking torward m_geoBottomPoint
				gap = ePawn.m_geoBottomPoint - ePawn.Location;
			}
			gap.Z = 0.0;
			testOffset.X = eGame.m_HOHFeetUpColHoriOffset;
			if(	(VSize(gap) > eGame.m_HOHFeetUpGap) &&
				ePawn.FastPointCheck(ePawn.Location + (X * eGame.m_HOHFeetUpColHoriOffset), ePawn.GetExtent(), true, true))
			{
				GoToState('s_HandOverHandFeetUp', 'FeetUp');
			}
		}
		else
		{
			if(IsPushingTowardFront(0.0) && m_GECanGoForward)
				ePawn.Acceleration = DampVec(ePawn.Acceleration, X * eGame.m_HOHForwardSpeed, 100.0, Level.m_dT);
			else
			{
				ePawn.Acceleration = vect(0, 0, 0);
				if(IsPushingTowardBack(0.0))
					GoToState(, 'TurnAround');
			}

			localSpeed = ePawn.ToLocalDir(ePawn.Velocity);

			if(localSpeed.X > 0)
				ePawn.LoopAnimOnly('piphstalfd0', , 0.2);
			else
				ePawn.LoopAnimOnly('piphstalnt0', 1.0, 0.2);
		}
    }

FallDown:
	// Let go by jumping
	ePawn.SetPhysics(PHYS_Falling);
	GoToState('s_PlayerJumping');

TurnAround:
	bInTransition = true;
	KillPawnSpeed();
	ePawn.PlayAnimOnly('piphstalnt0', 1.0, 0.1);
	Sleep(0.1);
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	ePawn.PlayAnimOnly('PipHStAltr0');
	FinishAnim(0);
	bInTransition = false;
	Stop;
}

state s_HandOverHandFeetUp
{
	ignores Fire;

	function BeginState()
	{
		m_camera.SetMode(ECM_HOHFU);
		ePawn.SetHandOverHandZones();
	}

	function EndState()
	{
		ePawn.SetCollisionSize(ePawn.default.CollisionRadius, ePawn.default.CollisionHeight);
		ePawn.ResetZones();
		bInTransition = false;
		ElapsedTime = 0.0f;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
	}

	event NoGE()
	{
		GoToState('s_PlayerJumping');
	}

	event bool HandOverHand(vector locationEnd, rotator orientationEnd)
	{
		if(!bInTransition && VSize(locationEnd - ePawn.Location) > 0.01)
		{
			ePawn.SetLocation(locationEnd);
			ePawn.SetRotation(orientationEnd);
		}
		return false;
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);

		HandleBreathSounds(deltaTime);
	}

    function PlayerMove( float DeltaTime )
    {
		local vector localSpeed, X, Y, Z, testPos, testExtent;

		if(bInTransition)
			return;

		GetAxes(ePawn.Rotation,X,Y,Z);

		ePawn.LookAt(LAHOH, Vector(Rotation), Vector(ePawn.Rotation), 0, 0, -70, 70);

		if( bDuck > 0 || bPressedJump )
		{
			bDuck = 0;
			testPos = ePawn.Location;
			testPos.Z -= eGame.m_HOHFeetUpColVertOffset;
			testExtent.X = ePawn.CollisionRadius;
			testExtent.Y = ePawn.CollisionRadius;
			testExtent.Z = ePawn.default.CollisionHeight;
			if(ePawn.FastPointCheck(testPos, testExtent, true, true))
			{
				if( bPressedJump )
					JumpLabelPrivate = 'FallDown';
				else
					JumpLabelPrivate = '';
				GoToState(, 'FeetDown');
			}
		}
		else
		{
			if(	IsPushingTowardBack(0.0) &&
				m_GECanGoBackward &&
				ePawn.FastPointCheck(ePawn.Location + (X * -3.0), ePawn.GetExtent(), true, true))
			{
				ePawn.Acceleration = DampVec(ePawn.Acceleration, X * -eGame.m_HOHFeetUpMoveSpeed, 100.0, Level.m_dT);
			}
			else if(IsPushingTowardFront(0.0) &&
					m_GECanGoForward &&
					ePawn.FastPointCheck(ePawn.Location + (X * eGame.m_HOHFeetUpColHoriOffset), ePawn.GetExtent(), true, true))
			{
				ePawn.Acceleration = DampVec(ePawn.Acceleration, X * eGame.m_HOHFeetUpMoveSpeed, 100.0, Level.m_dT);
			}
			else
			{
				ePawn.Acceleration = vect(0, 0, 0);
			}

			localSpeed = ePawn.ToLocalDir(ePawn.Velocity);

			if(localSpeed.X > 0)
				ePawn.LoopAnimOnly('piphpralfd0', , , true);
			else if(localSpeed.X < 0)
				ePawn.LoopAnimOnly('piphpralfd0');
			else
				ePawn.LoopAnimOnly('piphpralnt0', , 0.2);
		}
    }

FeetUp:
	bInTransition = true;
	KillPawnSpeed();
	ePawn.SetCollisionSize(ePawn.CollisionRadius, eGame.m_HOHFeetUpColHeight);
	ePawn.Move(vect(0.0, 0.0, 1.0) * eGame.m_HOHFeetUpColVertOffset);
	ePawn.m_CurrentArmsZone.Z -= eGame.m_HOHFeetUpColVertOffset;
	ePawn.PlaySound( Sound'FisherFoley.Play_Fisher_StandToCrouchGear', SLOT_SFX );
	ePawn.PlayAnimOnly('piphpralbg0');
	FinishAnim();
	bInTransition = false;
	Stop;

FeetDown:
	bInTransition = true;
	KillPawnSpeed();
	m_camera.SetMode(ECM_HOH);
	ePawn.Move(vect(0.0, 0.0, -1.0) * eGame.m_HOHFeetUpColVertOffset);
	ePawn.SetCollisionSize(ePawn.CollisionRadius, ePawn.default.CollisionHeight);
	ePawn.m_CurrentArmsZone.Z += eGame.m_HOHFeetUpColVertOffset;
	ePawn.PlaySound( Sound'FisherFoley.Play_Fisher_CrouchToStandGear', SLOT_SFX );
	ePawn.PlayAnimOnly('piphpraled0');
	FinishAnim();
	bInTransition = false;
	GoToState('s_HandOverHand', JumpLabelPrivate);
}

state s_HOHTargeting extends s_FirstPersonTargeting
{
	Ignores MayReload, CanSwitchGoggleManually;

	event ReduceCameraSpeed(out float turnMul)
	{
		turnMul = m_turnMul;
	}

	function BeginState()
	{
		bInTransition = true;
		ePawn.WalkingRatio = 0;
		ePawn.SoundWalkingRatio = 0;
		m_useTarget = true;
	}

	function EndState()
	{
		m_useTarget = false;
		// If gun is still in hands, remove view
		if( EMainHUD(myHud).hud_master == ActiveGun )
			EMainHUD(myHud).NormalView();
	}

	function ProcessScope()
	{
		if( !bInTransition )
			GoToState( ,'PutGunBack');
	}

	function SwitchCameraMode()
	{
		m_camera.SetMode(ECM_HOHFP);
	}

    function PlayerMove( float DeltaTime )
    {
		ePawn.LoopAnim('PipHStSpNt1', , 0.2);

		if(bInTransition)
			return;

		ePawn.AimAt(AAHOH, Vector(Rotation), Vector(ePawn.Rotation), -80, 50, -80, 80);

		if( bPressedJump )
		{
			JumpLabelPrivate = 'FallDown';
			GoToState(,'PutGunBack');
		}
	}

	function NotifyFiring()
	{
		Level.RumbleVibrate(0.07, 0.75);
		ePawn.BlendAnimOverCurrent('piphstspfr1', 1, ePawn.UpperBodyBoneName);
	}

PutGunBack:
	bInTransition = true;
	m_camera.SetMode(ECM_HOH);
	EMainHUD(myHud).NormalView();
	SheathWeapon('PipHStSpBg1',,true);
	FinishAnim(EPawn.ACTIONCHANNEL);
	ePawn.LoopAnimOnly('piphstalnt0');
	bInTransition = false;
	GoToState('s_HandOverHand', JumpLabelPrivate);

TakeGunOut:
	JumpLabelPrivate = '';
	DrawWeapon('PipHStSpBg1', true, true);
	FinishAnim(EPawn.ACTIONCHANNEL);
	EMainHUD(myHud).Slave(ActiveGun);
	bInTransition = false;
}

// ----------------------------------------------------------------------
// state s_GrabbingNarrowLadder
// ----------------------------------------------------------------------
state s_GrabbingNarrowLadder extends s_GrabbingGE
{
	ignores Fire;

    function BeginState()
    {
		m_SMInTrans = false;
		ePawn.SetPhysics(PHYS_Linear);
		ePawn.bWantsToCrouch = false;
		KillPawnSpeed();
		Super.BeginState();
    }

    function EndState()
    {
		if(bInTransition)
		{
			m_camera.StopFixeTarget();
			bInTransition = false;
		}
		m_LastNLPipeTime = Level.TimeSeconds;
		ePawn.bKeepNPCAlive = false;
    }

    function PlayerMove(float deltaTime)
    {
		if(m_SMInTrans && IsPushingFull() && ((m_EnterDir dot GetPushingDir()) < -0.7))
		{
			bIntransition = false;
			m_SMInTrans = false;
			GotoState(, 'OutFast');
		}
    }

OutFast:
	ePawn.PlayAnimOnly('LaddStAlIOR', , , true, true);
	FinishAnim();
	bIntransition = false;
	GoToState('s_PlayerWalking');

FromUnder:
	bInTransition = true;
	ePawn.m_climbingUpperHand = CHRIGHT;
	MoveToDestination(ArrivalSpeedApprox(1.0));
	m_camera.SetFixeTarget(ePawn.Location + vect(0.0,0.0,193.0), 193.0 / ePawn.GetAnimTime('LaddStAlUp0'));
	ePawn.PlayAnimOnly('LaddStAlUp0', , 0.2);
	FinishAnim();
	m_camera.StopFixeTarget();
	ePawn.PlayNarrowLadderWait(0.0);
	ePawn.Move(ePawn.ToWorldDir(vect(0.0,0.0,193.0)));
	if(CalculateLadderDestination())
		ePawn.m_topClimbing = true;
	MoveToDestination(ArrivalSpeedApprox(1.0));
	bInTransition = false;
	GoToState('s_NarrowLadder');

FromTop:
	bInTransition = true;
	ePawn.bKeepNPCAlive = true;
	MoveToDestination(ArrivalSpeedApprox(1.0));
	ePawn.m_locationEnd = ePawn.m_geoTopPoint;
	ePawn.m_locationEnd += (ePawn.m_geoNormal * (ePawn.CollisionRadius + 2.0));
	ePawn.m_locationEnd.Z -= (2.0 + ePawn.m_NarrowLadderArmsZone.Z);
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.Location) / ePawn.GetAnimTime(ePawn.ANLInTop));
	ePawn.PlayNarrowLadderInTop();
	FinishAnim();
	ePawn.SetLocation(ePawn.m_locationEnd);
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	ePawn.m_climbingUpperHand = CHNONE;
	ePawn.m_topHanging = true;
	ePawn.m_topClimbing = true;
	ePawn.PlayNarrowLadderWait(0.0);
	m_camera.StopFixeTarget();
	CalculateLadderDestination();
	MoveToDestination(ArrivalSpeedApprox(1.0));
	ePawn.bKeepNPCAlive = false;
	bInTransition = false;
	GoToState('s_NarrowLadder');

FromAir:
	bInTransition = true;
	ePawn.m_climbingUpperHand = CHNONE;
	ePawn.PlayAnimOnly('LaddStAlbg0');
	MoveToDestination(ArrivalSpeedApprox(1.0));
	FinishAnim(0);
	bInTransition = false;
	GoToState('s_NarrowLadder');

FromGround:
	bInTransition = true;
	ePawn.m_climbingUpperHand = CHRIGHT;
	MoveToDestination(ArrivalSpeedApprox(1.0));
	ePawn.PlayAnimOnly('LaddStAlIOR', , 0.1);
	m_SMInTrans = true;
	FinishAnim(0);
	m_SMInTrans = false;
	ePawn.PlayNarrowLadderWait(0.0);
	if(CalculateLadderDestination())
		ePawn.m_topClimbing = true;
	MoveToDestination(ArrivalSpeedApprox(1.0));
	bInTransition = false;
	GoToState('s_NarrowLadder');
}


// ----------------------------------------------------------------------
// state s_NarrowLadder
// ----------------------------------------------------------------------
state s_NarrowLadder
{
	ignores Fire;

	function bool CanInteract()
	{
		return true;
	}

	function bool CanAddInteract( EInteractObject IntObj )
	{
		return (IntObj.class.name == 'ETrapInteraction' && ePawn.m_topHanging && ePawn.m_topClimbing && !bInTransition);
	}

    function BeginState()
    {
		ePawn.SetNarrowLadderZones();
		m_SMInTrans = false;
		m_LPSlideStartTime = Level.TimeSeconds;
		m_camera.SetMode(ECM_HSphere);
    }

    function EndState()
    {
		ePawn.bKeepNPCAlive = false;
		if(m_camera.m_fixedTarget)
			m_camera.StopFixeTarget();
		m_SMInTrans = false;
		bInTransition = false;
        ePawn.ResetZones();
		KillPawnSpeed();
		m_LastNLPipeTime = Level.TimeSeconds;
    }

	event NoGE()
	{
		GoToState('s_PlayerJumping');
		ePawn.m_topHanging = false;
		ePawn.m_topClimbing = false;
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		if(!m_SMInTrans)
		{
			GoToState(, 'OutBottom');
			ePawn.m_topHanging = false;
			ePawn.m_topClimbing = false;
		}
		return true;
	}

    function PlayerMove( float DeltaTime )
    {
		local vector localSpeed, locationEnd;

		if(m_SMInTrans)
			return;

		PawnLookAt();

		if(aForward > eGame.m_backwardFull)
			m_LPSlideStartTime = Level.TimeSeconds;

		if(bPressedJump)
		{
			JumpRelease();
		}
		else if( bInTransition )
		{
			return;
		}
		else if(aForward > eGame.m_forwardGentle)
		{
			ePawn.Acceleration.Z = eGame.m_NLUpwardSpeed;
			// Play a little anim for one loop
			GoToState(, 'GoUp');
		}
		else if(aForward < eGame.m_backwardFull &&
				!ePawn.m_topHanging &&
				!ePawn.m_topClimbing &&
				(m_LPSlideStartTime < (Level.TimeSeconds - eGame.m_NLSlideDelay)) &&
				m_GECanSlide &&
				m_GECanGoBackward)
		{
			// use a timer for delay
			// We are not at the top and pulling hard, let's slide...
			ePawn.Acceleration.Z = eGame.m_NLDownwardSpeed;
			ePawn.m_topClimbing = false;
			ePawn.m_topHanging = false;
			GoToState('s_NarrowLadderSlideDown');
		}
		else if(aForward < eGame.m_backwardGentle && m_GECanGoBackward)
		{
			if(ePawn.FastPointCheck(ePawn.Location + vect(0,0,-10.0), ePawn.GetExtent(), true, true))
			{
				ePawn.Acceleration.Z = eGame.m_NLDownwardSpeed;
				// Play a little anim for one loop
				GoToState(, 'GoDown');
			}
			else
				GoToState(, 'OutBottom');
		}
		else
		{
			ePawn.PlayNarrowLadderWait(0.2);
			ePawn.Acceleration = vect(0, 0, 0);
		}
    }

OpenTrap:
	bInTransition = true;
	ePawn.PlayAnimOnly('LaddStAlOp0');
	Sleep(0.5);
	Interaction.Interact(self);
	FinishAnim();
	bInTransition = false;
	Stop;

GoUp:
	bInTransition = true;
	if(ePawn.m_topHanging)
	{
		ePawn.m_locationEnd = ePawn.ToWorld(ePawn.m_NLOutTopAnimOffset);
		if(CanGetOutTopLadder())
		{
			// Already at most top, get out
			ePawn.m_topHanging = false;
			ePawn.m_topClimbing = false;
			GoToState(, 'OutTop');
		}
		else
		{
			KillPawnSpeed();
		}
	}
	else
	{
		if(ePawn.m_topClimbing)
		{
			// At the top with one hand, get both hands at top
			KillPawnSpeed();
			ePawn.PlayNarrowLadderTopUp();
			FinishAnim();

			// Find justified position and move there
			CalculateLadderDestination();
			if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
				MoveToDestination(ArrivalSpeedApprox(1.0));
			ePawn.m_topHanging = true;
		}
		else
		{
			// just go up 2 bars
			ePawn.PlayNarrowLadderUp();
			FinishAnim();

			// Find justified position and move there
			if(CalculateLadderDestination())
				ePawn.m_topClimbing = true;
			if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
			{
				ePawn.SetLocation(ePawn.m_locationEnd);
				if(aForward > eGame.m_forwardGentle)
				{
					ePawn.Acceleration.Z = eGame.m_NLUpwardSpeed;
					GoTo('GoUp');
				}
				else
				{
					KillPawnSpeed();
				}
			}
		}
	}
	bInTransition = false;
	Stop;

GoDown:
	bInTransition = true;
	if(!ePawn.FastPointCheck(ePawn.Location + vect(0,0,-10.0), ePawn.GetExtent(), true, true))
	{
		bInTransition = false;
		GoToState(, 'OutBottom');
	}
	if(ePawn.m_topHanging)
	{
		ePawn.m_topHanging = false;
		KillPawnSpeed();
		ePawn.PlayAnimOnly('laddstaleur', , , true);
		ePawn.m_climbingUpperHand = CHRIGHT;
		ePawn.m_topClimbing = true;
	}
	else
	{
		if(ePawn.PlayNarrowLadderDown())
			KillPawnSpeed();
		ePawn.m_topClimbing = false;
	}
	FinishAnim();

	// Find justified position and move there
	CalculateLadderDestination();
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
	{
		ePawn.SetLocation(ePawn.m_locationEnd);
		if(	aForward < eGame.m_backwardFull &&
			!ePawn.m_topHanging &&
			!ePawn.m_topClimbing &&
			(m_LPSlideStartTime < (Level.TimeSeconds - eGame.m_NLSlideDelay)) &&
			m_GECanSlide &&
			m_GECanGoBackward)
		{
			// We are not at the top and pulling hard, let's slide...
			ePawn.Acceleration.Z = eGame.m_NLDownwardSpeed;
			ePawn.m_topClimbing = false;
			ePawn.m_topHanging = false;
			GoToState('s_NarrowLadderSlideDown');
		}
		else if(aForward < -eGame.m_forwardGentle && m_GECanGoBackward)
		{
			ePawn.Acceleration.Z = eGame.m_NLDownwardSpeed;
			GoTo('GoDown');
		}
		else
		{
			KillPawnSpeed();
		}
	}
	bInTransition = false;
	Stop;

OutBottom:
	m_SMInTrans = true;
	m_camera.SetMode(ECM_Walking);
	ePawn.PlayNarrowLadderOutBottom();
	FinishAnim(0);
	m_SMInTrans = false;
	GoToState('s_PlayerWalking');

Adjust:
	bInTransition = true;
	CalculateLadderDestination();
	KillPawnSpeed();
	if(ePawn.m_locationEnd.Z > ePawn.Location.Z && m_GECanGoBackward)
		ePawn.m_locationEnd.Z -= eGame.m_NLStepSize * 2.0;
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
	{
		if(m_GECanGoBackward)
			MoveToDestination(ArrivalSpeedApprox(1.0));
		else
			MoveToDestination(100.0);
	}
	ePawn.m_climbingUpperHand = CHNONE;
	bInTransition = false;
	Stop;

OutTop:
	ePawn.bKeepNPCAlive = true;
	m_SMInTrans = true;
	KillPawnSpeed();
	m_camera.SetMode(ECM_Walking);
	ePawn.m_locationEnd = ePawn.ToWorld(ePawn.m_NLOutTopAnimOffset);
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_NLOutTopAnimOffset) / ePawn.GetAnimTime('laddstaleu0'));
	ePawn.PlayAnimOnly('laddstaleu0');
	FinishAnim();
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
		ePawn.SetLocation(ePawn.m_locationEnd);
	else
	{
		ePawn.SetLocation(ePawn.m_locationEnd + vect(0,0,15));
		ePawn.Move(vect(0,0,-15));
	}
	ePawn.PlayLedgeHoistingEnd();
	m_camera.StopFixeTarget();
	ePawn.bKeepNPCAlive = false;
	m_SMInTrans = false;
	GoToState('s_PlayerWalking');

}

native(1171) final function bool CanGetOutTopLadder();

// Calculates the closest bar and returns if top of ladder
native(1147) final function bool CalculateLadderDestination();

state s_SlideDownBase
{
    function BeginState()
    {
		bInTransition = false;
		m_camera.SetMode(ECM_HSphere);
    }

    function EndState()
    {
		bInTransition = false;
        ePawn.ResetZones();
		m_LastNLPipeTime = Level.TimeSeconds;
    }

	event NoGE()
	{
		if(bInTransition)
			return;
		GoToState('s_PlayerJumping');
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		GoToState(, 'landed');
		return true;
	}

landed:
	bInTransition = true;
	if(ePawn.PlayNLPLand(bDuck > 0, m_SMTemp))
		FinishAnim();
	else
		Sleep(0.15);
	bInTransition = false;
	GoToState('s_PlayerWalking');
}

state s_NarrowLadderSlideDown extends s_SlideDownBase
{
	ignores Fire;
    function BeginState()
    {
		Super.BeginState();
		ePawn.SetNarrowLadderZones();
		ePawn.PlaySound(Sound'FisherFoley.Play_FisherSlideLadder', SLOT_SFX);
    }

    function EndState()
    {
		Super.EndState();
		ePawn.StopSound(Sound'FisherFoley.Play_FisherSlideLadder');
    }

	event bool NarrowLadder(vector locationEnd, rotator orientationEnd, int NLType)
	{
		ePawn.SetLocation(locationEnd);
		ePawn.SetRotation(orientationEnd);
		if(!bInTransition && !m_GECanGoBackward)
		{
			KillPawnSpeed();
			bInTransition = true;
			GotoState(, 'GoBack');
		}
		return false;
	}

    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z, localSpeed;

        GetAxes(ePawn.Rotation,X,Y,Z);
		PawnLookAt();

		m_SMTemp = ePawn.Acceleration.Z;

		if(bInTransition)
		{
			return;
		}
		else if(bPressedJump)
		{
			JumpRelease();
		}
		else if(aForward < eGame.m_backwardFull)
		{
			// Acceleration
			ePawn.Velocity.Z = FMax(	ePawn.Velocity.Z - (eGame.m_NLSlideDownInertia * DeltaTime),
									-eGame.m_NLSlideDownMaxSpeed);
			ePawn.Acceleration = Z * ePawn.Velocity.Z;
		}
		else
		{
			// Deceleration
			ePawn.Velocity.Z += (eGame.m_NLSlideDownInertia * DeltaTime);
			ePawn.Acceleration = Z * ePawn.Velocity.Z;

			if(ePawn.Velocity.Z > -eGame.m_NLSlideDownMinSpeed)
				GoToState(, 'GoBack');
		}
    }

begin:
	ePawn.PlayNarrowLadderSlideDown(0.2);
	Stop;

GoBack:
	bInTransition = true;
	ePawn.PlayNarrowLadderWait(0.2);
	Sleep(0.2);
	bInTransition = false;
	GoToState('s_NarrowLadder', 'Adjust');
}

// ----------------------------------------------------------------------
// state s_GrabbingPipe
// ----------------------------------------------------------------------
state s_GrabbingPipe extends s_GrabbingGE
{
	ignores Fire;

    function BeginState()
    {
		m_SMInTrans = false;
		bInTransition = true;
		ePawn.SetPhysics(PHYS_Linear);
		ePawn.bWantsToCrouch = false;
		ePawn.Acceleration = vect(0.0, 0.0, 0.0);
		Super.BeginState();
    }

    function EndState()
    {
		if(m_camera.m_fixedTarget)
			m_camera.StopFixeTarget();
		bInTransition = false;
		ePawn.bKeepNPCAlive = false;
		m_LastNLPipeTime = Level.TimeSeconds;
    }

    function PlayerMove(float deltaTime)
    {
		if(m_SMInTrans && IsPushingFull() && ((m_EnterDir dot GetPushingDir()) < -0.7))
		{
			bIntransition = false;
			m_SMInTrans = false;
			GotoState(, 'OutFast');
		}
    }

OutFast:
	ePawn.PlayAnimOnly('pipvstalior', , , true, true);
	FinishAnim();
	bIntransition = false;
	GoToState('s_PlayerWalking');

FromTop:
	ePawn.bKeepNPCAlive = true;
	// We detected the pipe from above
	MoveToDestination(ArrivalSpeedApprox(1.0));
	ePawn.m_locationEnd = ePawn.m_geoTopPoint;
	ePawn.m_locationEnd += (ePawn.m_geoNormal * (ePawn.CollisionRadius + 2.0));
	ePawn.m_locationEnd.Z -= (-1.0 + ePawn.m_PipeArmsZone.Z);
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_locationEnd - ePawn.Location) / ePawn.GetAnimTime('pipvstalbul'));
	ePawn.PlayPipe('pipvstalbul', 'pipvstalbur');
	FinishAnim();
	ePawn.SetLocation(ePawn.m_locationEnd);
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	ePawn.PlayPipe('pipvstalntl', 'pipvstalntr', , , true);
	// We are at the top
	CalculatePipeDestination();
	MoveToDestination(ArrivalSpeedApprox(1.0));
	m_camera.StopFixeTarget();
	ePawn.bKeepNPCAlive = false;
	GoToState('s_Pipe');

FromAir:
	ePawn.m_climbingUpperHand = CHRIGHT;
	ePawn.PlayAnimOnly('PipVStAlbg0', , 0.15);
	MoveToDestination(ArrivalSpeedApprox(1.0));
	FinishAnim(0);
	GoToState('s_Pipe');

FromGround:
	ePawn.m_climbingUpperHand = CHRIGHT;
	MoveToDestination(ArrivalSpeedApprox(1.0));
	ePawn.PlayPipe('pipvstaliol', 'pipvstalior', 0.1, , true);
	m_SMInTrans = true;
	FinishAnim();
	m_SMInTrans = false;
	GoToState('s_Pipe');

FromHOH:
	ePawn.m_climbingUpperHand = CHRIGHT;
	m_camera.SetFixeTarget(ePawn.ToWorld(vect(0.0, 0.0, 22.0)), 22.0 / ePawn.GetAnimTime('PipHStAlBV0'));
	ePawn.PlayAnimOnly('PipHStAlBV0', , 0.1);
	FinishAnim(0);
	m_camera.StopFixeTarget();
	ePawn.Move(vect(0,0,22.0));
	ePawn.PlayPipe('pipvstalntl', 'pipvstalntr', , , true);
	GoToState('s_Pipe');
}


// ----------------------------------------------------------------------
// state s_Pipe
// ----------------------------------------------------------------------
state s_Pipe
{
	ignores Fire;
    function BeginState()
    {
		m_SMInTrans = false;
		ePawn.SetPipeZones();
		m_camera.SetMode(ECM_HSphere);
    }

    function EndState()
    {
		if(m_camera.m_fixedTarget)
			m_camera.StopFixeTarget();
		bInTransition = false;
		m_SMInTrans = false;
        ePawn.ResetZones();
		KillPawnSpeed();
		m_LastNLPipeTime = Level.TimeSeconds;
		ElapsedTime = 0.0f;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
		ePawn.bKeepNPCAlive = false;
    }

	event NoGE()
	{
		if(bInTransition)
			return;

		GoToState('s_PlayerJumping');
	}

	event bool Pipe(vector locationEnd, rotator orientationEnd)
	{
		if(!m_SMInTrans && VSize(locationEnd - ePawn.Location) > 0.01)
		{
			ePawn.SetLocation(locationEnd);
			ePawn.SetRotation(orientationEnd);
		}
		return false;
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		if(!m_SMInTrans)
		{
			GoToState(, 'OutBottom');
			ePawn.m_topHanging = false;
			ePawn.m_topClimbing = false;
		}
		return true;
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);

		HandleBreathSounds(deltaTime);
	}

    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z, localSpeed, preferedCheckLocation, otherCheckLocation, revOffset;
		local bool canGoUp;
		local int outTopResult;

		if(m_SMInTrans)
			return;

		canGoUp = true;

        GetAxes(ePawn.Rotation,X,Y,Z);
		PawnLookAt();

		if(CalculatePipeDestination() && aForward > eGame.m_forwardGentle)
		{
			revOffset = ePawn.m_POutTopAnimOffset;
			revOffset.Y = -revOffset.Y;
			if(ePawn.m_climbingUpperHand == CHRIGHT)
			{
				preferedCheckLocation = ePawn.ToWorld(ePawn.m_POutTopAnimOffset);
				otherCheckLocation = ePawn.ToWorld(revOffset);
			}
			else
			{
				preferedCheckLocation = ePawn.ToWorld(revOffset);
				otherCheckLocation = ePawn.ToWorld(ePawn.m_POutTopAnimOffset);
			}

			outTopResult = CanGetOutTopPipe(preferedCheckLocation, otherCheckLocation);
			if(outTopResult == 1)
			{
				GoToState(, 'OutTopNormal');
				return;
			}
			else if(outTopResult == 2)
			{
				GoToState(, 'OutTopReverse');
				return;
			}
			else
			{
				if(m_PType > 0)
				{
					if(m_PType == 1)
						GoToState('s_GrabbingHandOverHand', 'FromPipe');
					else
						GoToState('s_GrabbingZipLine', 'FromPipe');
					return;
				}

				canGoUp = false;
			}
			ePawn.Acceleration = vect(0, 0, 0);
		}

		if(bPressedJump)
		{
			JumpRelease();
		}
		else if( bInTransition )
		{
			if(!m_GECanGoBackward && ePawn.Acceleration.Z < 0.0)
				KillPawnSpeed();
			return;
		}
		else if(aForward > eGame.m_forwardGentle && canGoUp)
		{
			ePawn.Acceleration = Z * eGame.m_PUpwardSpeed;
			ePawn.PlayPipe('pipvstalupl', 'pipvstalupr');
			GoToState('s_Pipe', 'Move');
		}
		else if((aForward < eGame.m_backwardFull) &&
				m_GECanSlide &&
				m_GECanGoBackward &&
				ePawn.FastPointCheck(ePawn.Location - vect(0,0,20.0), ePawn.GetExtent(), true, true))
		{
			// We are not at the top and pulling hard, let's slide...
			ePawn.Acceleration = Z * eGame.m_PDownwardSpeed;
			GoToState('s_PipeSlideDown');
		}
		else if(aForward < eGame.m_backwardGentle &&
				m_GECanGoBackward)
		{
			if(ePawn.FastPointCheck(ePawn.Location + vect(0,0,-10.0), ePawn.GetExtent(), true, true))
			{
				ePawn.Acceleration = Z * eGame.m_PDownwardSpeed;
				ePawn.PlayPipe('pipvstalupr', 'pipvstalupl', , true);
				GoToState(, 'Move');
			}
			else
			{
				GoToState(, 'OutBottom');
			}
		}
		else
		{
			ePawn.PlayPipe('pipvstalntl', 'pipvstalntr', , , true);
			ePawn.Acceleration = vect(0, 0, 0);
		}
    }

OutBottom:
	m_camera.SetMode(ECM_Walking);
	bInTransition = true;
	m_SMInTrans = true;
	ePawn.PlayPipeOutBottom();
	FinishAnim(0);
	bInTransition = false;
	m_SMInTrans = false;
	GoToState('s_PlayerWalking');

Move:
	bInTransition = true;
	FinishAnim();
	if(CalculatePipeDestination() && ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
		MoveToDestination(ArrivalSpeedApprox(1.0));
	KillPawnSpeed();
	bInTransition = false;
	Stop;

Adjust:
	bInTransition = true;
	CalculatePipeDestination();
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
		MoveToDestination(ArrivalSpeedApprox(1.0));
	bInTransition = false;
	Stop;

OutTopReverse:
	bInTransition = true;
	m_SMInTrans = true;
	if(ePawn.m_climbingUpperHand == CHLEFT)
		ePawn.m_climbingUpperHand = CHRIGHT;
	else
		ePawn.m_climbingUpperHand = CHLEFT;

OutTopNormal:
	m_camera.SetMode(ECM_Walking);
	bInTransition = true;
	ePawn.bKeepNPCAlive = true;
	m_SMInTrans = true;
	ePawn.m_locationEnd = ePawn.m_geoTopPoint + (ePawn.m_geoNormal * ePawn.m_PipeArmsZone.X);
	ePawn.m_locationEnd.Z -= ePawn.m_PipeArmsZone.Z;
	ePawn.m_locationStart = ePawn.Location;
	ePawn.m_orientationStart = ePawn.Rotation;
	ePawn.m_orientationEnd = ePawn.Rotation;
	if(ePawn.FastPointCheck(ePawn.m_locationEnd, ePawn.GetExtent(), true, true))
		MoveToDestination(ArrivalSpeedApprox(1.0));
	if(ePawn.m_climbingUpperHand == CHRIGHT)
		ePawn.m_locationEnd = ePawn.ToWorld(ePawn.m_POutTopAnimOffset);
	else
	{
		ePawn.m_locationEnd = ePawn.m_POutTopAnimOffset;
		ePawn.m_locationEnd.Y = -ePawn.m_locationEnd.Y;
		ePawn.m_locationEnd = ePawn.ToWorld(ePawn.m_locationEnd);
	}
	m_camera.SetFixeTarget(ePawn.m_locationEnd, VSize(ePawn.m_POutTopAnimOffset) / ePawn.GetAnimTime('pipvstaleur'));
	ePawn.PlayPipe('pipvstaleul', 'pipvstaleur');
	FinishAnim();
	ePawn.SetLocation(ePawn.m_locationEnd);
	ePawn.PlayLedgeHoistingEnd();
	m_camera.StopFixeTarget();
	m_SMInTrans = false;
	ePawn.bKeepNPCAlive = false;
	bInTransition = false;
	GoToState('s_PlayerWalking');
}

native(1172) final function int CanGetOutTopPipe(vector preferedCheckLocation, vector otherCheckLocation);

// Calculates the best position and returns if top of pipe
native(1146) final function bool CalculatePipeDestination();

state s_PipeSlideDown extends s_SlideDownBase
{
	ignores Fire;
    function BeginState()
    {
		Super.BeginState();
		ePawn.SetPipeZones();
		ePawn.PlaySound(Sound'FisherFoley.Play_FisherSlideTube', SLOT_SFX);
    }
    function EndState()
    {
		Super.EndState();
		ePawn.StopSound(Sound'FisherFoley.Play_FisherSlideTube');
    }

	event bool Pipe(vector locationEnd, rotator orientationEnd)
	{
		if(VSize(locationEnd - ePawn.Location) > 0.01)
		{
			ePawn.SetLocation(locationEnd);
			ePawn.SetRotation(orientationEnd);
		}
		return false;
	}

    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z, localSpeed;

        GetAxes(ePawn.Rotation,X,Y,Z);
		PawnLookAt();

		m_SMTemp = ePawn.Acceleration.Z;

		if(bInTransition)
		{
			return;
		}
		else if(bPressedJump)
		{
			JumpRelease();
		}
		else if(!m_GECanGoBackward)
		{
			KillPawnSpeed();
			GoToState(, 'GoBack');
		}
		else if(aForward < eGame.m_backwardFull)
		{
			// Acceleration
			ePawn.Velocity.Z = FMax(	ePawn.Velocity.Z - (eGame.m_PSlideDownInertia * DeltaTime),
									-eGame.m_PSlideDownMaxSpeed);
			ePawn.Acceleration = Z * ePawn.Velocity.Z;
		}
		else
		{
			// Deceleration
			ePawn.Velocity.Z = FMin(ePawn.Velocity.Z + (eGame.m_PSlideDownInertia * DeltaTime), 0.0);
			ePawn.Acceleration = Z * ePawn.Velocity.Z;

			if(ePawn.Velocity.Z > -eGame.m_PSlideDownMinSpeed)
				GoToState(, 'GoBack');
		}
    }

begin:
	ePawn.PlayPipe('pipvstaldn0', 'pipvstaldn0', 0.2, , true);
	Stop;

GoBack:
	bInTransition = true;
	ePawn.PlayPipe('pipvstalntl', 'pipvstalntr', 0.2, , true);
	Sleep(0.2);
	bInTransition = false;
	GoToState('s_Pipe', 'Adjust');
}

// ----------------------------------------------------------------------
// state s_GrabbingZipLine
// ----------------------------------------------------------------------
state s_GrabbingZipLine extends s_GrabbingGE
{
	ignores Fire;

	function BeginState()
	{
		bInTransition = false;
		m_SMInTrans = false;
		m_camera.SetMode(ECM_FSphere);
		Super.BeginState();
	}

    function EndState()
    {
		m_LastZipLineTime = Level.TimeSeconds;
		if(bInTransition)
		{
			m_camera.StopFixeTarget();
			bInTransition = false;
		}
		m_SMInTrans = false;
    }

    function PlayerMove( float DeltaTime )
    {
        local vector flatDir;

		if(m_SMInTrans)
		{
			flatDir = ePawn.m_geoNormal;
			flatDir.Z = 0;
			ePawn.RotateTowardsRotator(Rotator(flatDir));
		}
    }

FromPipe:
	bInTransition = true;
	ePawn.SetZipLineZones();
	m_camera.SetFixeTarget(ePawn.ToWorld(vect(0.0, 0.0, -22.0)), 22.0 / ePawn.GetAnimTime('Pipvstalbhr'));
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.Acceleration = vect(0.0, 0.0, 0.0);
	if(ePawn.m_climbingUpperHand == CHRIGHT)
		ePawn.PlayAnimOnly('Pipvstalbhr', , 0.2);
	else
		ePawn.PlayAnimOnly('Pipvstalbhl', , 0.2);
	FinishAnim();
	ePawn.SetRotation(ePawn.Rotation + rot(0, 32768, 0));
	FlipCameraLocalTarget();
	ePawn.Move(vect(0,0,-22));
	m_camera.StopFixeTarget();
	m_SMInTrans = true;
	ePawn.PlayAnimOnly('piphstalnt0');
	ePawn.Acceleration = ePawn.m_geoNormal * 200.0;
	ePawn.Acceleration.Z = 0.0f;
	Sleep(0.0);
	ePawn.PlayAnimOnly('ZippStAlDn0', , 0.3);
	Sleep(0.3);
	m_SMInTrans = false;
	bInTransition = false;
	GoToState('s_ZipLine');

FromAir:
	ePawn.SetPhysics(PHYS_Linear);
	ePawn.Acceleration = ePawn.m_geoNormal * eGame.m_ZLSlideDownMinSpeed;
	ePawn.PlayAnimOnly('ZippStAlBg0', , 0.1);
	MoveToDestination(ArrivalSpeedApprox(1.0));
	GoToState('s_ZipLine');
}

state s_ZipLine
{
	ignores Fire;
    function BeginState()
    {
		ePawn.SetZipLineZones();
		ePawn.PlaySound(Sound'FisherFoley.Play_MLayer_FisherZipLine', SLOT_SFX);
		m_camera.SetMode(ECM_FSphere);
    }

    function EndState()
    {
        ePawn.ResetZones();
		ePawn.StopSound(Sound'FisherFoley.Play_MLayer_FisherZipLine');
		m_LastZipLineTime = Level.TimeSeconds;
    }

	event NoGE()
	{
		GoToState('s_PlayerJumping');
	}

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		if(!Wall.IsA('EBreakableGlass'))
			GoToState('s_PlayerJumping');

		return true;
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		GoToState('s_PlayerWalking');

		KillPawnSpeed();
		return true;
	}

	event bool ZipLine(vector locationEnd, rotator orientationEnd, int ZLType)
	{
		if(ZLType == 1)
		{
			GoToState('s_PlayerJumping');
		}
		else
		{
			if(VSize(locationEnd - ePawn.Location) > 0.01)
			{
				ePawn.SetLocation(locationEnd);
				ePawn.SetRotation(orientationEnd);
			}
			ePawn.Velocity = ePawn.m_geoNormal * VSize(ePawn.Velocity);
		}
		return false;
	}

	function CheckForCrouch()
	{
		local vector extent;
		if(bDuck > 0)
		{
			ePawn.bWantsToCrouch = !ePawn.bWantsToCrouch;
			ePawn.PlayZipLineTransition(ePawn.bWantsToCrouch);
			bDuck = 0;
		}
	}

    function PlayerMove( float DeltaTime )
    {
		local vector localSpeed;
		local float speed, minSpeed, maxSpeed;

		PawnLookAt();
		CheckForCrouch();

		speed = VSize(ePawn.Velocity);
		maxSpeed = eGame.m_ZLSlideDownMaxSpeed;
		minSpeed = eGame.m_ZLSlideDownMinSpeed;
		if(ePawn.bWantsToCrouch)
		{
			maxSpeed *= 2.0;
			minSpeed *= 2.0;
		}

		if(bPressedJump)
		{
			ePawn.SetPhysics(PHYS_Falling);
			GoToState('s_PlayerJumping');
			return;
		}

		if(aForward < eGame.m_backwardGentle)
			speed = DampFloat(speed, minSpeed, eGame.m_ZLSlideDownInertia, Level.m_dT);
		else
			speed = DampFloat(speed, maxSpeed, eGame.m_ZLSlideDownInertia, Level.m_dT);

		ePawn.Velocity = ePawn.m_geoNormal * speed;
		ePawn.Acceleration = ePawn.Velocity;

		ePawn.PlayZipLineWait(ePawn.bWantsToCrouch);
    }
}

state s_GrabbingPole extends s_GrabbingGE
{
	ignores Fire;

    function BeginState()
    {
		KillPawnSpeed();
		bInTransition = true;
		m_SMInTrans = false;
		ePawn.SetPhysics(PHYS_Linear);
		ePawn.bWantsToCrouch = false;
		m_camera.SetMode(ECM_FSphere);
		Super.BeginState();
    }

    function EndState()
    {
		bInTransition = false;
		m_LastNLPipeTime = Level.TimeSeconds;
    }

	event bool NotifyHitWall(vector HitNormal, actor Wall)
	{
		GoToState('s_PlayerJumping');
		return true;
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		GoToState('s_PlayerWalking');
		return true;
	}

    function PlayerMove( float DeltaTime )
    {
		if(m_SMInTrans && IsPushingFull() && ((m_EnterDir dot GetPushingDir()) < -0.7))
		{
			bIntransition = false;
			m_SMInTrans = false;
			GotoState(, 'OutFast');
		}
		ePawn.Acceleration = DampVec(ePawn.Acceleration, vect(0,0,0), 400.0, DeltaTime);
	}

OutFast:
	ePawn.PlayAnimOnly('RopeStAlIOR', , , true, true);
	FinishAnim();
	bIntransition = false;
	GoToState('s_PlayerWalking');

FromGround:
	ePawn.m_climbingUpperHand = CHRIGHT;
	ePawn.PlayPole('RopeStAlIOR', 'RopeStAlIOL', 0.1, false);
	MoveToDestination(ArrivalSpeedApprox(0.5));
	m_SMInTrans = true;
	FinishAnim(0);
	m_SMInTrans = false;
	GoToState('s_Pole');

FromAir:
	ePawn.m_climbingUpperHand = CHRIGHT;
	ePawn.PlayPole('RopeStAlBg0', 'RopeStAlBg0', 0.1, false);
	MoveToDestination(ArrivalSpeedApprox(2.0));
	FinishAnim(0);
	GoToState('s_Pole');
}

state s_Pole
{
	ignores Fire;
    function BeginState()
    {
		bInTransition = false;
		m_GEgoingUp = false;
		ePawn.SetPoleZones();
		m_camera.SetMode(ECM_FSphere);
    }

    function EndState()
    {
		bInTransition = false;
		m_GEgoingUp = false;
        ePawn.ResetZones();
		m_LastNLPipeTime = Level.TimeSeconds;
		ElapsedTime = 0.0f;
		if (bIsPlaying)
		{
			ePawn.PlaySound( Sound'FisherVoice.Stop_Sq_FisherBreathLong', SLOT_SFX );
			bIsPlaying = false;
		}
    }

	event bool Pole(vector locationEnd, rotator orientationEnd, int RType)
	{
		m_GECanGoForward = (RType != 1);
		if((!bInTransition) && (RType != 1) && (VSize(ePawn.Velocity) > 0.0))
		{
			ePawn.SetLocation(locationEnd);
			ePawn.SetRotation(orientationEnd);
		}
		return false;
	}

	event NoGE()
	{
		GoToState('s_PlayerJumping');
	}

	event bool NotifyLanded(vector HitNormal, Actor HitActor)
	{
		GoToState('s_PlayerWalking');
		return true;
	}

	function PlayerTick( float deltaTime )
	{
		Super.PlayerTick(DeltaTime);

		HandleBreathSounds(deltaTime);
	}

    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z, localSpeed, forwardDir;
		local rotator turnRotator;
		local bool turning;
		local float deltaYaw;

		PawnLookAt();
		if( bInTransition )
			return;

        GetAxes(ePawn.Rotation,X,Y,Z);

		if(Abs(aStrafe) > eGame.m_forwardGentle)
		{
			if(aStrafe > 0)
				turnRotator.Yaw = eGame.m_PLRotationSpeed * DeltaTime;
			else
				turnRotator.Yaw = -eGame.m_PLRotationSpeed * DeltaTime;

			turnRotator.Pitch = 0;
			turnRotator.Roll = 0;
			forwardDir = X * ePawn.m_PoleArmsZone.X;
			ePawn.Move(forwardDir - (forwardDir << turnRotator));

			forwardDir = ePawn.m_geoBottomPoint - ePawn.Location;
			forwardDir.Z = 0.0;
			turnRotator = Rotator(forwardDir);
			deltaYaw = turnRotator.Yaw - ePawn.Rotation.Yaw;
			ePawn.SetRotation(turnRotator);

			if((!m_GEgoingUp) && ((Abs(aForward) < eGame.m_forwardGentle) || ((aForward > eGame.m_forwardGentle) && (!m_GECanGoForward))) && deltaYaw != 0)
			{
				ePawn.Acceleration = vect(0, 0, 0);

				if(aStrafe < 0)
					ePawn.PlayPole('RopeStAlTrR', 'RopeStAlTrL', 0.2, false, false);
				else
					ePawn.PlayPole('RopeStAlTrR', 'RopeStAlTrL', 0.2, false, true);

				turning = true;
			}
		}

		if(bPressedJump)
		{
			JumpRelease();
		}
		else if( m_GEgoingUp )
		{
			if(m_GECanGoForward)
				ePawn.Acceleration = DampVec(ePawn.Acceleration, Z * eGame.m_PLUpwardSpeed, 120, deltaTime);
			else
				ePawn.Acceleration = vect(0, 0, 0);

			return;
		}
		else if(aForward > eGame.m_forwardGentle &&
				m_GECanGoForward &&
				ePawn.FastPointCheck(ePawn.Location + vect(0,0,5.0), ePawn.GetExtent(), true, true))
		{
			if(VSize(ePawn.Acceleration) == 0)
				ePawn.PlayPole('RopeStAlUpR', 'RopeStAlUpL', 0.2, true);
			else
				ePawn.PlayPole('RopeStAlUpR', 'RopeStAlUpL', 0.0, true);
			ePawn.Acceleration = DampVec(ePawn.Acceleration, Z * eGame.m_PLUpwardSpeed, 120, deltaTime);
			GoToState(, 'MoveUp');
		}
		else if(aForward < eGame.m_backwardGentle)
		{
			if(ePawn.FastPointCheck(ePawn.Location + vect(0,0,-10.0), ePawn.GetExtent(), true, true))
				GoToState('s_PoleSlideDown');
			else
				GoToState(, 'OutBottom');
		}
		else if(!turning)
		{
			ePawn.PlayPole('RopeStAlNtR', 'RopeStAlNtL', 0.2, false);
			ePawn.Acceleration = vect(0, 0, 0);
		}
    }

OutBottom:
	bInTransition = true;
	ePawn.PlayPole('RopeStAlIOR', 'RopeStAlIOL', 0.2, false, true);
	FinishAnim(0);
	bInTransition = false;
	GoToState('s_PlayerWalking');
	Stop;

MoveUp:
	m_GEgoingUp = true;
	FinishAnim();
	m_GEgoingUp = false;
}

state s_PoleSlideDown extends s_SlideDownBase
{
	ignores Fire;
    function BeginState()
    {
		Super.BeginState();
		ePawn.SetPoleZones();
		ePawn.PlaySound(Sound'FisherFoley.Play_FisherSlideTube', SLOT_SFX);
		m_camera.SetMode(ECM_FSphere);
    }
    function EndState()
    {
		KillPawnSpeed();
		Super.EndState();
		m_GEgoingUp = false;
		ePawn.StopSound(Sound'FisherFoley.Play_FisherSlideTube');
    }

	event bool Pole(vector locationEnd, rotator orientationEnd, int RType)
	{
		if(RType == 3)
		{
			ePawn.SetPhysics(PHYS_Falling);
			GoToState('s_PlayerJumping');
		}
		ePawn.SetLocation(locationEnd);

		ePawn.SetRotation(orientationEnd);
		return false;
	}

    function PlayerMove( float DeltaTime )
    {
		local vector X, Y, Z, localSpeed, forwardDir;
		local rotator turnRotator;

        GetAxes(ePawn.Rotation,X,Y,Z);
		PawnLookAt();

		m_SMTemp = ePawn.Acceleration.Z;

		if(bInTransition)
			return;

		if(Abs(aStrafe) > eGame.m_forwardGentle)
		{
			if(aStrafe > 0)
				turnRotator.Yaw = eGame.m_PLRotationSpeed * DeltaTime;
			else
				turnRotator.Yaw = -eGame.m_PLRotationSpeed * DeltaTime;

			turnRotator.Pitch = 0;
			turnRotator.Roll = 0;
			forwardDir = X * ePawn.m_PoleArmsZone.X;
			ePawn.Move(forwardDir - (forwardDir << turnRotator));

			forwardDir = ePawn.m_geoBottomPoint - ePawn.Location;
			forwardDir.Z = 0.0;
			ePawn.SetRotation(Rotator(forwardDir));
		}

		if(bPressedJump)
		{
			JumpRelease();
		}
		else if(aForward < eGame.m_backwardGentle)
		{
			// Acceleration
			ePawn.Velocity.Z = FMax(	ePawn.Velocity.Z - (eGame.m_PLSlideDownInertia * DeltaTime),
									-eGame.m_PLSlideDownMaxSpeed);
			ePawn.Acceleration = Z * ePawn.Velocity.Z;
		}
		else
		{
			// Deceleration
			ePawn.Velocity.Z += (eGame.m_PLSlideDownInertia * DeltaTime);
			ePawn.Acceleration = Z * ePawn.Velocity.Z;

			if(ePawn.Velocity.Z > -eGame.m_PLSlideDownMinSpeed)
				GoToState('s_Pole');
		}
	}

landed:
	bInTransition = true;
	if(!ePawn.PlayNLPLand(bDuck > 0, m_SMTemp))
		ePawn.PlayPole('RopeStAlIOR', 'RopeStAlIOL', 0.1, false, true);
	FinishAnim(0);
	bInTransition = false;
	GoToState('s_PlayerWalking');

begin:
	ePawn.PlayPole('RopeStAlNtR', 'RopeStAlNtL', 0.2, false);
	if(m_SMTemp < -400)
		ePawn.Acceleration.Z = m_SMTemp * 0.5;
}

//---------------------------------------[David Kalina - 17 Apr 2001]-----
//
// Description
//		RIGHT NOW, just some hacky shit to restart the level
//
//------------------------------------------------------------------------
state s_Dead
{
	Ignores Fire, ProcessHeadSet, ReturnFromInteraction, OnGroundScope, UsePalm; // OnGroundScope from air camera

	event bool CanSaveGame()
	{
		return false;
	}

	function BeginState()
	{
		// Prevents having another EndMission overwrite this one, set failure type
		iGameOverMsg = 0;

		ePawn.bCollideSB = false;

		// Remove Sam's vision
		SetCameraMode(self, 5/*REN_DynLight*/);

		DiscardPickup();

		if(m_camera.IsBTW())
			ePawn.bBatmanHack = true;

		if(!(m_camera.IsBTW() && m_camera.m_volSize == EVS_Minute))
			m_camera.SetMode(ECM_FSphere);

		// reset all channels
		ePawn.AnimBlendToAlpha(EPawn.BLENDMOVEMENTCHANNEL,0,0.10);
		ePawn.AnimBlendToAlpha(EPawn.REFPOSECHANNEL,0,0.10);
		ePawn.AnimBlendToAlpha(EPawn.ACTIONCHANNEL,0,0.10);

		SetTimer(4, false);
	}

	function Timer()
	{
		// End this
		EndMission(false, 0);
	}

Begin:
	Sleep(0.5f);
	// PlayTest temp
	if(eGame.bDemoMode && myHUD.GetStateName() != 's_QuickLoadMenu')
	{
		EMainHUD(myHUD).SaveState();
		myHUD.GotoState('s_QuickLoadMenu');
	}
}

defaultproperties
{
    bDebugNavPoints=true
    m_ThrowMinSpeed=(X=800.000000,Y=0.000000,Z=100.000000)
    m_ThrowMaxSpeed=(X=1500.000000,Y=0.000000,Z=250.000000)
    m_ThrowVarSpeed=1000.000000
    m_speedRun=420.000000
    m_speedRunCr=299.000000
    m_speedWalk=120.000000
    m_speedWalkCr=60.000000
    m_speedWalkFP=120.000000
    m_speedWalkFPCr=75.000000
    m_speedWalkSniping=100.000000
    m_speedBTWCr=75.000000
    m_speedBTW=130.000000
    m_speedFence=70.000000
    m_speedGrab=80.000000
    m_speedGrabFP=60.000000
    m_speedCarry=130.000000
    m_turnMul=0.500000
    m_towardAngle=0.707000
}