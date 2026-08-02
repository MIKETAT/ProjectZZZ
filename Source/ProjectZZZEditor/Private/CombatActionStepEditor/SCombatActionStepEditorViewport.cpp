#include "SCombatActionStepEditorViewport.h"

#include "FCombatActionStepEditorViewportClient.h"

void SCombatActionStepEditorViewport::Construct(const FArguments& InArgs)
{
	PreviewScene = InArgs._PreviewScene;

	check(PreviewScene.IsValid());
	
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SCombatActionStepEditorViewport::MakeEditorViewportClient()
{
	check(PreviewScene.IsValid());
	
	const TWeakPtr<SEditorViewport> WeakViewport = SharedThis(this);
	EditorViewportClient = MakeShared<FCombatActionStepEditorViewportClient>(PreviewScene.Get(), WeakViewport);
	return EditorViewportClient.ToSharedRef();
}

void SCombatActionStepEditorViewport::FocusOnBounds(const FBox& Bounds)
{
	if (!EditorViewportClient.IsValid() || !Bounds.IsValid)
	{
		return;
	}

	EditorViewportClient->FocusViewportOnBox(Bounds, true);
	Invalidate();
}

void SCombatActionStepEditorViewport::SetShapeQueryVisualization(const FAttackDetectionVisualizationRequest& Request)
{
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->SetShapeQueryVisualizationRequest(Request);
		Invalidate();	
	}
}
