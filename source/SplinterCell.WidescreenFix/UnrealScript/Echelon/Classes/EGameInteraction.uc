class EGameInteraction extends EInteraction;

#exec OBJ LOAD FILE=..\Sounds\Interface.uax

var EInteractObject ExitInteraction;
var bool bInteracting;

function bool KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
{
	local string actionName;
	actionName = FindAction(Key);

	if( Epc.Level.Pauser != None )
		return false;

	//=============================================================================
	// Joshua - Automatically toggle controller mode
	//=============================================================================
	if (Key == IK_Joy1  || Key == IK_Joy2  || Key == IK_Joy3  || Key == IK_Joy4  ||
		Key == IK_Joy5  || Key == IK_Joy6  || Key == IK_Joy7  || Key == IK_Joy8  ||
		Key == IK_Joy9  || Key == IK_Joy10 || Key == IK_Joy11 || Key == IK_Joy12 ||
		Key == IK_Joy13 || Key == IK_Joy14 || Key == IK_Joy15 || Key == IK_Joy16 ||
		Key == IK_JoyX  || Key == IK_JoyY  || Key == IK_JoyZ  || Key == IK_JoyR  ||
		Key == IK_JoyU  || Key == IK_JoyV  || Key == IK_AnalogUp || Key == IK_AnalogDown ||
		Key == IK_AnalogLeft || Key == IK_AnalogRight)
	{
		if (!Epc.eGame.bUseController)
		{
			Epc.eGame.bUseController = true;
			Epc.m_curWalkSpeed = 5;
		}
	}
	else if (Key != IK_MouseX && Key != IK_MouseY)
	{
		if (Epc.eGame.bUseController)
		{
			Epc.eGame.bUseController = false;
		}
	}

	if( Action == IST_Press || Action == IST_Hold )
	{

		//clauzon 9/17/2002 replaced a switch checking the key pressed by the mapped action test.
		if(actionName == "Interaction")
		{
			if( Epc.IManager.GetNbInteractions() > 0 &&
				Epc.CanInteract() &&
				!Epc.bStopInput )							// Prevent interacting in cinematic
			{
				bInteracting = true;

				// Go into GameInteraction menu
				//Log("Interaction pressed with"$Epc.IManager.GetNbInteractions()$" interaction on stack");
				GotoState('s_GameInteractionMenu');

				return true; // grabbed
			}
		}
	}

	//=============================================================================
	// Joshua - Bind controller input
	//=============================================================================
	Epc.SetKey("Joy1 Interaction", "");
	Epc.SetKey("Joy2 Duck", "");
	Epc.SetKey("Joy3 Scope", "");
	Epc.SetKey("Joy4 Jump", "");
	Epc.SetKey("Joy5 BackToWall", "");
	Epc.SetKey("Joy6 QuickInventory", "");
	Epc.SetKey("Joy7 AltFire", "");
	Epc.SetKey("Joy8 Fire", "");
	//Epc.SetKey("Joy9 None", "");
	//Epc.SetKey("Joy10 FullInventory", "");
	Epc.SetKey("Joy11 SwitchROF", "");
	Epc.SetKey("Joy12 ResetCamera", "");
	Epc.SetKey("Joy13 DPadUp", "");
	Epc.SetKey("Joy14 DPadDown", "");
	Epc.SetKey("Joy15 DPadLeft", "");
	Epc.SetKey("Joy16 DPadRight", "");
	Epc.SetKey("AnalogUp MoveForward", "");
	Epc.SetKey("AnalogDown MoveBackward", "");
	Epc.SetKey("AnalogLeft StrafeLeft", "");
	Epc.SetKey("AnalogRight StrafeRight", "");
	Epc.SetKey("JoyX \"Axis aStrafe DeadZone=0.3\"", "");
	Epc.SetKey("JoyY \"Axis aForward DeadZone=0.3\"", "");
	Epc.SetKey("JoyZ \"Axis aTurn DeadZone=0.3\"", "");
	Epc.SetKey("JoyV \"Axis aLookUp DeadZone=0.3\"", "");

	if (Epc.GetStateName() == 's_FirstPersonTargeting' ||
		Epc.GetStateName() == 's_SplitTargeting' ||
		Epc.GetStateName() == 's_RappellingTargeting' ||
		Epc.GetStateName() == 's_PlayerBTWTargeting' ||
		Epc.GetStateName() == 's_PlayerSniping' ||
		Epc.GetStateName() == 's_SplitSniping' ||
		Epc.GetStateName() == 's_RappellingSniping')
	{
		Epc.SetKey("Joy4 ZoomToggle", "");
	}

	return false; // continue input processing
}

state s_GameInteractionMenu
{
	function BeginState()
	{
		Epc.SetPause(true);
		Epc.IManager.SelectedInteractions = 1;

		// Add exit button, spawn it only the first time in
		if( ExitInteraction == None )
			ExitInteraction = Epc.IManager.spawn(class'EExitInteraction', Epc.IManager);
		Epc.IManager.ShowExit(ExitInteraction);

		//Log("Interaction menu in");
	}

	function EndState()
	{
		if( Epc.IManager.GetCurrentInteraction() == None )
			Log("ERROR: Interaction not valid on stack.");

		Epc.SetPause(false);

		//Log("Interaction menu .. Interaction released .. initInteract");
		Epc.IManager.GetCurrentInteraction().InitInteract(Epc);
		Epc.IManager.SelectedInteractions = -1;

		// Remove exit button
		Epc.IManager.ShowExit(None);
	}

	function bool KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta )
	{
		local string actionName;
		actionName = FindAction(Key);

		if( Action == IST_Press )
		{
			//clauzon 9/17/2002 replaced a switch checking the key pressed by the mapped action test.
			if(actionName=="MoveForward" || Key == IK_MouseWheelUp)
			{
				if( Epc.IManager.SelectNextItem() )
				{
					//Log("Interaction menu UP");
					Epc.EPawn.playsound(Sound'Interface.Play_ActionChoice', SLOT_Interface);
				}
			}
			else if(actionName == "MoveBackward" || Key == IK_MouseWheelDown)
			{
				if( Epc.IManager.SelectPreviousItem() )
				{
					//Log("Interaction menu DOWN");
					Epc.EPawn.playsound(Sound'Interface.Play_ActionChoice', SLOT_Interface);
				}
			}

		}
		else if( Action == IST_Release )
		{
			if(actionName=="Interaction")
			{
				bInteracting = false;
				// Exit GameInteraction menu
				//Log("Interaction released");
				GotoState('');
			}
		}

		return true;
	}
}
