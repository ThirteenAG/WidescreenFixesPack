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
