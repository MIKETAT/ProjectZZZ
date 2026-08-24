#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitDetectionComponent.generated.h"

class ACharacterBase;

USTRUCT()
struct FDamageStateData
{
	GENERATED_BODY()

	double FrameTimeInMontage{-1.f};

	bool bTriggered{false};

	FVector MeshComponentRelativeLocation{FVector::ZeroVector};

	FQuat MeshComponentRelativeRotation{FQuat::Identity};
};

USTRUCT()
struct FDetectionCollisionShapeData
{
	GENERATED_BODY()

	FName BodyName{NAME_None};

	// Relative Location to Bone
	FVector RelativeLocation{FVector::ZeroVector};

	// Relative Rotation to Bone
	FQuat RelativeRotation{FQuat::Identity};

	FCollisionShape CollisionShape;

	// Last World Location		
	FVector LastLocation{FVector::ZeroVector};

	bool bValidLastLocation{false};
};

struct FHitDetectionFrameData
{
	void Reset() { *this = FHitDetectionFrameData(); }
	
	bool IsValid() const { return FMath::IsFinite(MontageTime) && MontageTime >= 0.f; }	

	double MontageTime{-1.f};

	FTransform BoneToMesh{FTransform::Identity};

	FTransform MeshToWorld{FTransform::Identity};
};

enum class EHitDetectionPoseSource : uint8 {
	RealTimeTick,
	ReconstructedSample
};

struct FHitDetectionSweepDebugRecord
{
	double StartTimeInMontage{-1.0};
	double EndTimeInMontage{-1.0};

	FVector StartLocation{FVector::ZeroVector};
	FVector EndLocation{FVector::ZeroVector};

	FQuat SweepRotation{FQuat::Identity};

	bool bInitialOverlap{false};

	EHitDetectionPoseSource EndPoseSource{EHitDetectionPoseSource::RealTimeTick};
};

struct FHitDetectionReferencePose
{
	double MontageTime{-1.f};
	
	FTransform ShapeWorldTransform{FTransform::Identity};

	float MontageWeight{-1.f};
};

struct FHitDetectionValidationStatistics
{
	void AddSample(const double CenterError, const double EndPointError)
	{
		++SampleCount;

		TotalCenterError += CenterError;
		TotalEndPointError += EndPointError;

		MaxCenterError = FMath::Max(MaxCenterError, CenterError);
		MaxEndPointError = FMath::Max(MaxEndPointError, EndPointError);
	}

	double GetAverageCenterError() const
	{
		return SampleCount > 0 ? TotalCenterError / SampleCount : 0.f;
	}

	double GetAverageEndPointError() const
	{
		return SampleCount > 0 ? TotalEndPointError / SampleCount : 0.f;
	}
	
	int32 SampleCount{0};

	double TotalCenterError{0.f};

	double MaxCenterError{0.f};

	double TotalEndPointError{0.f};

	double MaxEndPointError{0.f};
};

struct FHitDetectionValidationState
{
	void Reset() { *this = FHitDetectionValidationState(); }
	
	FHitDetectionFrameData PreviousSimulatedDetectionFrame;

	double DetectionAccumulator{0.f};

	double MaxReferenceInterval{0.f};

	FHitDetectionValidationStatistics ReconSampleStatistics;

	FHitDetectionValidationStatistics CompensatedSweepStatistics;

	FHitDetectionValidationStatistics UnCompensatedSweepStatistics;

	TArray<FHitDetectionSweepDebugRecord> CompensatedSweepDebugRecords;

	TArray<FHitDetectionSweepDebugRecord> UnCompensatedSweepDebugRecords;
};

// 重建采样点碰撞体需要的数据
struct FHitDetectionReconstructionContext
{
	FHitDetectionFrameData PreviousFrame;

	FHitDetectionFrameData CurrentFrame;

	bool bHasPoseCorrection{false};

	FTransform PreviousMontageBoneToMesh{FTransform::Identity};

	FTransform CurrentMontageBoneToMesh{FTransform::Identity};
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTZZZ_API UHitDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHitDetectionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EnableHitDetection(const FAnimNotifyEventReference& EventReference);

	void DisableHitDetection(const FAnimNotifyEventReference& EventReference);

	bool PerformHitDetection(const FHitDetectionFrameData& CurrentFrame);

	bool ProcessWeaponCollisionData(TArray<FHitResult>& TotalHitResults, FDetectionCollisionShapeData& ShapeData,
		const FTransform& ShapeWorldTransform, const FCollisionQueryParams& QueryParams);
	
	bool PrepareHitDetection(int32 MontageInstanceID);
	
private:
	void BuildCollisionData();

	void ResetHitDetectionStatus();

	FTransform CalculateCollisionShapeWorldTransform(const FDetectionCollisionShapeData& ShapeData, const FTransform& BoneToMesh, const FTransform& MeshToWorld) const;

	void DrawCollisionShapeDebug(const FDetectionCollisionShapeData& ShapeData, const FTransform& WorldTransform,
		const FColor& Color, uint8 DepthPriority, float Thickness) const;
	
	bool CacheDamageStateData(TArray<FDamageStateData>& OutDamageStateData, const FName& BoneOrSocketName, const FAnimNotifyEventReference& AnimNotifyEventReference) const;

	bool IsHitDetectionActive() const { return ActiveMontageInstanceID != INDEX_NONE; }

	void GetCapsuleAxisEndPoints(const FTransform& Transform, const FCollisionShape& Capsule, FVector& OutA, FVector& OutB) const;
	
	UFUNCTION(BlueprintCallable, Category = "HitDetection|Debug")
	void ToggleSweepTrajectoryVisibility();

	UFUNCTION(BlueprintCallable, Category = "HitDetection|Debug")
	void ToggleReferenceTrajectoryVisibility();

	UFUNCTION(BlueprintCallable, Category = "HitDetection|Debug")
	void ToggleCompensatedSweepTrajectoryVisibility();

	UFUNCTION(BlueprintCallable, Category = "HitDetection|Debug")
	void ToggleUnCompensatedSweepTrajectoryVisibility();
	
private:
	// Helper Function
	FTransform InterpTransform(const FTransform& From, const FTransform& To, const double Alpha) const;

	double GetCurrentMontageTime() const;

	FTransform ReconstructMeshToWorldAtSampleTime(const double SampleTime, const double PreviousMontageTime, const double CurrentMontageTime,
		const FTransform& PreviousMeshToWorld, const FTransform& CurrentMeshToWorld) const;
	
	void CaptureRealTimeReferenceCollisionShape(const FHitDetectionFrameData& FrameData);

	void CapturePreviousObservedFrame();

	void DrawCompleteTrajectoryView();

	ULineBatchComponent* GetTrajectoryLineBatcher() const;

	void DrawSweepTrajectory(ULineBatchComponent& LineBatcher, const TArray<FHitDetectionSweepDebugRecord>& Records, uint32 BatchID,
		const FLinearColor& RealTimeColor, const FLinearColor& ReconstructedColor) const;

	void DrawReferenceTrajectory(ULineBatchComponent& LineBatcher) const;

	bool ExtractMontageBoneComponentSpaceTransforms(double PreviousMontageTime, double CurrentMontageTime,
		FTransform& OutPreviousBoneComponentSpace, FTransform& OutCurrentBoneComponentSpace) const;

	FTransform ApplyInterpolatedBonePoseCorrection(	const FTransform& PreviousMontageBoneComponentSpace, const FTransform& CurrentMontageBoneComponentSpace,
													const FTransform& PreviousFinalBoneComponentSpace, const FTransform& CurrentFinalBoneComponentSpace,
													const FTransform& SampleMontageBoneComponentSpace, double Alpha) const;
	
	bool BuildCurrentDetectionFrame(FHitDetectionFrameData& OutFrame) const;

	bool ProcessRealTimeTickPose(const FHitDetectionFrameData& CurrentFrame, TArray<FHitResult>& OutHitResult, const FCollisionQueryParams& QueryParams);

	bool ProcessMissedSamples(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame, TArray<FHitResult>& OutHitResult, const FCollisionQueryParams& QueryParams);

	void RecordExecutedCollisionQuery(double MontageTime, const FTransform& ShapeWorldTransform,
		const FVector& PreviousShapeLocation, const bool bInitialOverlap, const EHitDetectionPoseSource EndPoseSource);

	void UpdateTrajectoryValidation(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame, const float DeltaTime);

	bool FindReferenceCollisionShapeAtMontageTime(const double MontageTime, FTransform& OutReferenceShapeWorldTransform, float* OutMontageWeight = nullptr) const;

	void BuildReconstructionContext(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame, FHitDetectionReconstructionContext& OutContext) const;

	bool BuildReconstructedCollisionShapeWorldTransform(const FDamageStateData& StateData, const FDetectionCollisionShapeData& ShapeData,
		const FHitDetectionReconstructionContext& Context, FTransform& OutShapeWorldTransform) const;

	bool ProcessValidationDetectionInterval(const FHitDetectionFrameData& PreviousFrame, const FHitDetectionFrameData& CurrentFrame);

	void CalculateCollisionShapeError(const FTransform& ReconShapeWorldTransform, const FTransform& ReferenceShapeWorldTransform,
		const FCollisionShape& CollisionShape, double& OutCenterError, double& OutEndPointError) const;

	void ValidationSweepTrajectory(const FHitDetectionSweepDebugRecord& SweepRecord, const FCollisionShape& CollisionShape, FHitDetectionValidationStatistics& OutStatistics) const;

	void FinalizeTrajectoryValidation();

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<ECollisionChannel> TraceChannel{ECC_MAX};
	
	static constexpr double Tolerance = 1.e-6;
	
	TArray<FDamageStateData> WeaponSocketDamageStateData;
	
	TArray<FDetectionCollisionShapeData> CollisionShapes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FName WeaponBoneName{NAME_None};

	UPROPERTY()
	TObjectPtr<ACharacterBase> Character;
	
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> OwnerMesh;
	
	int32 ActiveMontageSlotIndex{INDEX_NONE};

	int32 ActiveMontageInstanceID{INDEX_NONE};

	int32 PreviousObservedMontageInstanceID{INDEX_NONE};

	FHitDetectionFrameData PreviousDetectionFrame;

	FHitDetectionFrameData  PreviousObservedFrame;		// Reset when montage end/interrupt

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ValidationDetectionFrameRate{15};
	
	bool bPendingDisable{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 SampleCountPerSecond = 60;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bEnableTrajectoryVisualization{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 DrawCount = 8;
	
	TArray<FHitDetectionSweepDebugRecord> SweepDebugRecords;
	TArray<FHitDetectionReferencePose> ReferenceShapeRecord;

	static constexpr uint32 SweepTrajectoryBatchID = 0x48445301;
	static constexpr uint32 ReferenceTrajectoryBatchID = 0x48445201;
	static constexpr uint32 CompensatedSweepTrajectoryBatchID = 0x48445302;
	static constexpr uint32 UnCompensatedSweepTrajectoryBatchID = 0x48445303;

	bool bShowSweepTrajectory{false};
	bool bShowReferenceTrajectory{false};
	bool bShowCompensatedSweepTrajectory{false};
	bool bShowUnCompensatedSweepTrajectory{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool bEnableTrajectoryValidation{false};

	FHitDetectionValidationState ValidationState;
};
