#pragma once

#include "CombatActionEditorTypes.h"
#include "Character/Combat/AttackDetectionGeometry.h"

struct FAttackDetectionVisualizationFrame
{
	void Reset()
	{
		Shapes.Reset();
		Sweeps.Reset();
	}

	TArray<FAttackShapeQueryGeometry> Shapes;

	TArray<FAttackSweepGeometry> Sweeps;
};

class FCombatActionStepEditorViewportClient : public FEditorViewportClient
{
public:
	FCombatActionStepEditorViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget = nullptr);

	//virtual ~FCombatActionStepEditorViewportClient() override;

	virtual void Tick(float InDeltaTime) override;

	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	
	void SetShapeQueryVisualizationRequest(const FAttackDetectionVisualizationRequest& InVisualizationRequest);

	void DrawVisualizationCache(FPrimitiveDrawInterface* PDI);

	bool IsVisualizationActiveAtTime(const float CurrentTime) const;
	
	void DrawShapeGeometry(FPrimitiveDrawInterface* PDI, const FAttackShapeQueryGeometry& Geometry);

	void DrawSweepGeometry(FPrimitiveDrawInterface* PDI, const FAttackSweepGeometry& Geometry);
	
private:
	float GetPreviewCurrentTime() const;

	void TickPreviewScene(float InDeltaTime);
	
	void UpdateAttackDetectionVisualizationCache();

	void UpdateShapeQueryCache();

	void UpdateWeaponSweepCache();

	void UpdateActorPathSweepCache();

	void DrawCollisionShapePDI(FPrimitiveDrawInterface* PDI, const FVector& Location, const FQuat& Rotation, const FCollisionShape& Shape);

	void DrawSweepBoxPDI(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FQuat& Rotation, const FVector& HalfExtent);

	void DrawSweepSpherePDI(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const float Radius);

	void DrawSweepCapsulePDI(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FQuat& Rotation,
		const float Radius, const float HalfHeight);

	bool BuildPreviewActorTransformAtTime(
		const float AnchorTime, const FTransform& AnchorWorldTransform,
		const float SampleTime, FTransform& OutSampleWorldTransform) const;
	
private:
	FAttackDetectionVisualizationRequest VisualizationRequest;

	FAttackDetectionVisualizationFrame VisualizationFrame;
	
	const float PreviewInstantHalfWindow = 1.f / 30.f;
	
	const float PreviewSampleInterval = 1.f / 60.f;

	const FColor Color = FColor::Green;
	
	const FLinearColor TraceColor = FLinearColor::Green;

	const FLinearColor TraceHitColor = FLinearColor::Red;
	
	const uint8 DepthPriority = SDPG_World;
	
	const float Thickness = 1.f;
};
