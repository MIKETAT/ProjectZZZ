// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/UCombatDodgeAction.h"

#include "Character/CharacterFrameDataBus.h"

UAnimMontage* UUCombatDodgeAction::GetAnimMontage(const FCharacterFrameDataBus& Data) const
{
	FVector2D Input = Data.PlayerInputs.RawMovementInput;
	if (Input.IsNearlyZero())
	{
		return Montage;
	}

	float InputAngle = FMath::RadiansToDegrees(FMath::Atan2(Input.X, Input.Y));
	for (const auto& Entry : DodgeEntries)
	{
		// Edge Case. Across border
		if (Entry.MaxAngle < Entry.MinAngle)
		{
			if (InputAngle >= Entry.MinAngle || InputAngle <= Entry.MaxAngle)
			{
				return Entry.Montage;
			}
		}
		// common case
		else
		{
			if (InputAngle >= Entry.MinAngle && InputAngle <= Entry.MaxAngle)
			{
				return Entry.Montage;
			}
		}
	}
	// Default Value
	return Montage;
}
