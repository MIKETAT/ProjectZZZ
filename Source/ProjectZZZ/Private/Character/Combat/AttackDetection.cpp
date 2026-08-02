#include "Character/Combat/AttackDetection.h"

bool FAttackDetectionSegmentBinding::ResolveDetectionSpec(FAttackDetectionSpec& OutSpec) const
{
	OutSpec = FAttackDetectionSpec{};
	switch (SpecSource)
	{
		case EAttackDetectorSpecSource::Preset:
			{
				if (!IsValid(Preset))
				{
					return false;
				}
		
				OutSpec = Preset->DetectionSpec;
				return true;
			}
		case EAttackDetectorSpecSource::Inline:
			{
				OutSpec = InlineSpec;
				return true;
			}
			default:
				break;
	}
	return false;
}

const FAttackDetectionSegmentBinding* FAttackDetectionConfig::FindSegmentBinding(const FName& InSegmentName) const
{
	return Segments.FindByPredicate([&InSegmentName](const FAttackDetectionSegmentBinding& Binding)
	{
		return Binding.SegmentName == InSegmentName;
	});
}

int32 FAttackDetectionConfig::CountSegmentBindings(const FName& InSegmentName) const
{
	int32 Count = 0;
	for (const FAttackDetectionSegmentBinding& Segment : Segments)
	{
		if (Segment.SegmentName == InSegmentName)
		{
			++Count;
		}
	}
	return Count;
}
