#pragma once

#include "CoreMinimal.h"
#include "AttackDetection.h"
#include "GameplayTagContainer.h"
#include "MotionWarpCalcMethod.h"
#include "Input/PlayerInputHandlerComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "CombatStep.generated.h"

struct FAttackDetectionConfig;
class ULevelSequence;
class AEnemyCharacterBase;
enum class EMotionWarpCalculationRules : uint8;
enum class EAttackStrength : uint8;
class UGameplayEffect;
enum class EInputAction : uint8;

UENUM(BlueprintType)
enum class ECombatActionPriority : uint8
{
	None = 0					UMETA(DisplayName = "None"),

	Switch = 5					UMETA(DisplayName = "Switch"),
	
	BasicAttack = 10			UMETA(DisplayName = "Attack"),
	
	Dodge = 20					UMETA(DisplayName = "Dodge"),

	CounterAttack = 50			UMETA(DisplayName = "CounterAttack"),
	
	HitReaction = 100			UMETA(DisplayName = "HitReaction"),

	Knockback = 150				UMETA(DisplayName = "Knockback"),

	HitFly = 200				UMETA(DisplayName = "HitFly"),

	Ultimate = 225				UMETA(DisplayName = "Ultimate"),	
	
	Dead = 255					UMETA(DisplayName = "Dead"),
};

UENUM(BlueprintType)
enum class EAttackStrength : uint8
{
	None,
	Light_Knockback,
	Significant_Knockback,
	Launch
};

UENUM(BlueprintType)
enum class ECameraLookAtMode : uint8
{
	LookAtAgent,
	LookAtEnemy,
	LookAtMiddle
};

USTRUCT(BlueprintType)
struct FCinematicCameraConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bEnableCinematicCamera{false};

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableCinematicCamera"))
	FVector LocalOffset{FVector::ZeroVector};

	/*UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableCinematicCamera"))
	ERelativeTarget RelativeTarget{ERelativeTarget::Agent};*/
	
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableCinematicCamera"))
	ECameraLookAtMode Mode{ECameraLookAtMode::LookAtAgent};
};

USTRUCT(BlueprintType)
struct FHitPayloadConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bEnableHitPayload{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	float DamageMultiplier{1.f};	// 伤害倍率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	float DazeMultiplier{1.5f};		// 失衡倍率

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	bool bIsHeavyAttack{false};		// 重击效果

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	EAttackStrength AttackStrength{EAttackStrength::None};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	TSubclassOf<UGameplayEffect> ImpactEffectOnTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableHitPayload"))
	TSubclassOf<UGameplayEffect> HitFeedbackEffectOnSelf;
};

UENUM(BlueprintType)
enum class EMotionWarpTrackingMode : uint8
{
	None,
	StaticWorldPoint,
	DynamicComponent
};

USTRUCT(BlueprintType)
struct FMotionWarpConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	FName WarpTargetName{FName("WarpTarget")};

	UPROPERTY(EditDefaultsOnly)
	EMotionWarpCalculationRules Rules{EMotionWarpCalculationRules::None};
	
	UPROPERTY(EditDefaultsOnly, meta = (BaseStruct = "/Script/ProjectZZZ.MotionWarpCalcMethod", ExcludeBaseStruct))
	FInstancedStruct CalculationMethod;

	UPROPERTY(EditDefaultsOnly)
	EMotionWarpTrackingMode TrackingMode{EMotionWarpTrackingMode::None};

};

USTRUCT(BlueprintType)
struct FParryActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsParryAction{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParryAction"))
	float ParrySocketOffset{0.f};		// parry socket to root relative offset along forward direction

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParryAction"))
	FName LoopSectionName{FName("WaitLoop")};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParryAction"))
	FName FollowSectionName{FName("Follow")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParryAction"))
	float HitStopDuration{0.05f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParryAction"))
	float HitStopTimeScale{0.1f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParryAction"))
	TSubclassOf<UGameplayEffect> ParryEffectOnEnemy{nullptr};
};

USTRUCT(BlueprintType)
struct FParriedActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsParriedAction{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsParriedAction"))
	float ParryOffset{0.f};
};

USTRUCT(BlueprintType)
struct FUltimateActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsUltimateAction{false};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsUltimateAction"))
	TObjectPtr<ULevelSequence> CutInSequence{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsUltimateAction"))
	FName SequenceBindingTag{FName("RuntimeAgent")};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsUltimateAction"))
	FGameplayTag CameraRequestTag{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsUltimateAction"))
	FLinearColor BackgroundColor{FLinearColor::Green};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bIsUltimateAction", ClampMin = "1", ClampMax = "255"))
	int32 AgentStencilValue{42};
};

USTRUCT(BlueprintType)
struct FCombatActionContext
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ACharacterBase> Enemy{nullptr};
};

UCLASS(BlueprintType)
class PROJECTZZZ_API UCombatActionStep : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual UAnimMontage* GetAnimMontage(const FVector2D& MovementInput) const { return Montage; };

public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Montage{nullptr};
	
	UPROPERTY(EditDefaultsOnly)
	EInputAction TriggerInput{EInputAction::EInputAction_Max};

	UPROPERTY(EditDefaultsOnly)
	ECombatActionPriority Priority{ECombatActionPriority::None};

	UPROPERTY(EditDefaultsOnly)
	bool bIsAttackAction{false};		// Basic Attack / Special Attack / Ultimate / Chain Attack / Quick Assist / Dodge Attack / Rush Attack

	UPROPERTY(EditDefaultsOnly)
	bool bIsHitReaction{false};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	uint8 bIsBasicAttack : 1 {false};
	
	UPROPERTY(EditDefaultsOnly, Category = "Features | Tag")
	FGameplayTag ActionTag{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly, Category = "Features | Tag")
	FGameplayTagContainer RequiredTags{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly)
	TMap<EInputAction, TObjectPtr<UCombatActionStep>> ComboLinks;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Features | Cost")
	TSubclassOf<UGameplayEffect> CostGameplayEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Features | Camera")
	FCinematicCameraConfig CameraConfig;
	
	UPROPERTY(EditDefaultsOnly, Category = "Features | MotionWarp")
	bool bEnableMotionWarp{false};
	
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bEnableMotionWarp"), Category = "Features | MotionWarp")
	float MotionWarpingEffectiveDistance{500.f};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (EditCondition = "bEnableMotionWarp", ShowOnlyInnerProperties), Category = "Features | MotionWarp")
	TArray<FMotionWarpConfig> WarpConfigs;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | HitPayload")
	FHitPayloadConfig HitPayloadConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | ParryConfig")
	FParryActionConfig ParryConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | Ultimate")
	FUltimateActionConfig UltimateConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Feature | AttackDetection")
	FAttackDetectionConfig AttackDetectionConfig;
	
	// for enemy
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | ParriedActionConfig")
	FParriedActionConfig ParriedActionConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | QuickAssist")
	bool bIsQuickAssist{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), Category = "Features | ChainAttack")
	bool bIsChainAttack{false};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties, EditCondition = "bIsQuickAssist || bIsChainAttack"), Category = "Features | SpecialAction")
	float AttackEntryForwardOffset{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ShowOnlyInnerProperties, EditCondition = "bIsQuickAssist || bIsChainAttack"), Category = "Features | SpecialAction")
	float AttackEntryLateralOffset{0.f};
};

UCLASS(BlueprintType)
class PROJECTZZZ_API UAgentCombatSteps : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<UCombatActionStep>> CombatSteps;
};

USTRUCT()
struct FBufferedIntent
{
	GENERATED_BODY()

public:
	bool IsValid() const { return ActionStep != nullptr; }

	void SetIntent(const UCombatActionStep* InCombatStep, const float InExpirationTime, const ECombatActionPriority InPriority)
	{
		ActionStep = InCombatStep;
		ExpirationTime = InExpirationTime;
		Priority = InPriority;
	}
	
	void Reset()
	{
		ActionStep = nullptr;
		ExpirationTime = 0.0f;
		Priority = ECombatActionPriority::None;
	}

public:
	UPROPERTY()
	const UCombatActionStep* ActionStep{nullptr};

	float ExpirationTime{0.0f};

	ECombatActionPriority Priority{ECombatActionPriority::None};
};

USTRUCT()
struct PROJECTZZZ_API FCombatExecutionState
{
	GENERATED_BODY()
	
public:
	void Reset()
	{
		CurrentStep = nullptr;
		MontageInstanceId = INDEX_NONE;

		bInputBufferWindowOpen = false;
		bProceedWindowOpen = false;
		bMovementInterruptWindowOpen = false;
		bIsRecoveryWindowOpen = false;
		bParryWindowOpen = false;
		bHasConfirmedNextAction = false;
		bHasSuccessfullyStarted = false;
	}
	
	UPROPERTY()
	TObjectPtr<const UCombatActionStep> CurrentStep{nullptr};

	bool bActionLogicFinished{false};

	int32 LogicFinishedActionRequestId{INDEX_NONE};
	
	int32 MontageInstanceId{INDEX_NONE};
	
	// Todo: use Bitmask
	uint8 bInputBufferWindowOpen : 1 {false};
	uint8 bProceedWindowOpen : 1 {false};
	uint8 bMovementInterruptWindowOpen : 1 {false};
	uint8 bIsRecoveryWindowOpen : 1 {false};
	uint8 bParryWindowOpen  : 1 {false};	// only parry?
	uint8 bHasSuccessfullyStarted : 1 {false};
	uint8 bHasConfirmedNextAction : 1 {false};
};
