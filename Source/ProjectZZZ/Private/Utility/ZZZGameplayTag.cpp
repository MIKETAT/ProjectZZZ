// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/ZZZGameplayTag.h"


namespace Combat::CombatWindows
{
	UE_DEFINE_GAMEPLAY_TAG(InputBufferWindow, FName(TEXTVIEW("Combat.Window.InputBuffer")))
	UE_DEFINE_GAMEPLAY_TAG(ProceedWindow, FName(TEXTVIEW("Combat.Window.ProceedWindow")))
	UE_DEFINE_GAMEPLAY_TAG(IsRecoveryWindow, FName(TEXTVIEW("Combat.Window.IsRecoveryWindow")))
	UE_DEFINE_GAMEPLAY_TAG(ParryWindow, FName(TEXTVIEW("Combat.Window.ParryWindow")))
}
