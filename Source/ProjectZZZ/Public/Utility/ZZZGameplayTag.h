// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace Combat::Gait
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Idle)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Run)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sprint)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dodge)
}

namespace Combat::Status
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Daze)
}

/*namespace Combat::StatusTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_01)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_02)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_03)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_04)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_05)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_01)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic_Attack_01)
}*/

namespace Combat::CombatWindows
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputBufferWindow)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ProceedWindow)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(IsRecoveryWindow)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ParryWindow)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(MovementInterruptWindow)
}

namespace Combat::Data
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageMultiplier)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DazeMultiplier)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(StunDMGMultiplier)
}

namespace Combat::Event
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChainAttack)
}
