// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/CharacterBase.h"
#include "AbilitySystem/AgentAbilitySystemComponent.h"
#include "AbilitySystem/AgentAttributeSet.h"
#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "Animation/Component/CombatAnimSchedulerComponent.h"
#include "Character/Component/CharacterCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


ACharacterBase::ACharacterBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 80.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 850.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CombatComponent = CreateDefaultSubobject<UCharacterCombatComponent>(TEXT("CombatComponent"));
	CombatAnimSchedulerComponent = CreateDefaultSubobject<UCombatAnimSchedulerComponent>(TEXT("CombatAnimSchedulerComponent"));

	AgentAbilitySystemComponent = CreateDefaultSubobject<UAgentAbilitySystemComponent>(TEXT("AgentAbilitySystemComponent"));
	AgentAbilitySystemComponent->SetIsReplicated(true);
	BaseCombatAttribute = CreateDefaultSubobject<UBaseCombatAttributeSet>(TEXT("BaseCombatAttributeSet"));
}

void ACharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Refresh States
	RefreshLocomotionState(DeltaTime);
	RefreshInput(DeltaTime);
}

void ACharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AgentAbilitySystemComponent)
	{
		AgentAbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (CombatComponent)
		{
			CombatComponent->InjectAndBindASC(AgentAbilitySystemComponent);
		}
	}
	InitializeAttributes();
}

void ACharacterBase::UnPossessed()
{
	Super::UnPossessed();
}

void ACharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ACharacterBase::RefreshInput(const float DeltaTime)
{
	auto InputDirection{GetCharacterMovement()->GetCurrentAcceleration() / GetCharacterMovement()->GetMaxAcceleration()};
	bHasMovementInput = InputDirection.GetSafeNormal().SizeSquared() > UE_KINDA_SMALL_NUMBER;
}

void ACharacterBase::RefreshLocomotionState(const float DeltaTime)
{
	LocomotionState.WorldLocation = GetActorLocation();
	LocomotionState.WorldRotation = GetActorRotation();
	LocomotionState.WorldVelocity = GetVelocity();
	LocomotionState.WorldVelocity2D = FVector{LocomotionState.WorldVelocity.X, LocomotionState.WorldVelocity.Y, 0.0f};

	const FRotator YawRotation{0.f, LocomotionState.WorldRotation.Yaw, 0.f};
	
	LocomotionState.LocalVelocity2D = YawRotation.UnrotateVector(LocomotionState.WorldVelocity2D);
	
	const FVector WorldAcceleration{GetCharacterMovement()->GetCurrentAcceleration()};
	
	LocomotionState.WorldAcceleration2D = FVector{WorldAcceleration.X, WorldAcceleration.Y, 0.0f};
	LocomotionState.LocalAcceleration2D = YawRotation.UnrotateVector(LocomotionState.WorldAcceleration2D); 
}

void ACharacterBase::InitializeAttributes()
{
	ApplyGameplayEffectToSelf(BaseInitGE);
	ApplyGameplayEffectToSelf(GetExclusiveInitGE());
}

void ACharacterBase::ApplyGameplayEffectToSelf(const TSubclassOf<UGameplayEffect>& Effect)
{
	if (AgentAbilitySystemComponent && IsValid(Effect))
	{
		FGameplayEffectContextHandle ContextHandle = AgentAbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		if (FGameplayEffectSpecHandle SpecHandle = AgentAbilitySystemComponent->MakeOutgoingSpec(Effect, 1, ContextHandle); SpecHandle.IsValid())
		{
			AgentAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}
