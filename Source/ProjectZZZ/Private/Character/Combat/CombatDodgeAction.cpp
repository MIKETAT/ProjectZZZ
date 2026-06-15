// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/CombatDodgeAction.h"

#include "Character/CharacterFrameDataBus.h"

UAnimMontage* UCombatDodgeAction::GetAnimMontage(const FVector2D& MovementInput) const
{
	if (MovementInput.IsNearlyZero())
	{
		return DefaultDodgeMontage;
	}

	float InputAngle = FMath::RadiansToDegrees(FMath::Atan2(MovementInput.X, MovementInput.Y));
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
	return DefaultDodgeMontage;
}
