#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "Character/CharacterFrameDataBus.h"
#include "Components/ActorComponent.h"
#include "PlayerInputHandlerComponent.generated.h"

struct FCharacterFrameDataBus;
class ACharacterBase;
struct FInputActionInstance;

UENUM(BlueprintType, meta = (Bitflags))
enum class EInputAction : uint8
{
	EInputActionFlag_Movement					= 0		UMETA(DisplayName = "Movement"),
	EInputActionFlag_Look						= 1		UMETA(DisplayName = "Look"),
	EInputActionFlag_Dodge						= 2		UMETA(DisplayName = "Dodge"),
	EInputActionFlag_Basic_Attack				= 3		UMETA(DisplayName = "Basic_Attack"),
	EInputActionFlag_Special_Attack				= 4		UMETA(DisplayName = "Special_Attack"),
	EInputActionFlag_Ultimate					= 5		UMETA(DisplayName = "Ultimate"),
	EInputActionFlag_SwitchCharacter_Previous	= 6		UMETA(DisplayName = "SwitchCharacter_Previous"),
	EInputActionFlag_SwitchCharacter_Next		= 7		UMETA(DisplayName = "SwitchCharacter_Next"),
	EInputActionFlag_Chain_Attack_Left			= 8		UMETA(DisplayName = "Chain_Attack_Left"),
	EInputActionFlag_Chain_Attack_Right			= 9		UMETA(DisplayName = "Chain_Attack_Right"),
	EInputActionFlag_Chain_Attack_Cancel		= 10	UMETA(DisplayName = "Chain_Attack_Cancel"),

	// todo: UI Input
	EInputAction_Max									UMETA(Hidden)
};

static_assert(static_cast<uint8>(EInputAction::EInputAction_Max) <= 32, "Bitset Exceeded");

USTRUCT(BlueprintType)
struct FInputBitmask
{
	GENERATED_BODY()

	FInputBitmask() : MaskData(0) {}
public:
	FORCEINLINE void Set(EInputAction Action, bool bActive)
	{
		checkf(static_cast<uint8>(Action) < 32, TEXT("Input Action Out of Boundary"));

		if (bActive)
		{
			MaskData |= (1U << static_cast<uint8>(Action));
		} else
		{
			MaskData &= ~(1U << static_cast<uint8>(Action));
		}
	}

	FORCEINLINE bool Test(EInputAction Action) const
	{
		checkf(static_cast<uint8>(Action) < 32, TEXT("Input Action Out of Boundary"));

		return (MaskData & (1U << static_cast<uint8>(Action)));
	}

	FORCEINLINE void Reset() { MaskData = 0; }

	FORCEINLINE bool Any() const { return MaskData != 0; }

	// Todo: check if needed
	FORCEINLINE bool TestAll(const FInputBitmask& Other) const
	{
		return (MaskData & Other.MaskData) == Other.MaskData; 
	}

	FORCEINLINE bool TestAny(const FInputBitmask& Other) const
	{
		return (MaskData & Other.MaskData) != 0;
	} 

public:
	template<typename Func>
	void ForEachSetAction(Func&& Fn) const
	{
		uint32 Bits = MaskData;
		
		while (Bits)
		{
			uint32 Index = FMath::CountTrailingZeros(Bits);
			Fn(static_cast<EInputAction>(Index));

			Bits &= (Bits - 1);
		}
	}
	
public:
	uint32 MaskData;
};

USTRUCT()
struct FPlayerInputs
{
	GENERATED_BODY()

public:
	void ConsumeInputAction(EInputAction Action)
	{
		InputActionBitmask.Set(Action, false);	
	}
	
	FInputBitmask InputActionBitmask;
	FVector2D RawMovementInput{FVector2D::ZeroVector};
	FVector2D RawLookInput{FVector2D::ZeroVector};
};

USTRUCT(BlueprintType)
struct PROJECTZZZ_API FCharacterFrameDataBus
{
	GENERATED_BODY()
	
public:
	bool HasMovementInput() const { return !PlayerInputs.RawMovementInput.IsNearlyZero(); }
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	uint8 bIsLocalPlayer : 1 {false};
	
	FPlayerInputs PlayerInputs;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UPlayerInputHandlerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInputHandlerComponent(const FObjectInitializer& ObjectInitializer);

	// ActorComponent Interface
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void InitializeComponent() override;
protected:
	virtual void BeginPlay() override;
	// ~ActorComponent Interface

public:
	void BuildCharacterFrameDataBus();
	
	void RegisterInput();
	
	FCharacterFrameDataBus& GetCharacterFrameDataBus() { return DataBus; };

	bool HasMovementInput() const { return DataBus.HasMovementInput(); };
private:
	// On_Input_XXX Function
	void On_Input_Movement(const FInputActionInstance& Instance);
	void On_Input_Look(const FInputActionInstance& Instance);
	void On_Input_Dodge(const FInputActionInstance& Instance);
	void On_Input_Basic_Attack(const FInputActionInstance& Instance);
	void On_Input_Special_Attack(const FInputActionInstance& Instance);
	void On_Input_Ultimate(const FInputActionInstance& Instance);
	void On_Input_SwitchCharacter_Previous(const FInputActionInstance& Instance);
	void On_Input_SwitchCharacter_Next(const FInputActionInstance& Instance);
	void On_Input_ChainAttack_Left(const FInputActionInstance& Instance);
	void On_Input_ChainAttack_Right(const FInputActionInstance& Instance);
	void On_Input_ChainAttack_Cancel(const FInputActionInstance& Instance);

public:
	// Input Action Asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input | IMC")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input | IMC", meta=(ClampMin = "0", UIMin = "0"))
	uint8 DefaultInputMappingContextPriority{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Movement_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Look_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Dodge_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Basic_Attack_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Special_Attack_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* Ultimate_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* SwitchCharacter_Previous_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* SwitchCharacter_Next_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* ChainAttack_Left_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* ChainAttack_Right_Action{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* ChainAttack_Cancel_Action{nullptr};
private:
	UPROPERTY()
	FCharacterFrameDataBus DataBus;
	
	FInputBitmask InputActionBitmask;
	FVector2D RawInputMovementVector{FVector::ZeroVector};
	FVector2D RawInputLookVector{FVector::ZeroVector};

	UPROPERTY()
	UEnhancedInputComponent* EnhancedInputComponent{nullptr};
};
 