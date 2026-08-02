#include "FCombatActionStepEditorToolkit.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimNotify/AnimNotify_TriggerAttackDetection.h"
#include "Animation/AnimNotifyState/AnimNotifyState_AttackDetection.h"
#include "Character/Combat/CombatActionValidation.h"
#include "Character/Combat/CombatStep.h"
#include "CombatActionStepEditor/CombatActionTimelineItem.h"
#include "CombatActionStepEditor/FCombatActionEditorValidation.h"
#include "CombatActionStepEditor/SCombatActionSegmentInspector.h"
#include "CombatActionStepEditor/SCombatActionStepEditorViewport.h"
#include "CombatActionStepEditor/SCombatActionTimeline.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatActionStepToolkit, Log, All);

#define LOCTEXT_NAMESPACE "FCombatActionStepEditorToolkit"

FCombatActionStepEditorToolkit::~FCombatActionStepEditorToolkit()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

void FCombatActionStepEditorToolkit::InitCombatActionStepEditor(UCombatActionStep* Action)
{
	check(Action != nullptr);

	UE_LOG(LogCombatActionStepToolkit, Display, TEXT("Initialize Toolkit = %p, Asset = %s"), this, *GetNameSafe(Action))

	// Register Undo/Redo
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
	
	// 保存本次操作的CombatAction
	EditingAction = Action;

	// PreviewScene
	PreviewScene = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues());

	// PreviewMeshComponent
	CreatePreviewMeshComponent();
	
	// Viewport
	ViewportWidget = SNew(SCombatActionStepEditorViewport).PreviewScene(PreviewScene);

	// Refresh
	RefreshPreview();
	
	// DetailsView
	CreateDetailsView();
	

	RebuildTimelineItems();

	// Layout
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(TEXT("CombatActionStepEditorLayout_v2"))->AddArea(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split(
			FTabManager::NewStack()
				->SetSizeCoefficient(0.65f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
			)
			->Split(FTabManager::NewStack()
				->SetSizeCoefficient(0.35f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
	);
	
	InitAssetEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), AppIdentifier,
		Layout, true, true, Action);
}

FName FCombatActionStepEditorToolkit::GetToolkitFName() const
{
	return FName(TEXT("CombatActionStepEditor"));
}

FText FCombatActionStepEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("BaseToolkitName", "Combat Action Step Editor");
}

FString FCombatActionStepEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("Combat Action Step");
}

FLinearColor FCombatActionStepEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.5f, 0.2f, 0.5f);
}

void FCombatActionStepEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(OverviewTabId, FOnSpawnTab::CreateSP(this, &FCombatActionStepEditorToolkit::SpawnOverviewTab));
	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FCombatActionStepEditorToolkit::SpawnDetailsTab));
	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FCombatActionStepEditorToolkit::SpawnViewportTab));
}

void FCombatActionStepEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(OverviewTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(ViewportTabId);
	
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

void FCombatActionStepEditorToolkit::CreateDetailsView()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(EditingAction.Get());
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FCombatActionStepEditorToolkit::HandleFinishedChangingProperties);
}

void FCombatActionStepEditorToolkit::CreatePreviewMeshComponent()
{
	USkeletalMeshComponent* NewPreviewMeshComponent = NewObject<USkeletalMeshComponent>(
			GetTransientPackage(), NAME_None, RF_Transient);
	PreviewMeshComponent = NewPreviewMeshComponent;
	PreviewScene->AddComponent(NewPreviewMeshComponent, FTransform::Identity, false);

	USkeletalMeshComponent* NewSamplingComponent = NewObject<USkeletalMeshComponent>(
		GetTransientPackage(), NAME_None, RF_Transient);
	SamplingMeshComponent = NewSamplingComponent;
	PreviewScene->AddComponent(NewSamplingComponent, FTransform::Identity, false);

	SamplingMeshComponent->SetVisibility(false);
	SamplingMeshComponent->SetHiddenInGame(true);
}

void FCombatActionStepEditorToolkit::RefreshPreview()
{
	UCombatActionStep* Action = EditingAction.Get();
	USkeletalMeshComponent* MeshComponent = PreviewMeshComponent.Get();
	USkeletalMeshComponent* SamplingComponent = SamplingMeshComponent.Get();
	if (!Action || !MeshComponent || !SamplingComponent)
	{
		return;
	}

	USkeletalMesh* NewPreviewMesh{nullptr};
	
#if WITH_EDITORONLY_DATA
	NewPreviewMesh = Action->PreviewSkeletalMesh;
#endif
	
	UAnimMontage* NewPreviewMontage = Action->Montage;
	
	const bool bPreviewMeshChanged = CurrentPreviewMesh != NewPreviewMesh;
	const bool bPreviewMontageChanged = CurrentPreviewMontage != NewPreviewMontage;

	if (!bPreviewMeshChanged && !bPreviewMontageChanged)
	{
		return;
	}

	if (bPreviewMeshChanged)
	{
		MeshComponent->Stop();
		MeshComponent->SetAnimation(nullptr);
		MeshComponent->SetSkeletalMesh(NewPreviewMesh);

		SamplingComponent->Stop();
		SamplingComponent->SetAnimation(nullptr);
		SamplingComponent->SetSkeletalMesh(NewPreviewMesh);
	}
	
	if (bPreviewMeshChanged || bPreviewMontageChanged)
	{
		MeshComponent->Stop();
		MeshComponent->SetAnimation(nullptr);

		SamplingComponent->Stop();
		SamplingComponent->SetAnimation(nullptr);

		if (NewPreviewMesh && NewPreviewMontage)
		{
			MeshComponent->PlayAnimation(NewPreviewMontage, true);
			SamplingComponent->PlayAnimation(NewPreviewMontage, false);

			if (UAnimSingleNodeInstance* Instance = SamplingComponent->GetSingleNodeInstance())
			{
				Instance->SetPlaying(false);
			}
		}
	}
	
	CurrentPreviewMesh = NewPreviewMesh;
	CurrentPreviewMontage = NewPreviewMontage;
	
	MeshComponent->UpdateBounds();
	SamplingComponent->UpdateBounds();

	if (bPreviewMeshChanged && CurrentPreviewMesh.IsValid() && ViewportWidget.IsValid())
	{
		ViewportWidget->FocusOnBounds(MeshComponent->Bounds.GetBox());
	}

	RefreshViewportHelper();
	RefreshAttackDetectionVisualization();
}

void FCombatActionStepEditorToolkit::RefreshInspector()
{
	if (SegmentInspectorWidget.IsValid())
	{
		SegmentInspectorWidget->Refresh();
	}
}

void FCombatActionStepEditorToolkit::RefreshTimelineTrack()
{
	if (TimelineWidget.IsValid())
	{
		TimelineWidget->RefreshTrack();
	}
}

UAnimSingleNodeInstance* FCombatActionStepEditorToolkit::GetPreviewAnimInstance() const
{
	if (PreviewMeshComponent.IsValid())
	{
		return PreviewMeshComponent->GetSingleNodeInstance();
	}
	return nullptr;
}

TSharedRef<SDockTab> FCombatActionStepEditorToolkit::SpawnOverviewTab(const FSpawnTabArgs& SpawnTabArgs) const
{
	check(SpawnTabArgs.GetTabId().TabType == OverviewTabId);

	FText Text = EditingAction.IsValid()
	? FText::Format(LOCTEXT("EditingCombatStepActionText", "Editing Combat Step Action : {0}"), FText::FromString(EditingAction->GetName()))
	: LOCTEXT("EditingCombatStepActionText", "Invalid Action Step");
	
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(LOCTEXT("OverviewTab", "Overview"))
		[
			SNew(STextBlock)
			.Text(Text)
		];
	
	return Tab;
}

TSharedRef<SDockTab> FCombatActionStepEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& SpawnTabArgs)
{
	check(SpawnTabArgs.GetTabId().TabType == DetailsTabId);

	check(DetailsView.IsValid());

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab", "Details"))
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SBox)
				[
					SAssignNew(SegmentInspectorWidget, SCombatActionSegmentInspector)
					.SelectionContext(&SelectedSegmentContext)
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSeparator)
			]
			
			+SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				DetailsView.ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> FCombatActionStepEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& SpawnTabArgs)
{
	check(SpawnTabArgs.GetTabId().TabType == ViewportTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTabLabel1", "Viewport"))
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				ViewportWidget.ToSharedRef()
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(TimelineWidget, SCombatActionTimeline)
				.HeaderWidth(200.f)
				.OnPlayPauseClick(FOnClicked::CreateSP(this, &FCombatActionStepEditorToolkit::HandlePlayPauseClicked))
				.OnResetClick(FOnClicked::CreateSP(this, &FCombatActionStepEditorToolkit::HandleResetClicked))
				.NormalizedPosition(TAttribute<float>::Create(
					TAttribute<float>::FGetter::CreateSP(this, &FCombatActionStepEditorToolkit::GetNormalizedValuePosition)))
				.OnScrubValueChanged(FOnFloatValueChanged::CreateSP(this, &FCombatActionStepEditorToolkit::HandleScrubValueChanged))
				.PreviewTimeText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FCombatActionStepEditorToolkit::GetPreviewTimeText)))
				.TimelineItems(&TimelineItems)
				.MontageLength(TAttribute<float>::CreateSP(this, &FCombatActionStepEditorToolkit::GetPreviewMontageLength))
				.CurrentTime(TAttribute<float>::CreateSP(this, &FCombatActionStepEditorToolkit::GetPreviewCurrentTime))
				.SelectedGuid(TAttribute<TOptional<FGuid>>::CreateSP(this, &FCombatActionStepEditorToolkit::GetSelectedNotifyGuid))
				.OnItemSelected(FOnCombatTimelineItemSelected::CreateSP(this, &FCombatActionStepEditorToolkit::HandleTimelineItemSelected))
				.OnItemDragged(FOnCombatTimelineItemDragged::CreateSP(this, &FCombatActionStepEditorToolkit::HandleTimelineItemDragged))
				.OnItemDragFinished(FOnCombatTimelineItemDragFinished::CreateSP(this, &FCombatActionStepEditorToolkit::HandleTimelineItemDragFinished))
			]
		];
}

FReply FCombatActionStepEditorToolkit::HandlePlayPauseClicked()
{
	if (UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance())
	{
		Instance->SetPlaying(!Instance->IsPlaying());
		RefreshViewportHelper();
	}

	return FReply::Handled();
}

FReply FCombatActionStepEditorToolkit::HandlePauseClicked()
{
	if (UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance())
	{
		Instance->SetPlaying(false);
	}
	RefreshViewportHelper();
	
	return FReply::Handled();
}

FReply FCombatActionStepEditorToolkit::HandleResetClicked()
{
	if (UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance())
	{
		Instance->SetPlaying(false);
		Instance->SetPosition(0.f, false);
	}

	return FReply::Handled();
}

void FCombatActionStepEditorToolkit::HandleScrubValueChanged(const float NormalizedValue)
{
	UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance();
	if (!Instance)
	{
		return;
	}

	const float Length{Instance->GetLength()};
	if (Length <= UE_SMALL_NUMBER)
	{
		return;
	}

	SetPreviewPosition(FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * Length);
}

void FCombatActionStepEditorToolkit::PostUndo(bool bSuccess)
{
	if (!bSuccess)
	{
		return;
	}

	HandleUndoRedo();
	
	FEditorUndoClient::PostUndo(bSuccess);
}

void FCombatActionStepEditorToolkit::PostRedo(bool bSuccess)
{
	if (!bSuccess)
	{
		return;
	}

	HandleUndoRedo();
	
	FEditorUndoClient::PostRedo(bSuccess);
}

float FCombatActionStepEditorToolkit::GetNormalizedValuePosition() const
{
	UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance();
	if (!Instance)
	{
		return 0.f;
	}

	const float Length{Instance->GetLength()};
	if (Length <= UE_SMALL_NUMBER)
	{
		return 0.f;
	}
	
	return FMath::Clamp(Instance->GetCurrentTime() / Length, 0.0f, 1.0f);
}

FText FCombatActionStepEditorToolkit::GetPreviewTimeText() const
{
	UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance();

	const float CurrentTime{Instance ? Instance->GetCurrentTime() : 0.f};
	const float Length{Instance ? Instance->GetLength() : 0.f};

	FNumberFormattingOptions NumberFormat;
	NumberFormat.MinimumFractionalDigits = 2;
	NumberFormat.MaximumFractionalDigits = 2;

	return FText::Format(
		LOCTEXT("PreviewTimeFormat", "{0} / {1}"),
		FText::AsNumber(CurrentTime, &NumberFormat),
		FText::AsNumber(Length, &NumberFormat)
	);
}

float FCombatActionStepEditorToolkit::GetPreviewMontageLength() const
{
	UAnimMontage* Montage = EditingAction.IsValid() ? EditingAction->Montage : nullptr;
	return Montage ? Montage->GetPlayLength() : 0.f;
}

float FCombatActionStepEditorToolkit::GetPreviewCurrentTime() const
{
	UAnimSingleNodeInstance* AnimInstance = GetPreviewAnimInstance(); 
	return AnimInstance ? AnimInstance->GetCurrentTime() : 0.f;
}

void FCombatActionStepEditorToolkit::HandleTimelineItemSelected(const FCombatTimelineHitResult& HitResult)
{
	SelectedNotifyGuid = HitResult.NotifyGuid;

	RefreshTimelineTrack();
	RefreshSelectedSegmentContext();
	RefreshValidation();
}

void FCombatActionStepEditorToolkit::HandleTimelineItemDragged(const FCombatTimelineHitResult& HitResult, const float DragDeltaTime)
{
	if (!HitResult.IsValid() || !CurrentPreviewMontage.IsValid())
	{
		return;
	}

	FAnimNotifyEvent* NotifyEvent = CurrentPreviewMontage->Notifies.FindByPredicate([&HitResult] (const FAnimNotifyEvent& Event)
	{
		return Event.Guid == HitResult.NotifyGuid;
	});

	if (!NotifyEvent)
	{
		return;
	}
	
	// first time update
	if (!ActiveDragTransaction.IsValid())
	{
		OriginStartTime = NotifyEvent->GetTime(EAnimLinkMethod::Type::Absolute);
		OriginEndTime = OriginStartTime + NotifyEvent->GetDuration();
		SelectedNotifyGuid = HitResult.NotifyGuid;
			
		ActiveDragTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("TimelineItemMoved", "Timeline Item Moved"));
		CurrentPreviewMontage->Modify();
	}

	// update
	if (HitResult.HitPart == ECombatTimelineHitPart::InstantHandle)
	{
		const float NewTime = FMath::Clamp(OriginStartTime + DragDeltaTime, 0.f, GetPreviewMontageLength());
		NotifyEvent->SetTime(NewTime);
	} else if (HitResult.HitPart == ECombatTimelineHitPart::StateStartHandle)
	{
		const float NewTime = FMath::Clamp(OriginStartTime + DragDeltaTime, 0.f, OriginEndTime);
		const float NewDuration = FMath::Clamp(OriginEndTime - NewTime, 0.f, GetPreviewMontageLength());
		NotifyEvent->SetTime(NewTime);
		NotifyEvent->SetDuration(NewDuration);
	} else if (HitResult.HitPart == ECombatTimelineHitPart::StateEndHandle)
	{
		const float NewTime = FMath::Clamp(OriginEndTime + DragDeltaTime, OriginStartTime, GetPreviewMontageLength());
		const float NewDuration = FMath::Clamp(NewTime - OriginStartTime, 0, GetPreviewMontageLength());
		
		NotifyEvent->SetDuration(NewDuration);
	} else if (HitResult.HitPart == ECombatTimelineHitPart::StateBody)
	{
		const float Duration = OriginEndTime - OriginStartTime;
		const float NewStartTime = FMath::Clamp(OriginStartTime + DragDeltaTime, 0.f, GetPreviewMontageLength() - Duration);
		NotifyEvent->SetTime(NewStartTime);
	}

	// Refresh
	RebuildTimelineItems();
}

void FCombatActionStepEditorToolkit::HandleTimelineItemDragFinished()
{
	if (!ActiveDragTransaction.IsValid())
	{
		return;
	}
	
	CurrentPreviewMontage->SortNotifies();
	CurrentPreviewMontage->RefreshCacheData();
	CurrentPreviewMontage->MarkPackageDirty();

	RebuildTimelineItems();
	
	ActiveDragTransaction.Reset();
	OriginStartTime = 0.f;
	OriginEndTime = 0.f;

	TimelineWidget->RefreshTrack();
}

TOptional<FGuid> FCombatActionStepEditorToolkit::GetSelectedNotifyGuid() const
{
	return SelectedNotifyGuid.IsSet() && SelectedNotifyGuid.GetValue().IsValid()? SelectedNotifyGuid : TOptional<FGuid>();
}

void FCombatActionStepEditorToolkit::SetPreviewPosition(const float NewPosition)
{
	UAnimSingleNodeInstance* Instance = GetPreviewAnimInstance();
	USkeletalMeshComponent* MeshComponent = PreviewMeshComponent.Get();
	if (!Instance || !MeshComponent)
	{
		return;
	}

	const float Length{Instance->GetLength()};
	if (Length <= UE_SMALL_NUMBER)
	{
		return;
	}

	const float ClampedPosition = FMath::Clamp(NewPosition, 0.0f, Length);
	const float PreviousPosition = Instance->GetCurrentTime();

	Instance->SetPlaying(false);
	Instance->SetPositionWithPreviousTime(ClampedPosition, PreviousPosition, false);

	MeshComponent->TickAnimation(0.f, false);
	MeshComponent->RefreshBoneTransforms(nullptr);
	MeshComponent->MarkRenderDynamicDataDirty();

	RefreshViewportHelper();
}

void FCombatActionStepEditorToolkit::HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshPreview();
	RebuildTimelineItems();
	RefreshAttackDetectionVisualization();
}

void FCombatActionStepEditorToolkit::RefreshViewportHelper()
{
	if (ViewportWidget.IsValid())
	{
		ViewportWidget->Invalidate();
	}
}

void FCombatActionStepEditorToolkit::RebuildTimelineItems()
{
	TimelineItems.Reset();
	RefreshValidation();
	
	UCombatActionStep* Action = EditingAction.Get();
	if (!Action || !Action->Montage)
	{
		SelectedNotifyGuid.Reset();
		RefreshTimelineTrack();
		RefreshSelectedSegmentContext();
		return;
	}

	UAnimMontage* Montage = Action->Montage;
	
	for (int32 Index = 0; Index < Montage->Notifies.Num(); Index++)
	{
		const FAnimNotifyEvent& NotifyEvent = Montage->Notifies[Index];
		// NotifyState
		if (const UAnimNotifyState_AttackDetection* AttackDetectionState = Cast<UAnimNotifyState_AttackDetection>(NotifyEvent.NotifyStateClass))
		{
			FCombatActionTimelineItem& Item = TimelineItems.AddDefaulted_GetRef();
			
			Item.NotifyGuid = NotifyEvent.Guid;
			Item.NotifyIndex = Index;
			Item.ItemType = ECombatActionTimelineItemType::NotifyState;
			Item.StartTime = NotifyEvent.GetTime(EAnimLinkMethod::Type::Absolute);
			Item.EndTime = Item.StartTime + NotifyEvent.GetDuration();
			Item.SegmentName = AttackDetectionState->SegmentName;
		}
		// Notify
		else if (const UAnimNotify_TriggerAttackDetection* TriggerAttackDetection = Cast<UAnimNotify_TriggerAttackDetection>(NotifyEvent.Notify))
		{
			FCombatActionTimelineItem& Item = TimelineItems.AddDefaulted_GetRef();
			
			Item.NotifyGuid = NotifyEvent.Guid;
			Item.NotifyIndex = Index;
			Item.ItemType = ECombatActionTimelineItemType::InstantNotify;
			Item.StartTime = NotifyEvent.GetTime(EAnimLinkMethod::Type::Absolute);
			Item.EndTime = Item.StartTime;
			Item.SegmentName = TriggerAttackDetection->SegmentName;
		}
	}

	ApplyValidationToTimelineItems();

	RefreshSelectedNotifyGuid();
	RefreshTimelineTrack();
	RefreshSelectedSegmentContext();
}

FAnimNotifyEvent* FCombatActionStepEditorToolkit::FindNotifyByGuid(const FGuid& NotifyGuid) const
{
	if (!EditingAction.IsValid() || !EditingAction->Montage)
	{
		return nullptr;
	}

	return EditingAction->Montage->Notifies.FindByPredicate([&NotifyGuid](const FAnimNotifyEvent& NotifyEvent)
	{
		return NotifyEvent.Guid == NotifyGuid;
	});
}

void FCombatActionStepEditorToolkit::HandleUndoRedo()
{
	RebuildTimelineItems();
	
	if (TimelineWidget.IsValid())
	{
		TimelineWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void FCombatActionStepEditorToolkit::RefreshSelectedNotifyGuid()
{
	// check if SelectedNotifyGuid still exist in TimelineItem
	if (SelectedNotifyGuid.IsSet())
	{
		if (!SelectedNotifyGuid->IsValid())
		{
			SelectedNotifyGuid.Reset();
		} else
		{
			const FGuid SelectedGuid = SelectedNotifyGuid.GetValue();
			const bool bStillExist = TimelineItems.ContainsByPredicate([&SelectedGuid](const FCombatActionTimelineItem& Item)
			{
				return SelectedGuid == Item.NotifyGuid;
			});

			if (!bStillExist)
			{
				SelectedNotifyGuid.Reset();		// Reset if not exist	
			}
		}
	}
}

FCombatActionSegmentSelectionContext FCombatActionStepEditorToolkit::BuildCombatActionSegmentSelectionContext() const
{
	FCombatActionSegmentSelectionContext Context;

	for (const auto& Issue : ValidationIssues)
	{
		Context.AllIssues.Add(Issue);	
	}
	
	const UCombatActionStep* Action = EditingAction.Get();
	if (!Action)
	{
		return Context;
	}
	
	if (!SelectedNotifyGuid.IsSet() || !SelectedNotifyGuid->IsValid())
	{
		Context.SelectionStatus = ECombatActionSegmentSelectionStatus::NoSelection;
		return Context;
	}
	
	const FAnimNotifyEvent* NotifyEvent = FindNotifyByGuid(SelectedNotifyGuid.GetValue());
	if (!NotifyEvent)
	{
		Context.SelectionStatus = ECombatActionSegmentSelectionStatus::NotifyNotFound;
		return Context;
	}

	Context.StartTime = NotifyEvent->GetTime(EAnimLinkMethod::Type::Absolute);
	Context.EndTime = Context.StartTime + NotifyEvent->GetDuration();
	Context.ItemType = ECombatActionTimelineItemType::None;

	// Guid and ValidationFlag
	Context.NotifyGuid = NotifyEvent->Guid;
	const FCombatActionTimelineItem* Item = TimelineItems.FindByPredicate([&Context](const FCombatActionTimelineItem& TimelineItem)
	{
		return TimelineItem.NotifyGuid == Context.NotifyGuid;
	});

	if (!Item)
	{
		Context.SelectionStatus = ECombatActionSegmentSelectionStatus::NotifyNotFound;
		return Context;
	}

	Context.SelectionStatus = ECombatActionSegmentSelectionStatus::Selected;
	Context.ValidationFlags = Item->ValidationFlags;

	for (const FCombatActionValidationIssue& Issue : ValidationIssues)
	{
		if (FCombatActionEditorValidation::DoesIssueApplyToItem(*Item, Issue))
		{
			Context.Issues.Add(Issue);
		}
	}

	if (UAnimNotify_TriggerAttackDetection* Notify = Cast<UAnimNotify_TriggerAttackDetection>(NotifyEvent->Notify))
	{
		Context.ItemType = ECombatActionTimelineItemType::InstantNotify;
		Context.SegmentName = Notify->SegmentName;
	} else if (UAnimNotifyState_AttackDetection* NotifyState = Cast<UAnimNotifyState_AttackDetection>(NotifyEvent->NotifyStateClass))
	{
		Context.ItemType = ECombatActionTimelineItemType::NotifyState;
		Context.SegmentName = NotifyState->SegmentName;
	} else
	{
		Context.SelectionStatus = ECombatActionSegmentSelectionStatus::UnsupportedNotifyType;
		Context.ItemType = ECombatActionTimelineItemType::None;
		return Context;
	}

	const FAttackDetectionConfig& Config = Action->AttackDetectionConfig;
	Context.bEnableDetection = Config.bEnableDetection;
	
	const int32 SegmentBindingCount = Config.CountSegmentBindings(Context.SegmentName);
	Context.BindingCount = SegmentBindingCount;
	
	if (SegmentBindingCount == 0)
	{
		Context.ResolveStatus = ECombatActionSegmentResolveStatus::MissingBinding;
		return Context;
	} else if (SegmentBindingCount > 1)
	{
		Context.ResolveStatus = ECombatActionSegmentResolveStatus::DuplicateBinding;
		return Context;
	}

	FAttackDetectionSpec Spec;
	const FAttackDetectionSegmentBinding* SegmentBinding = Config.FindSegmentBinding(Context.SegmentName);
	if (!SegmentBinding)
	{
		Context.ResolveStatus = ECombatActionSegmentResolveStatus::MissingBinding;
		return Context;
	}

	if (!SegmentBinding->ResolveDetectionSpec(Spec))
	{
		Context.ResolveStatus = ECombatActionSegmentResolveStatus::UnresolvedSpec;
		return Context;
	}

	Context.ResolvedSpec = MoveTemp(Spec);
	
	for (const auto& Issue : Context.Issues)
	{
		if (Issue.Severity == ECombatActionValidationSeverity::Error)
		{
			Context.ResolveStatus = ECombatActionSegmentResolveStatus::HasInvalidBinding;
			return Context;
		}
	}
	
	Context.ResolveStatus = ECombatActionSegmentResolveStatus::ResolvedSuccessfully;
	return Context;
}

void FCombatActionStepEditorToolkit::RefreshSelectedSegmentContext()
{
	SelectedSegmentContext.Reset();

	SelectedSegmentContext = BuildCombatActionSegmentSelectionContext();
	
	if (SegmentInspectorWidget.IsValid())
	{
		SegmentInspectorWidget->Refresh();
	}

	RefreshAttackDetectionVisualization();
}

void FCombatActionStepEditorToolkit::RefreshAttackDetectionVisualization()
{
	// for both InstantNotify and StateNotify
	FAttackDetectionVisualizationRequest Request;
	if (SelectedSegmentContext.ResolveStatus == ECombatActionSegmentResolveStatus::ResolvedSuccessfully
		&& SelectedSegmentContext.bEnableDetection
		&& SelectedSegmentContext.ItemType != ECombatActionTimelineItemType::None
		&& SelectedSegmentContext.ResolvedSpec->DetectionMode != EAttackDetectionMode::None
		&& PreviewMeshComponent.IsValid())
	{
		Request.bEnable = true;
		Request.Spec = SelectedSegmentContext.ResolvedSpec.GetValue();
		Request.PreviewMesh = PreviewMeshComponent;
		Request.SamplingMesh = SamplingMeshComponent;
		Request.StartTime = SelectedSegmentContext.StartTime;
		Request.EndTime = SelectedSegmentContext.EndTime;
		Request.Montage = CurrentPreviewMontage;
	}

	// if condition check failed, send empty request to clear previous shape
	ViewportWidget->SetShapeQueryVisualization(Request);
}

void FCombatActionStepEditorToolkit::RefreshValidation()
{
	ValidationIssues.Reset();

	if (EditingAction.IsValid())
	{
		CombatActionValidation::Validate(*EditingAction, ValidationIssues);
	}
}

void FCombatActionStepEditorToolkit::ApplyValidationToTimelineItems()
{
	FCombatActionEditorValidation::ApplyIssuesToTimelineItems(ValidationIssues, TimelineItems);
}

void FCombatActionStepEditorToolkit::GetSaveableObjects(TArray<UObject*>& OutObjects) const
{
	FAssetEditorToolkit::GetSaveableObjects(OutObjects);

	UCombatActionStep* Action = EditingAction.Get();
	if (Action && Action->Montage)
	{
		OutObjects.AddUnique(Action->Montage);
	}
}

#undef LOCTEXT_NAMESPACE
