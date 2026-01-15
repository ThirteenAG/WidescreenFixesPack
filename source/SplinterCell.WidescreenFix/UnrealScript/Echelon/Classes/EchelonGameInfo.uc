//=============================================================================
// EchelonGameInfo.
//
// default game info is normal single player
//
//=============================================================================
class EchelonGameInfo extends GameInfo
    native;



var EchelonLevelInfo    ELevel;
var EPlayerController	pPlayer;
var EVariable		    VarObject;

var	int					NumVisChecks;					// number of visibility checks done in this frame

var()  int HUD_OFFSET_X;      // Offset from left and right side of the screen
var()  int HUD_OFFSET_Y;      // Offset from top and bottom side of the screen
var()  bool bDemoMode;        // Set this variable at true to set the menu in demo mode
var()  bool bNoGore;
var()  bool bNoSamLookAt;
var()  string DemoMap;        // Map loaded for Demo
var()  string TrainingMap;    // Map loaded for Training
var    bool bStartGame;       // true only the first time you go in main menu

//=============================================================================
// Variables used as constants

var()		float	m_minInterpolSpeed;
var() 		float	m_grabbingDelay;
var() 		float	m_blinkDelay;

// Input


var(Input)  int     m_maxSpeedInterval;
var(Input)  int     m_defautSpeed;

var(Input) 	float	m_forwardGentle;
var(Input) 	float	m_forwardCrawl;
var(Input) 	float	m_forwardFull;
var(Input) 	float	m_backwardGentle;
var(Input) 	float	m_backwardCrawl;
var(Input) 	float	m_backwardFull;
var(Input) 	float	m_rightGentle;
var(Input) 	float	m_rightCrawl;
var(Input) 	float	m_rightFull;
var(Input) 	float	m_leftGentle;
var(Input) 	float	m_leftCrawl;
var(Input) 	float	m_leftFull;
var(Input) 	float	m_minForce;
var(Input) 	float	m_gentleForce;
var(Input) 	float	m_fullForce;
var(Input) 	float	m_inAirAccel;
var(Input) 	float	m_onGroundAccel;
var(Input) 	float	m_RollSpeed;
var(Input) 	float	m_walkBias;

var(Input) 	float	m_PlayerJoggingThreshold;
var()	   	float	m_JumpOutSpeed;

// GEO
var(Ledge) 	float	m_LGShimmySpeed;
var(Ledge) 	float	m_LGSpeedThreshold;
var(Ledge) 	float	m_LGMinLength;
var(Ledge) 	float	m_LGMaxGrabbingSpeed;
var(Ledge) 	float	m_LGMinGrabbingSpeed;
var(Ledge) 	float	m_LGMaxGrabbingAngle;
var(Ledge) 	float	m_LGMaxGrabbingDistance;
var(Ledge) 	float	m_LGMaxFallGrabSpeed;

var(Pole) 	float	m_PLMaxGrabbingSpeed;
var(Pole) 	float	m_PLMinGrabbingSpeed;
var(Pole) 	float	m_PLMaxGrabbingDistance;
var(Pole) 	float	m_PLMaxGrabbingAngle;
var(Pole) 	float	m_PLSlideDownMinSpeed;
var(Pole) 	float	m_PLSlideDownMaxSpeed;
var(Pole) 	float	m_PLSlideDownInertia;
var(Pole) 	float	m_PLUpwardSpeed;
var(Pole) 	float	m_PLFeetOffset;
var(Pole) 	float	m_PLRotationSpeed;

var(HOH) 	float	m_HOHForwardSpeed;
var(HOH) 	float	m_HOHMinLength;
var(HOH) 	float	m_HOHMaxGrabbingSpeed;
var(HOH) 	float	m_HOHMinGrabbingSpeed;
var(HOH) 	float	m_HOHMaxGrabbingDistance;
var(HOH) 	float	m_HOHFeetUpGap;
var(HOH) 	float	m_HOHFeetUpColHoriOffset;
var(HOH) 	float	m_HOHFeetUpColVertOffset;
var(HOH) 	float	m_HOHFeetUpMoveSpeed;
var(HOH) 	float	m_HOHFeetUpColHeight;

var(Ladder) 	float	m_NLUpwardSpeed;
var(Ladder) 	float	m_NLDownwardSpeed;
var(Ladder) 	float	m_NLStepSize;
var(Ladder) 	float	m_NLSlideDownMaxSpeed;
var(Ladder) 	float	m_NLSlideDownMinSpeed;
var(Ladder) 	float	m_NLSlideDownInertia;
var(Ladder) 	float	m_NLMaxGrabbingAngle;
var(Ladder) 	float	m_NLMaxGrabbingAngleTop;
var(Ladder) 	float	m_NLMaxGrabbingDistance;
var(Ladder) 	float	m_NLMaxGrabbingSpeed;
var(Ladder) 	float	m_NLMinGrabbingSpeed;
var(Ladder) 	float	m_NLSlideDelay;

var(Pipe) 	float	m_PUpwardSpeed;
var(Pipe) 	float	m_PDownwardSpeed;
var(Pipe) 	float	m_PSlideDownMaxSpeed;
var(Pipe) 	float	m_PSlideDownMinSpeed;
var(Pipe) 	float	m_PSlideDownInertia;
var(Pipe) 	float	m_PMaxGrabbingAngle;
var(Pipe) 	float	m_PMaxGrabbingAngleTop;
var(Pipe) 	float	m_PMaxGrabbingDistance;
var(Pipe) 	float	m_PMaxGrabbingSpeed;
var(Pipe) 	float	m_PMinGrabbingSpeed;

var(ZipLine) 	float	m_ZLSlideDownMaxSpeed;
var(ZipLine) 	float	m_ZLSlideDownMinSpeed;
var(ZipLine) 	float	m_ZLSlideDownInertia;
var(ZipLine) 	float	m_ZLMaxGrabbingDistance;
var(ZipLine) 	float	m_ZLMaxGrabbingSpeed;
var(ZipLine) 	float	m_ZLMinGrabbingSpeed;

var(Fence) 	float	m_FMaxGrabbingSpeed;
var(Fence) 	float	m_FMinGrabbingSpeed;

var(DamageHeight)  float MinBeforeDamage;
var(DamageHeight)  float MaxBeforeDeath;
var(DamageHeight)  float NPCCushionDivider;

var(SurfaceNoise) SurfaceNoiseInfo VeryQuietSurface;
var(SurfaceNoise) SurfaceNoiseInfo QuietSurface;
var(SurfaceNoise) SurfaceNoiseInfo NormalSurface;
var(SurfaceNoise) SurfaceNoiseInfo LoudSurface;
var(SurfaceNoise) SurfaceNoiseInfo VeryLoudSurface;

var bool bUseController; // Joshua - Adjusts HUD, inventory, lockpicking, keypad, turrets for controller


//=============================================================================

//----------------------------------------[FSchelling - 15 June 2001]-----
function PostBeginPlay()
{
	local class<EVariable> VarObjectClassName;

	Super.PostBeginPlay();

	//load the the specific variable class
	VarObjectClassName = class<EVariable>(DynamicLoadObject("EchelonPattern.VGame", class'Class'));

	if(VarObjectClassName != None)
	{
		//spawn variable class
		VarObject = spawn(VarObjectClassName,self);

		//log("Variable class for the game spawned: "$VarObject);
	}

	// set EchelonLevelInfo reference
	ELevel = EchelonLevelInfo(Level);

	if ( ELevel == none )
		log("ECHELON LEVEL INFO COULD NOT BE SET");

}

//---------------------------------------[David Kalina - 12 Apr 2001]-----
//
// Description
//		Spawns and logs in the Player Controller.
//		Since this is an Echelon Game, we make sure the controller casts
//		to an EPlayerController and store it for easy access.
//
// Input
//		Portal :
//		Options :
//		Error :
//
// Output
//		PlayerController :
//
//------------------------------------------------------------------------

event PlayerController Login(string Portal,
					         string Options,
					         out string Error)
{
	local PlayerController NewPlayer;

	NewPlayer = Super.Login(Portal, Options, Error);
	Log("Echelon -- New Player super login");

	pPlayer = EPlayerController(NewPlayer);    // set gameinfo reference to Echelon Player type

	//for the engine
	PlayerC = NewPlayer;

	if (pPlayer == none)
		Log("WARNING!!!!!!   EchelonGameInfo.pPlayer is NOT an EPlayerController.  This is bad.");

	pPlayer.m_curWalkSpeed = m_defautSpeed;

	return NewPlayer;
}

native(2345) final function string GetStringBinding(EchelonEnums.eKEY_BIND Key);
native(2346) final function EchelonEnums.eKEY_BIND GetKeyBinding(string Key);

defaultproperties
{
    HUD_OFFSET_X=24
    HUD_OFFSET_Y=39
    DemoMap="3_4_3Severonickel"
    TrainingMap="0_0_2_Training"
    bStartGame=true
    m_minInterpolSpeed=200.000000
    m_grabbingDelay=0.200000
    m_blinkDelay=3.000000
    m_maxSpeedInterval=5
    m_defautSpeed=1
    m_forwardGentle=0.500000
    m_forwardCrawl=0.100000
    m_forwardFull=0.900000
    m_backwardGentle=-0.500000
    m_backwardCrawl=-0.100000
    m_backwardFull=-0.900000
    m_rightGentle=0.500000
    m_rightCrawl=0.100000
    m_rightFull=0.900000
    m_leftGentle=-0.500000
    m_leftCrawl=-0.100000
    m_leftFull=-0.900000
    m_minForce=0.020000
    m_gentleForce=0.500000
    m_fullForce=0.900000
    m_inAirAccel=1000.000000
    m_onGroundAccel=2000.000000
    m_RollSpeed=350.000000
    m_walkBias=0.300000
    m_PlayerJoggingThreshold=300.000000
    m_JumpOutSpeed=200.000000
    m_LGShimmySpeed=50.000000
    m_LGSpeedThreshold=10.000000
    m_LGMinLength=28.000000
    m_LGMaxGrabbingSpeed=200.000000
    m_LGMinGrabbingSpeed=-1000.000000
    m_LGMaxGrabbingAngle=-0.600000
    m_LGMaxGrabbingDistance=1000.000000
    m_LGMaxFallGrabSpeed=200.000000
    m_PLMaxGrabbingSpeed=200.000000
    m_PLMinGrabbingSpeed=-1000.000000
    m_PLMaxGrabbingDistance=35.000000
    m_PLMaxGrabbingAngle=-0.600000
    m_PLSlideDownMinSpeed=200.000000
    m_PLSlideDownMaxSpeed=600.000000
    m_PLSlideDownInertia=600.000000
    m_PLUpwardSpeed=50.000000
    m_PLFeetOffset=80.000000
    m_PLRotationSpeed=20000.000000
    m_HOHForwardSpeed=100.000000
    m_HOHMinLength=15.000000
    m_HOHMaxGrabbingSpeed=200.000000
    m_HOHMinGrabbingSpeed=-1000.000000
    m_HOHMaxGrabbingDistance=1500.000000
    m_HOHFeetUpGap=100.000000
    m_HOHFeetUpColHoriOffset=45.000000
    m_HOHFeetUpColVertOffset=60.000000
    m_HOHFeetUpMoveSpeed=40.000000
    m_HOHFeetUpColHeight=18.000000
    m_NLUpwardSpeed=120.000000
    m_NLDownwardSpeed=-120.000000
    m_NLStepSize=32.000000
    m_NLSlideDownMaxSpeed=600.000000
    m_NLSlideDownMinSpeed=200.000000
    m_NLSlideDownInertia=600.000000
    m_NLMaxGrabbingAngle=-0.600000
    m_NLMaxGrabbingAngleTop=0.300000
    m_NLMaxGrabbingDistance=35.000000
    m_NLMaxGrabbingSpeed=100.000000
    m_NLMinGrabbingSpeed=-1000.000000
    m_NLSlideDelay=0.200000
    m_PUpwardSpeed=130.000000
    m_PDownwardSpeed=-130.000000
    m_PSlideDownMaxSpeed=600.000000
    m_PSlideDownMinSpeed=200.000000
    m_PSlideDownInertia=600.000000
    m_PMaxGrabbingAngle=-0.600000
    m_PMaxGrabbingAngleTop=0.300000
    m_PMaxGrabbingDistance=35.000000
    m_PMaxGrabbingSpeed=100.000000
    m_PMinGrabbingSpeed=-1000.000000
    m_ZLSlideDownMaxSpeed=400.000000
    m_ZLSlideDownMinSpeed=200.000000
    m_ZLSlideDownInertia=400.000000
    m_ZLMaxGrabbingDistance=1500.000000
    m_ZLMaxGrabbingSpeed=200.000000
    m_ZLMinGrabbingSpeed=-1000.000000
    m_FMaxGrabbingSpeed=200.000000
    m_FMinGrabbingSpeed=-1000.000000
    MinBeforeDamage=1250.000000
    MaxBeforeDeath=1500.000000
    NPCCushionDivider=2.000000
    VeryQuietSurface=(JogRadius=400.000000,CrouchJogRadius=225.000000,LandingRadius=500.000000)
    QuietSurface=(JogRadius=550.000000,CrouchJogRadius=308.000000,LandingRadius=650.000000)
    NormalSurface=(WalkRadius=200.000000,JogRadius=700.000000,CrouchJogRadius=392.000000,LandingRadius=800.000000)
    LoudSurface=(WalkRadius=350.000000,JogRadius=1100.000000,CrouchJogRadius=616.000000,LandingRadius=1200.000000)
    VeryLoudSurface=(WalkRadius=500.000000,JogRadius=1500.000000,CrouchWalkRadius=100.000000,CrouchJogRadius=840.000000,LandingRadius=1600.000000,QuietLandingRadius=200.000000)
    GameName="Echelon"
    PlayerControllerClassName="Echelon.EPlayerController"
}