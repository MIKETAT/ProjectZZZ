// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/EnemyCharacterBase.h"

#include "AbilitySystem/BaseCombatAttributeSet.h"
#include "AbilitySystem/EnemyAttributeSet.h"
#include "AI/EnemyCombatComponent.h"
#include "Character/Combat/CombatEventBusSubSystem.h"
#include "Character/Combat/ZZZCombatEventTypes.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utility/ZZZGameplayTag.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	EnemyAttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));

	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("EnemyCombatComponent"));
	CombatBase = EnemyCombatComponent;

	ParryFlashWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ParryFlashWidget"));
	if (ParryFlashWidget)
	{
		ParryFlashWidget->SetupAttachment(GetMesh(), ParrySocketName);
		ParryFlashWidget->SetWidgetSpace(EWidgetSpace::Screen);
	}

	GetCharacterMovement()->MaxWalkSpeed = 500.f;
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (AgentAbilitySystemComponent)
	{
		AgentAbilitySystemComponent->InitAbilityActorInfo(this, this);
		InitializeAttributes();
	}
} 

void AEnemyCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PrintDebugInfo();
}

void AEnemyCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemyCharacterBase::InitializeAttributes()
{
	ApplyGameplayEffectToSelf(BaseInitGE);
	ApplyGameplayEffectToSelf(EnemyExclusiveInitGE);
}

void AEnemyCharacterBase::Die()
{
	if (UCombatEventBusSubSystem* EventBus = GetWorld()->GetSubsystem<UCombatEventBusSubSystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("【怪物 %s】：啊！我死了！我正在向全宇宙广播我的死讯！"), *GetName());
		FTestDeathPayload Payload;
		EventBus->BroadcastEvent(Combat::Event::Death, this, this, this, Payload);
	}
}

void AEnemyCharacterBase::PrintDebugInfo()
{
	if (bPrintDebugInfo)
	{
		GEngine->ClearOnScreenDebugMessages();
		PrintAttributeSet(BaseCombatAttribute.Get());
		PrintAttributeSet(EnemyAttributeSet.Get());
	}
}

void AEnemyCharacterBase::PrintAttributeSet(UAttributeSet* Attribute)
{
	if (!GEngine || !Attribute)
	{
		return;
	}

	const UClass* SetClass = Attribute->GetClass();
	
	GEngine->AddOnScreenDebugMessage(
			-1,
			0.0f,
			FColor::Yellow,
			FString::Printf(TEXT("==== %s ===="), *SetClass->GetName()));

	int32 LineIndex = 0;

	for (TFieldIterator<FProperty> It(SetClass, EFieldIterationFlags::IncludeSuper); It; ++It)
	{
		const FProperty* Property = *It;
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);

		// 只处理 GAS 常用的 FGameplayAttributeData
		if (!StructProperty || StructProperty->Struct != TBaseStructure<FGameplayAttributeData>::Get())
		{
			continue;
		}

		const FGameplayAttributeData* AttrData =
			StructProperty->ContainerPtrToValuePtr<FGameplayAttributeData>(Attribute);

		if (!AttrData)
		{
			continue;
		}

		const float CurrentValue = AttrData->GetCurrentValue();
		const float BaseValue = AttrData->GetBaseValue();

		GEngine->AddOnScreenDebugMessage(
			-1,
			0.0f,
			FColor::White,
			FString::Printf(TEXT("[%02d] %s  Current: %.3f  Base: %.3f"),
				LineIndex++,
				*Property->GetName(),
				CurrentValue,
				BaseValue));
	}

	if (LineIndex == 0)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.0f,
			FColor::Silver,
			TEXT("(No FGameplayAttributeData found in this AttributeSet)"));
	}
}

