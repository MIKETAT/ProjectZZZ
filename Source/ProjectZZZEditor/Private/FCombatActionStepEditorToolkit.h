#pragma once
#include "Character/Combat/CombatActionValidation.h"
#include "CombatActionStepEditor/CombatActionEditorTypes.h"
#include "CombatActionStepEditor/CombatActionTimelineItem.h"

class SCombatActionSegmentInspector;
enum class ECombatTimelineHitPart : uint8;
struct FCombatTimelineHitResult;
class SCombatActionTimeline;
class SCombatActionTimelineTrack;
class SCombatActionStepEditorViewport;
class FCombatActionStepEditorViewportClient;
class UCombatActionStep;

class FCombatActionStepEditorToolkit : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	virtual ~FCombatActionStepEditorToolkit() override;
	
	void InitCombatActionStepEditor(UCombatActionStep* Action);
		
	virtual FName GetToolkitFName() const override;

	virtual FText GetBaseToolkitName() const override;

	virtual FString GetWorldCentricTabPrefix() const override;

	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	void HandleScrubValueChanged(const float NormalizedValue);

	// Undo/Redo
	virtual void PostUndo(bool bSuccess) override;

	virtual void PostRedo(bool bSuccess) override;

	// Attribute Getter
	float GetNormalizedValuePosition() const;

	FText GetPreviewTimeText() const;

	float GetPreviewMontageLength() const;

	float GetPreviewCurrentTime() const;

	void HandleTimelineItemSelected(const FCombatTimelineHitResult& HitResult);

	void HandleTimelineItemDragged(const FCombatTimelineHitResult& HitResult, const float DragDeltaTime);

	void HandleTimelineItemDragFinished();

private:
	void CreateDetailsView();

	void CreatePreviewMeshComponent();

	void RefreshPreview();

	void RefreshInspector();
	
	void RefreshTimelineTrack();

	UAnimSingleNodeInstance* GetPreviewAnimInstance() const;
	
	TSharedRef<SDockTab> SpawnOverviewTab(const FSpawnTabArgs& SpawnTabArgs) const;

	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& SpawnTabArgs);

	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& SpawnTabArgs);

	FReply HandlePlayPauseClicked();

	FReply HandlePauseClicked();

	FReply HandleResetClicked();

	TOptional<FGuid> GetSelectedNotifyGuid() const;

	void SetPreviewPosition(const float NewPosition);

	void HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	
	void RefreshViewportHelper();

	void RebuildTimelineItems();

	FAnimNotifyEvent* FindNotifyByGuid(const FGuid& NotifyGuid) const;

	void HandleUndoRedo();

	void RefreshSelectedNotifyGuid();

	FCombatActionSegmentSelectionContext BuildCombatActionSegmentSelectionContext() const;
	
	void RefreshSelectedSegmentContext();

	void RefreshAttackDetectionVisualization();

	void RefreshValidation();

	void ApplyValidationToTimelineItems();

	
protected:
	virtual void GetSaveableObjects(TArray<UObject*>& OutObjects) const override;
	
private:
	inline static const FName OverviewTabId = TEXT("CombatActionStepEditor.OverviewTab");

	inline static const FName DetailsTabId = TEXT("CombatActionStepEditor.Details");

	inline static const FName AppIdentifier = TEXT("CombatActionStepEditorApp");

	inline static const FName ViewportTabId = TEXT("CombatActionStepEditor.Viewport");
	
	TWeakObjectPtr<UCombatActionStep> EditingAction{nullptr};

	// Preview声明周期应该比ViewportWidget、ViewportClient更长
	TSharedPtr<FPreviewScene> PreviewScene{nullptr};

// Widget
	TSharedPtr<SCombatActionStepEditorViewport> ViewportWidget{nullptr};
	
	TSharedPtr<IDetailsView> DetailsView;

	TSharedPtr<SCombatActionTimeline> TimelineWidget{nullptr};
	
	TWeakObjectPtr<USkeletalMeshComponent> PreviewMeshComponent{nullptr};

	TWeakObjectPtr<USkeletalMeshComponent> SamplingMeshComponent{nullptr};

	TWeakObjectPtr<USkeletalMesh> CurrentPreviewMesh{nullptr};

	TWeakObjectPtr<UAnimMontage> CurrentPreviewMontage{nullptr};

	TArray<FCombatActionTimelineItem> TimelineItems;

	TOptional<FGuid> SelectedNotifyGuid;

	float OriginStartTime{0.f};

	float OriginEndTime{0.f};

	TUniquePtr<FScopedTransaction> ActiveDragTransaction;

	TArray<FCombatActionValidationIssue> ValidationIssues;

	// Segment Details
	FCombatActionSegmentSelectionContext SelectedSegmentContext{};

	TSharedPtr<SCombatActionSegmentInspector> SegmentInspectorWidget{nullptr};
};

