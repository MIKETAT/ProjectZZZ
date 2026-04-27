// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/ZZZGameplayTag.h"

namespace Combat::Gait
{
	UE_DEFINE_GAMEPLAY_TAG(Idle, FName(TEXTVIEW("Combat.Gait.Idle")))
	UE_DEFINE_GAMEPLAY_TAG(Run, FName(TEXTVIEW("Combat.Gait.Run")))
	UE_DEFINE_GAMEPLAY_TAG(Sprint, FName(TEXTVIEW("Combat.Gait.Sprint")))
	UE_DEFINE_GAMEPLAY_TAG(Dodge, FName(TEXTVIEW("Combat.Gait.Dodge")))
}

namespace Combat::Status
{
	UE_DEFINE_GAMEPLAY_TAG(Daze, FName(TEXTVIEW("Combat.Status.Daze")))
}

/*namespace Combat::StatusTags
{
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_01, FName(TEXTVIEW("Combat.StatusTags.Basic_Attack_01")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_02, FName(TEXTVIEW("Combat.StatusTags.Basic_Attack_02")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_03, FName(TEXTVIEW("Combat.StatusTags.Basic_Attack_03")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_04, FName(TEXTVIEW("Combat.StatusTags.Basic_Attack_04")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_05, FName(TEXTVIEW("Combat.StatusTags.Basic_Attack_05")))
}*/

namespace Combat::CombatWindows
{
	UE_DEFINE_GAMEPLAY_TAG(InputBufferWindow, FName(TEXTVIEW("Combat.Window.InputBuffer")))
	UE_DEFINE_GAMEPLAY_TAG(ProceedWindow, FName(TEXTVIEW("Combat.Window.ProceedWindow")))
	UE_DEFINE_GAMEPLAY_TAG(IsRecoveryWindow, FName(TEXTVIEW("Combat.Window.IsRecoveryWindow")))
	UE_DEFINE_GAMEPLAY_TAG(ParryWindow, FName(TEXTVIEW("Combat.Window.ParryWindow")))
	UE_DEFINE_GAMEPLAY_TAG(MovementInterruptWindow, FName(TEXTVIEW("Combat.Window.MovementInterruptWindow")))
}

namespace Combat::Data
{
	UE_DEFINE_GAMEPLAY_TAG(DamageMultiplier, FName(TEXTVIEW("Combat.Data.DamageMultiplier")))
	UE_DEFINE_GAMEPLAY_TAG(DazeMultiplier, FName(TEXTVIEW("Combat.Data.DazeMultiplier")))
	UE_DEFINE_GAMEPLAY_TAG(StunDMGMultiplier, FName(TEXTVIEW("Combat.Data.StunDMGMultiplier")))
}

namespace Combat::Event
{
	UE_DEFINE_GAMEPLAY_TAG(Death, FName(TEXTVIEW("Combat.Event.Death")));
}
