// Implement by Chatgpt


#include "Animation/UAnimationModifier_FixRootOffset.h"

void UUAnimationModifier_FixRootOffset::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnApply_Implementation(AnimationSequence);

	if (!AnimationSequence)
	{
		return;
	}

	//---------------------------------------
	// 获取DataModel
	//---------------------------------------

	const IAnimationDataModel* DataModel =
		AnimationSequence->GetDataModel();

	IAnimationDataController& Controller =
		AnimationSequence->GetController();

	if (!DataModel)
	{
		return;
	}

	//---------------------------------------
	// 找到Pelvis轨道
	//---------------------------------------

	const FBoneAnimationTrack* BoneTrack =
		DataModel->FindBoneTrackByName(PelvisBoneName);

	if (!BoneTrack)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Bone %s not found"),
			*PelvisBoneName.ToString());

		return;
	}

	//---------------------------------------
	// 复制原数据
	//---------------------------------------

	TArray<FVector3f> PosKeys =
		BoneTrack->InternalTrackData.PosKeys;

	TArray<FQuat4f> RotKeys =
		BoneTrack->InternalTrackData.RotKeys;

	TArray<FVector3f> ScaleKeys =
		BoneTrack->InternalTrackData.ScaleKeys;

	if (PosKeys.Num()==0)
	{
		return;
	}

	//---------------------------------------
	// 第0帧偏移
	//---------------------------------------

	FVector3f Offset=
		PosKeys[0];

	Offset.Z=0.f;

	//---------------------------------------
	// 修改所有位置Key
	//---------------------------------------

	for(FVector3f& Pos:PosKeys)
	{
		Pos.X-=Offset.X;
		Pos.Y-=Offset.Y;
	}

	//---------------------------------------
	// 写回
	//---------------------------------------

	FText text;
	Controller.OpenBracket(text);

	Controller.SetBoneTrackKeys(
		PelvisBoneName,
		PosKeys,
		RotKeys,
		ScaleKeys);

	Controller.CloseBracket();
	AnimationSequence->MarkPackageDirty();
}
