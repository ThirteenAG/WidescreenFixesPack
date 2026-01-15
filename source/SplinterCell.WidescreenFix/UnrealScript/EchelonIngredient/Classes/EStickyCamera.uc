class EStickyCamera extends EAirCamera;

#exec OBJ LOAD FILE=..\Sounds\FisherEquipement.uax

var(StickyCamera) float MinFov;
var(StickyCamera) float MaxFov;
var(StickyCamera) float ZoomSpeed;
var(StickyCamera) float Damping;

var float current_fov;

function PostBeginPlay()
{
	Super.PostBeginPlay();

    HUDTex       = EchelonLevelInfo(Level).TICON.qi_ic_stickycam_surv;
    HUDTexSD     = EchelonLevelInfo(Level).TICON.qi_ic_stickycam_surv_sd;
    InventoryTex = EchelonLevelInfo(Level).TICON.inv_ic_stickycam_surv;
    ItemName     = "StickyCamera";
	ItemVideoName = "gd_sticky_cam.bik";
    Description  = "StickyCameraDesc";
	HowToUseMe  = "StickyCameraHowToUseMe";

	current_fov	 = MaxFov;
}

state s_Camera
{
	function BeginState()
	{
		Super.BeginState();

		current_fov			= MaxFov;
	}

	function Tick( float DeltaTime )
	{
		local bool zoomed;
        // Night vision
        if( Epc.bDPadLeft != 0 )
		{
			if( RenderingMode != REN_NightVision )
			{
				if( RenderingMode != REN_ThermalVision )
					PlaySound(Sound'FisherEquipement.Play_GoggleRun', SLOT_SFX);
				RenderingMode = REN_NightVision;
			}
			else
				RenderingMode = REN_DynLight;

			Epc.SetCameraMode(self, RenderingMode);
			Epc.bDPadLeft	= 0;
		}
		// Thermal vision
		else if( Epc.bDPadRight != 0 )
		{
			if( RenderingMode != REN_ThermalVision )
			{
				if( RenderingMode != REN_NightVision )
					PlaySound(Sound'FisherEquipement.Play_GoggleRun', SLOT_SFX);
				RenderingMode = REN_ThermalVision;
			}
			else
				RenderingMode = REN_DynLight;

			Epc.SetCameraMode(self, RenderingMode);

	        Epc.ThermalTexture	= Level.pThermalTexture_B;
            Epc.bBigPixels		= true;
			Epc.bDPadRight		= 0;
		}

		// Zoom in
        if( Epc.bIncSpeedPressed == true )
		{
			Epc.bIncSpeedPressed=false;
			current_fov -= DeltaTime * ZoomSpeed;
			if ( current_fov >= MinFov )
			{
				zoomed = true;

			}
		}
		// Zoom out
		else if( Epc.bDecSpeedPressed == true )
		{
			Epc.bDecSpeedPressed=false;
			current_fov += DeltaTime * ZoomSpeed;
			if ( current_fov <= MaxFov )
			{
				zoomed = true;
			}
		}

		if(zoomed)
		{
			if (!IsPlaying(Sound'FisherEquipement.Play_StickyCamZoom'))
				PlaySound(Sound'FisherEquipement.Play_StickyCamZoom', SLOT_SFX);
			Level.RumbleVibrate(0.07f, 0.5);
			if(FRand() > 0.5)
				Epc.m_camera.Hit(60, 20000, true);
			else
				Epc.m_camera.Hit(-60, 20000, true);
		}

		// Clamp fov and calculate zoom factor
        current_fov = FClamp(current_fov, MinFov, MaxFov);
		MaxDamping = Damping;
		MaxDamping /= (MaxFov)/current_fov;

		// Modify vision fov
		Epc.SetCameraFOV(self, current_fov);

        Super.Tick(DeltaTime);
	}
}

defaultproperties
{
    MinFov=10.000000
    MaxFov=90.000000
    ZoomSpeed=95.000000
    Damping=400.000000
    MaxQuantity=20
    ObjectHudClass=Class'EStickyView'
    StaticMesh=StaticMesh'EMeshIngredient.Item.StickyCamera'
}