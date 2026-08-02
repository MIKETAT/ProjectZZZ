#pragma once
#include "SEditorViewport.h"

struct FAttackDetectionVisualizationRequest;
class FCombatActionStepEditorViewportClient;

class SCombatActionStepEditorViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SCombatActionStepEditorViewport) {}

		SLATE_ARGUMENT(TSharedPtr<FPreviewScene>, PreviewScene)
		
	SLATE_END_ARGS()
		
	void Construct(const FArguments& InArgs);

	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

	void FocusOnBounds(const FBox& Bounds);

	void SetShapeQueryVisualization(const FAttackDetectionVisualizationRequest& Request);
	
private:
	TSharedPtr<FPreviewScene> PreviewScene{nullptr};

	TSharedPtr<FCombatActionStepEditorViewportClient> EditorViewportClient;
};
