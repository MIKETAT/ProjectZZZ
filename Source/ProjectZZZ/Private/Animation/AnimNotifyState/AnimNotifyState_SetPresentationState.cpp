#include "Animation/AnimNotifyState/AnimNotifyState_SetPresentationState.h"

void UAnimNotifyState_SetPresentationState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
	{
		PreviousState = Character->GetCharacterState();
		Character->SetCharacterState(State);
		UE_LOG(LogTemp, Error, TEXT("Enter State: %s"), *UEnum::GetValueAsString(State));
	}
}

void UAnimNotifyState_SetPresentationState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
	{
		Character->SetCharacterState(PreviousState);
		UE_LOG(LogTemp, Error, TEXT("Enter State: %s"), *UEnum::GetValueAsString(PreviousState));
	}
}
